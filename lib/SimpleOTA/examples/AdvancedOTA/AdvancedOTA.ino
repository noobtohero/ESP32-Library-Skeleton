#include <Arduino.h>
#include <SimpleOTA.h>
#include <WiFi.h>


const char *ssid = "YOUR_SSID";
const char *password = "YOUR_PASSWORD";

// Custom WebServer if needed (optional)
// WebServer myServer(80);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected");

  // Setup OTA
  // OTA.useServer(&myServer); // Use a custom server instance
  OTA.begin("1.0.0");
  OTA.setAuth("admin", "admin123");

  // Example of triggering online update
  // In real app, this might be triggered by a button or timer
  // OTA.onlineUpdate("http://example.com/manifest.txt");

  OTA.onUpdateError([](int err) { Serial.printf("Update Error: %d\n", err); });
}

void loop() {
  OTA.loop();
  // myServer.handleClient(); // if using custom server
}
