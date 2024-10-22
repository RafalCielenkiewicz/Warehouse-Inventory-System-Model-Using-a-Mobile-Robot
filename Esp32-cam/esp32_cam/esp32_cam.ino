#include "Camera.h"
#include "Control.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

// Ustawienia Wi-Fi
const char* ssid = "UPC8168135";
const char* password = "v6smsjYhsmpn";

AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Inicjalizacja kamery
  setupCamera();

  // Ustawienia serwera
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Robot Live Stream and Control Panel</title>";
    html += "<style>body { font-family: Arial, sans-serif; background-color: #f0f0f0; text-align: center; }</style>";
    html += "<h2>Robot Live Stream and Control Panel</h2>";
    html += "<img src=\"/stream\" width=\"640\" height=\"480\"><br>";
    html += "</html>";
    request->send(200, "text/html", html);
  });

  server.on("/stream", HTTP_GET, handleStream);

  // Obsługa komend sterujących
  server.on("/forward", HTTP_GET, [](AsyncWebServerRequest *request) {
    sendCommand("F");
    request->send(200, "OK");
  });

  server.begin();
}

void loop() {
  // Pusta pętla
}
