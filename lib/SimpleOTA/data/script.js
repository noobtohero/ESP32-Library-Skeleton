document.getElementById("fileInput").addEventListener("change", function (e) {
  if (e.target.files.length > 0) {
    document.getElementById("fileName").value = e.target.files[0].name;
  }
});

const statusMessage = document.getElementById("statusMessage");
const freezeOverlay = document.getElementById("freezeOverlay");
const freezeText = document.getElementById("freezeText");
const freezeProgress = document.getElementById("freezeProgress");
const overlayProgressCircle = document.getElementById("overlayProgressCircle");
const circularContainer = document.querySelector(".circular-progress");
const circleCircumference = 2 * Math.PI * 45;

function showMessage(type, text) {
  statusMessage.className = "message " + type;
  statusMessage.textContent = text;
  statusMessage.style.display = "block";
  window.scrollTo({ top: 0, behavior: "smooth" });
}

function freezeUI(text) {
  freezeText.textContent = text || "Updating...";
  freezeProgress.textContent = "0%";
  if (overlayProgressCircle) {
    overlayProgressCircle.style.strokeDashoffset = circleCircumference;
  }
  if (circularContainer) circularContainer.style.display = "block";
  freezeOverlay.style.display = "flex";
}

function unfreezeUI() {
  freezeOverlay.style.display = "none";
}

function startPolling(expectReboot = true) {
  freezeUI(
    expectReboot
      ? "Device is rebooting... Please wait."
      : "Waiting for device..."
  );
  if (expectReboot && circularContainer) {
    circularContainer.style.display = "none";
  }
  let attempts = 0;
  let wasUpdating = false;

  const poll = () => {
    attempts++;
    fetch("/ota/status")
      .then((response) => {
        if (response.ok) {
          response
            .json()
            .then((data) => {
              // Case 1: Was updating, now is not (even if never saw it offline)
              if (wasUpdating && !data.isUpdating) {
                if (circularContainer) circularContainer.style.display = "none";
                freezeText.textContent = "Back online! Refreshing...";
                setTimeout(() => location.reload(), 1000);
                return;
              }

              wasUpdating = data.isUpdating;
              if (data.isUpdating) {
                freezeText.textContent =
                  "Update in progress... Waiting for reboot.";
                if (data.progress >= 0 && overlayProgressCircle) {
                  const offset =
                    circleCircumference -
                    (data.progress / 100) * circleCircumference;
                  overlayProgressCircle.style.strokeDashoffset = offset;
                  freezeProgress.textContent = data.progress + "%";
                } else {
                  freezeProgress.textContent = "";
                }
              } else if (attempts > 6 && expectReboot) {
                // Fallback: If it's been a while and still not updating, just refresh
                setTimeout(() => location.reload(), 1000);
                return;
              }
              setTimeout(poll, 2000);
            })
            .catch(() => setTimeout(poll, 2000));
        } else {
          wasUpdating = true; // Connection failed/404, assume updating/rebooting
          setTimeout(poll, 2000);
        }
      })
      .catch(() => {
        wasUpdating = true;
        console.log("Waiting for device... (" + attempts + ")");
        if (attempts > 120) {
          unfreezeUI();
          showMessage("error", "Reboot timeout. Please check device.");
          return;
        }
        setTimeout(poll, 2000);
      });
  };

  setTimeout(poll, 2000);
}

