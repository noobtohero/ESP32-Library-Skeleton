#include "SimpleOTA.h"

#ifdef ESP32
#include <FS.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WiFiClientSecure.h> // Added for HTTPS support
#include <algorithm>

#else
#error "This library is for ESP32 only"
#endif

#include "webui.h"

SimpleOTA OTA;

// --- Constructor & Destructor ---

SimpleOTA::SimpleOTA()
    : _server(nullptr), _internalServer(false),
      _autoCheckInterval(DEFAULT_INTERVAL), _lastCheckTime(0),
      _forceCheck(false), _rebootRequired(false), _isUpdating(false),
      _otaProgress(0), _otaSize(0) {
  strcpy(_currentVersion, "0.0.0");
  strcpy(_deviceID, "ESP32-Device");
  _authUser[0] = '\0';
  _authPass[0] = '\0';
}

SimpleOTA::~SimpleOTA() {
  if (_internalServer && _server) {
    delete _server;
  }
}

// --- Setup & Lifecycle ---

void SimpleOTA::begin(const char *version, WebServer *server) {
  setVersion(version);
  if (server) {
    useServer(server);
  } else {
    _server = new WebServer(80);
    _internalServer = true;
    _server->begin();
  }

  _prefs.begin("simpleota", false);

  // Load Device ID
  if (_prefs.isKey("deviceid")) {
    _prefs.getString("deviceid", _deviceID, sizeof(_deviceID));
  } else {
    strncpy(_deviceID, "ESP32-Device", sizeof(_deviceID) - 1);
    _deviceID[sizeof(_deviceID) - 1] = '\0';
  }

  // Load Cloud Config
  _manifestUrl = _prefs.getString("url", "");
  if (_prefs.isKey("interval")) {
    _autoCheckInterval = _prefs.getULong("interval", DEFAULT_INTERVAL);
  } else {
    _autoCheckInterval = DEFAULT_INTERVAL;
  }
  _prefs.end();

  _setupRoutes();

  // Create background task for handling server
  xTaskCreateUniversal(_loopTask, "OTA_Task", 8192, this, 1, &_taskHandle, 1);

  Serial.printf(
      "OTA: Config Loaded -> ID: %s, Interval: %lu hour(s), URL: %s\n",
      _deviceID, _autoCheckInterval / 3600000, _manifestUrl.c_str());
}

void SimpleOTA::useServer(WebServer *server) {
  if (_internalServer && _server) {
    delete _server;
    _internalServer = false;
  }
  _server = server;
  _setupRoutes();
}

