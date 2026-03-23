#include <Arduino.h>
#include <SimpleOTA.h>
#include <WiFi.h>


const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // Setup OTA
  OTA.setVersion("1.0.0");
  OTA.setAuth("admin", "admin123");

  OTA.onUpdateStart([]() { Serial.println("OTA Update Started"); });

  OTA.onUpdateEnd([]() { Serial.println("OTA Update Finished"); });

  OTA.onUpdateProgress([](int progress, int total) {
    Serial.printf("Progress: %d%%\n", (progress * 100) / total);
  });

  OTA.onUpdateError([](int error) { Serial.printf("Error: %d\n", error); });

  OTA.begin("1.0.0");

  // To access Web UI: http://<IP>/ota
  Serial.print("OTA Ready at: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ota");
}

void loop() { OTA.loop(); }