document.getElementById("otaForm").addEventListener("submit", function (e) {
  e.preventDefault();
  const fileInput = document.getElementById("fileInput");
  const file = fileInput.files[0];

  if (!file) {
    showMessage("error", "Please select a file first");
    return;
  }

  freezeUI("Uploading Firmware...");
  statusMessage.style.display = "none";

  const xhr = new XMLHttpRequest();
  xhr.open("POST", "/ota/fileUpdate");

  xhr.upload.onprogress = function (e) {
    if (e.lengthComputable) {
      const percent = (e.loaded / e.total) * 100;
      const offset =
        circleCircumference - (percent / 100) * circleCircumference;
      if (overlayProgressCircle)
        overlayProgressCircle.style.strokeDashoffset = offset;
      freezeProgress.textContent = Math.round(percent) + "%";
    }
  };

  xhr.onload = function () {
    if (xhr.status === 200 && xhr.responseText === "OK") {
      showMessage("success", "Update Successful! Rebooting...");
      startPolling(true);
    } else {
      unfreezeUI();
      showMessage(
        "error",
        "Update Failed: " + (xhr.responseText || xhr.statusText)
      );
    }
  };

  xhr.onerror = function () {
    unfreezeUI();
    showMessage("error", "Network Error");
  };

  const formData = new FormData();
  formData.append("firmware", file);
  xhr.send(formData);
});

// Config Form
document.getElementById("configForm").addEventListener("submit", function (e) {
  e.preventDefault();

  const manifestUrl = document.getElementById("manifestUrl").value;
  const deviceID = document.getElementById("deviceID").value;
  const checkInterval = document.getElementById("checkInterval").value;

  const configData = new FormData();
  configData.append("url", manifestUrl);
  configData.append("interval", checkInterval);
  configData.append("deviceID", deviceID);

  fetch("/ota/saveConfigs", { method: "POST", body: configData })
    .then(() => {
      showMessage("success", "Settings Saved Successfully");
      // Update local labels immediately
      document.getElementById("deviceName").textContent = deviceID;
    })
    .catch(() => showMessage("error", "Error saving settings"));
});

// Cloud Update Now
document
  .getElementById("cloudUpdateBtn")
  .addEventListener("click", function () {
    const url = document.getElementById("manifestUrl").value;
    if (!url) return alert("Please enter a Manifest URL first");

    if (confirm("Check for updates from: " + url + "?")) {
      freezeUI("Triggering Cloud Update...");
      const formData = new FormData();
      if (url) formData.append("url", url);

      fetch("/ota/onlineUpdate", { method: "POST", body: formData })
        .then((r) => {
          if (r.ok) {
            showMessage(
              "success",
              "Cloud Update Initialized. Waiting for device..."
            );
            startPolling(true);
          } else {
            unfreezeUI();
            showMessage("error", "Failed to initialize cloud update.");
          }
        })
        .catch(() => {
          unfreezeUI();
          showMessage("error", "Network error initiating update.");
        });
    }
  });

// Rollback Recovery
document.getElementById("rollbackBtn").addEventListener("click", function () {
  if (
    confirm(
      "Are you sure you want to rollback? The device will reboot immediately."
    )
  ) {
    freezeUI("Initiating Rollback...");
    fetch("/ota/rollback", { method: "POST" })
      .then((r) => {
        if (r.ok) {
          startPolling();
        } else {
          unfreezeUI();
          showMessage("error", "Rollback failed.");
        }
      })
      .catch(() => {
        unfreezeUI();
        showMessage("error", "Network Error.");
      });
  }
});

// Initial Load
function loadStatus() {
  fetch("/ota/status")
    .then((r) => r.json())
    .then((data) => {
      if (data.version)
        document.getElementById("currentVersion").textContent = data.version;
      if (data.url) document.getElementById("manifestUrl").value = data.url;
      if (data.deviceID) {
        document.getElementById("deviceName").textContent = data.deviceID;
        document.getElementById("deviceID").value = data.deviceID;
      }
      if (data.freespace) {
        const mb = (data.freespace / (1024 * 1024)).toFixed(2);
        document.getElementById("freeSpace").textContent = mb + " MB";
      }
      if (data.interval !== undefined)
        document.getElementById("checkInterval").value = data.interval;
      if (
        data.lastMessage &&
        data.lastMessage !== "" &&
        data.lastMessage !== "Success"
      ) {
        showMessage("info", "Last Status: " + data.lastMessage);
      }
    })
    .catch(() => console.warn("Device offline"));
}

window.onload = loadStatus;