void SimpleOTA::_loopTask(void *param) {
  SimpleOTA *ota = (SimpleOTA *)param;
  for (;;) {
    if (ota->_internalServer && ota->_server) {
      ota->_server->handleClient();
    }
    if (ota->_rebootRequired) {
      delay(500); // Wait for pending responses
      ESP.restart();
    }

    // Auto Update Check
    if (ota->_forceCheck ||
        (ota->_autoCheckInterval > 0 && ota->_manifestUrl.length() > 0)) {
      bool shouldCheck = ota->_forceCheck;
      if (!shouldCheck &&
          (millis() - ota->_lastCheckTime > ota->_autoCheckInterval)) {
        shouldCheck = true;
      }

      if (shouldCheck) {
        ota->_forceCheck = false;
        ota->_lastCheckTime = millis();
        Serial.println(">>> Checking for cloud update...");
        ota->onlineUpdate();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(2));
  }
}

// --- Configuration ---

void SimpleOTA::setVersion(const char *version) {
  strncpy(_currentVersion, version, sizeof(_currentVersion) - 1);
  _currentVersion[sizeof(_currentVersion) - 1] = '\0';
}

String SimpleOTA::getVersion() { return String(_currentVersion); }

void SimpleOTA::setAuth(const char *username, const char *password) {
  strncpy(_authUser, username, sizeof(_authUser) - 1);
  strncpy(_authPass, password, sizeof(_authPass) - 1);
  _authUser[sizeof(_authUser) - 1] = '\0';
  _authPass[sizeof(_authPass) - 1] = '\0';
}

const char *SimpleOTA::getAuthUser() { return _authUser; }

const char *SimpleOTA::getAuthPass() { return _authPass; }

void SimpleOTA::setManifestUrl(const char *url) {
  _manifestUrl = String(url);
  _prefs.begin("simpleota", false);
  _prefs.putString("url", _manifestUrl);
  _prefs.end();
}

String SimpleOTA::getManifestUrl() { return _manifestUrl; }

void SimpleOTA::setAutoCheckInterval(unsigned long hours) {
  _autoCheckInterval = hours * 3600000;
  _prefs.begin("simpleota", false);
  _prefs.putULong("interval", _autoCheckInterval);
  _prefs.end();
}

unsigned long SimpleOTA::getAutoCheckInterval() {
  return _autoCheckInterval / 3600000;
}

void SimpleOTA::setDeviceID(const char *id) {
  if (id) {
    strncpy(_deviceID, id, sizeof(_deviceID) - 1);
    _deviceID[sizeof(_deviceID) - 1] = '\0';
    _prefs.begin("simpleota", false);
    _prefs.putString("deviceid", _deviceID);
    _prefs.end();
  }
}

void SimpleOTA::saveConfigs(const char *url, unsigned long hours,
                            const char *deviceID) {
  _prefs.begin("simpleota", false);

  if (url) {
    _manifestUrl = String(url);
    _prefs.putString("url", _manifestUrl);
  }

  if (hours > 0) {
    _autoCheckInterval = hours * 3600000;
    _prefs.putULong("interval", _autoCheckInterval);
  }

  if (deviceID) {
    strncpy(_deviceID, deviceID, sizeof(_deviceID) - 1);
    _deviceID[sizeof(_deviceID) - 1] = '\0';
    _prefs.putString("deviceid", _deviceID);
  }

  _prefs.end();
}

const char *SimpleOTA::getDeviceID() const { return _deviceID; }

void SimpleOTA::clearSettings() {
  _prefs.begin("simpleota", false);
  _prefs.clear();
  _prefs.end();
  _manifestUrl = "";
  _autoCheckInterval = DEFAULT_INTERVAL;
}

size_t SimpleOTA::getOtaSize() { return _otaSize; }

bool SimpleOTA::isRebootRequired() { return _rebootRequired; }

String SimpleOTA::getLastMessage() { return _lastMessage; }

// --- Update Operations ---

void SimpleOTA::onlineUpdate(const char *manifestUrl) {
  _isUpdating = true;
  _otaProgress = 0;
  String targetUrl = (manifestUrl != nullptr)
                         ? (setManifestUrl(manifestUrl), _manifestUrl)
                         : _manifestUrl;

  Serial.println("OTA: Fetching manifest from: " + targetUrl);

  if (targetUrl.length() == 0) {
    _lastMessage = "No manifest URL provided.";
    _isUpdating = false;
    _otaProgress = 0;
    if (_errorCallback)
      _errorCallback(OTA_ERROR_NO_FILE);
    return;
  }

  String manifest = "";
  {
    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // Allow HTTPS without certificate validation
    WiFiClient client;

    HTTPClient http;
    if (targetUrl.startsWith("https://")) {
      http.begin(secureClient, targetUrl);
    } else {
      http.begin(client, targetUrl);
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
      _lastMessage = "Manifest HTTP Error: " + String(httpCode);
      _isUpdating = false;
      _otaProgress = 0;
      if (_errorCallback)
        _errorCallback(OTA_ERROR_STREAM);
      http.end();
      return;
    }
    manifest = http.getString();
    Serial.println("--- RAW MANIFEST START ---");
    Serial.println(manifest);
    Serial.println("--- RAW MANIFEST END ---");
    http.end();
  }

  // Parse manifest
  String url = "";
  String version = "";
  String md5 = "";
  if (!_parseManifest(manifest, url, version, md5)) {
    _lastMessage = "Invalid Manifest Content";
    _isUpdating = false;
    _otaProgress = 0;
    Serial.println("OTA Error: " + _lastMessage);
    if (_errorCallback)
      _errorCallback(OTA_ERROR_MANIFEST);
    return;
  }

  Serial.println("OTA: Manifest Parsed. Version: " + version + " URL: " + url);

  // Check version if needed
  if (version.length() > 0 && String(_currentVersion) == version) {
    _lastMessage = "Already on target version: " + version;
    _isUpdating = false;
    _otaProgress = 0;
    return;
  }

  // Start Update from URL
  {
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    WiFiClient client;

    HTTPClient http;
    if (url.startsWith("https://")) {
      http.begin(secureClient, url);
    } else {
      http.begin(client, url);
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
      _lastMessage = "Firmware HTTP Error: " + String(httpCode);
      _isUpdating = false;
      _otaProgress = 0;
      if (_errorCallback)
        _errorCallback(OTA_ERROR_STREAM);
      http.end();
      return;
    }

    int totalLength = http.getSize();
    _otaSize = totalLength;

    if (md5.length() > 0) {
      Update.setMD5(md5.c_str());
    }

    if (Update.begin(totalLength)) {
      if (_startCallback)
        _startCallback();

      WiFiClient *stream = http.getStreamPtr();

      // Use chunks for progress reporting
      uint8_t buffer[1024];
      size_t totalWritten = 0;
      while (http.connected() && totalWritten < totalLength) {
        size_t available = stream->available();
        if (available > 0) {
          size_t toRead = std::min(available, sizeof(buffer));
          size_t readLen = stream->readBytes(buffer, toRead);
          if (Update.write(buffer, readLen) != readLen) {
            _lastMessage =
                "Update Write Error: " + String(Update.errorString());
            break;
          }
          totalWritten += readLen;
          int progress = (totalWritten * 100) / totalLength;
          if (progress != _otaProgress) {
            _otaProgress = progress;
            if (_progressCallback) {
              _progressCallback(totalWritten, totalLength);
            }
          }
        }
        delay(1);
      }

      if (totalWritten == totalLength) {
        if (Update.end(true)) {
          if (Update.isFinished()) {
            _lastMessage = "Success";
            if (_endCallback)
              _endCallback();
            _rebootRequired = true;
          } else {
            _lastMessage = "Update not finished";
          }
          _lastMessage = "Update End Error: " + String(Update.errorString());
          _isUpdating = false;
          _otaProgress = 0;
        }
      } else {
        _lastMessage = "Written mismatch: " + String(totalWritten) + "/" +
                       String(totalLength);
        _isUpdating = false;
        _otaProgress = 0;
      }
    } else {
      _lastMessage = "Update Begin Error: " + String(Update.errorString());
      _isUpdating = false;
      _otaProgress = 0;
    }
    http.end();
  }
}

void SimpleOTA::fileUpdate(const char *path, const char *version) {
  _isUpdating = true;
  _otaProgress = 0;
  if (!LittleFS.exists(path)) {
    _isUpdating = false;
    _otaProgress = 0;
    if (_errorCallback)
      _errorCallback(OTA_ERROR_NO_FILE);
    return;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    _isUpdating = false;
    _otaProgress = 0;
    if (_errorCallback)
      _errorCallback(OTA_ERROR_FILE_READ);
    return;
  }

  size_t fileSize = file.size();
  if (_startCallback)
    _startCallback();

  if (!Update.begin(fileSize)) {
    _isUpdating = false;
    if (_errorCallback)
      _errorCallback(Update.getError());
    file.close();
    return;
  }

  // Write file to Update in chunks for progress reporting
  uint8_t buffer[1024];
  size_t totalWritten = 0;
  while (file.available()) {
    size_t len = file.read(buffer, sizeof(buffer));
    if (Update.write(buffer, len) != len) {
      Update.printError(Serial);
      break;
    }
    totalWritten += len;
    int progress = (totalWritten * 100) / fileSize;
    if (progress != _otaProgress) {
      _otaProgress = progress;
      if (_progressCallback) {
        _progressCallback(totalWritten, fileSize);
      }
    }
  }

  if (totalWritten == fileSize && Update.end(true)) {
    if (_endCallback)
      _endCallback();
    _rebootRequired = true;
  } else {
    _isUpdating = false;
    _otaProgress = 0;
    if (_errorCallback)
      _errorCallback(Update.getError());
  }
}

bool SimpleOTA::rollback() {
  if (Update.canRollBack()) {
    if (Update.rollBack()) {
      Serial.println("Rollback success");
      return true;
    } else {
      Serial.println("Rollback failed");
    }
  } else {
    Serial.println("Rollback not possible");
  }
  return false;
}

// --- Callbacks ---

void SimpleOTA::onUpdateStart(OTACallback cb) { _startCallback = cb; }

void SimpleOTA::onUpdateEnd(OTACallback cb) { _endCallback = cb; }

void SimpleOTA::onUpdateProgress(OTAProgressCallback cb) {
  _progressCallback = cb;
}

void SimpleOTA::onUpdateError(OTAErrorCallback cb) { _errorCallback = cb; }

// --- Web Server & Routing ---

void SimpleOTA::_setupRoutes() {
  if (!_server)
    return;

  _server->on("/ota", HTTP_GET, [this]() {
    if (_authUser[0] != '\0' &&
        !_server->authenticate(getAuthUser(), getAuthPass())) {
      return _server->requestAuthentication();
    }
    _handleOTAIndex();
  });

  _server->on("/ota/status", HTTP_GET, [this]() { _handleOTAStatus(); });

  _server->on(
      "/ota/fileUpdate", HTTP_POST,
      [this]() {
        if (_authUser[0] != '\0' &&
            !_server->authenticate(getAuthUser(), getAuthPass())) {
          return _server->requestAuthentication();
        }
        _server->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      },
      [this]() {
        if (_authUser[0] != '\0' &&
            !_server->authenticate(getAuthUser(), getAuthPass())) {
          return;
        }
        _handleOTAUpdate();
      });

  _server->on("/ota/rollback", HTTP_POST, [this]() { _handleOTARollback(); });

  _server->on("/ota/saveConfigs", HTTP_POST, [this]() {
    if (_authUser[0] != '\0' &&
        !_server->authenticate(getAuthUser(), getAuthPass())) {
      return _server->requestAuthentication();
    }

    String urlStr = _server->hasArg("url") ? _server->arg("url") : "";
    unsigned long interval =
        _server->hasArg("interval") ? _server->arg("interval").toInt() : 0;
    String deviceIDStr =
        _server->hasArg("deviceID") ? _server->arg("deviceID") : "";

    saveConfigs(urlStr.length() > 0 ? urlStr.c_str() : nullptr, interval,
                deviceIDStr.length() > 0 ? deviceIDStr.c_str() : nullptr);

    _forceCheck = true; // Trigger update check immediately
    _server->send(200, "text/plain", "Configuration saved.");
  });

  _server->on("/ota/onlineUpdate", HTTP_POST, [this]() {
    if (_authUser[0] != '\0' &&
        !_server->authenticate(getAuthUser(), getAuthPass())) {
      return _server->requestAuthentication();
    }
    if (_server->hasArg("url")) {
      setManifestUrl(_server->arg("url").c_str());
    }
    _isUpdating = true; // Set flag immediately
    _forceCheck = true; // Trigger update check immediately
    _server->send(200, "text/plain", "Update process started.");
  });
}

void SimpleOTA::_handleOTAIndex() {
  _server->sendHeader("Content-Encoding", "gzip");
  _server->send_P(200, "text/html", (const char *)INDEX_HTML_GZ,
                  INDEX_HTML_GZ_LEN);
}

void SimpleOTA::_handleOTAUpdate() {
  HTTPUpload &upload = _server->upload();
  if (upload.status == UPLOAD_FILE_START) {
    _isUpdating = true;
    _otaProgress = 0;
    _otaSize = 0;

    if (_startCallback)
      _startCallback();

    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    }

    if (Update.size() > 0) {
      _otaProgress = Update.progress() * 100 / Update.size();
    }

    if (_progressCallback) {
      _progressCallback(Update.progress(), Update.size());
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) { // true to set the size to the current progress
      Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      if (_endCallback)
        _endCallback();
      _rebootRequired = true;
    } else {
      Update.printError(Serial);
      if (_errorCallback)
        _errorCallback(Update.getError());
    }
  }
}

