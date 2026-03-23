/**
 * SimpleOTA Headless / API-Only Example
 *
 * This example demonstrates how to use SimpleOTA as a background service
 * without its default Web UI. You can use it with your own Dashboard
 * or trigger updates via MQTT/other protocols.
 */

#include <Arduino.h>
#include <SimpleOTA.h>
#include <WiFi.h>


// Your existing WebServer (if any)
WebServer server(80);

void setup() {
  Serial.begin(115200);

  // Connect WiFi
  WiFi.begin("YOUR_SSID", "YOUR_PASS");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Define your own routes
  server.on("/", []() {
    server.send(
        200, "text/plain",
        "Welcome to My Own Dashboard. Use /ota/status for JSON diagnostics.");
  });

  // SimpleOTA Setup
  // Passing our own 'server' pointer allows SimpleOTA to add its API endpoints
  // (/ota/status, /ota/update, etc.) to our existing server.
  OTA.begin("1.0.0", &server);

  // Start the server
  server.begin();
  Serial.println("\nReady!");
  Serial.print("Check status at: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/ota/status");
}

void loop() {
  // SimpleOTA works in a background Task, so nothing needed here.
  // Your main loop remains clean!
}