void SimpleOTA::_handleOTAStatus() {
  String json = "{";
  json += "\"version\":\"" + getVersion() + "\",";
  json += "\"deviceID\":\"" + String(getDeviceID()) + "\",";
  json += "\"freespace\":" + String(ESP.getFreeSketchSpace()) + ",";
  json += "\"url\":\"" + getManifestUrl() + "\",";
  json += "\"interval\":" + String(getAutoCheckInterval()) + ",";
  json += "\"isUpdating\":" + String(_isUpdating ? "true" : "false") + ",";
  json += "\"progress\":" + String(_otaProgress) + ",";
  json += "\"lastMessage\":\"" + getLastMessage() + "\"";
  json += "}";
  _server->send(200, "application/json", json);
}

void SimpleOTA::_handleOTARollback() {
  if (rollback()) {
    _server->send(200, "text/plain", "Rollback Success. Rebooting...");
    _rebootRequired = true;
  } else {
    _server->send(500, "text/plain", "Rollback Failed");
  }
}

// --- Helpers ---

bool SimpleOTA::_parseManifest(const String &content, String &url,
                               String &version, String &md5) {
  int lineStart = 0;
  bool foundUrl = false;
  while (lineStart < content.length()) {
    int lineEnd = content.indexOf('\n', lineStart);
    if (lineEnd == -1)
      lineEnd = content.length();
    String line = content.substring(lineStart, lineEnd);
    line.trim();

    if (line.startsWith("url=")) {
      url = line.substring(4);
      foundUrl = true;
    } else if (line.startsWith("version=")) {
      version = line.substring(8);
    } else if (line.startsWith("md5=")) {
      md5 = line.substring(4);
    }
    lineStart = lineEnd + 1;
  }
  return foundUrl;
}

String SimpleOTA::_getContentType(String filename) {
  if (filename.endsWith(".html"))
    return "text/html";
  else if (filename.endsWith(".css"))
    return "text/css";
  else if (filename.endsWith(".js"))
    return "application/javascript";
  else if (filename.endsWith(".ico"))
    return "image/x-icon";
  else if (filename.endsWith(".gz"))
    return "application/x-gzip";
  return "text/plain";
}
