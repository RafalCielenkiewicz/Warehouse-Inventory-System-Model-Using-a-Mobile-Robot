#include "camera_setup.h"
#include "web_server.h"
#include "wifi_setup.h"
#include "qr_code_reader.h"
#include "serial_websocket.h"

void setup() {
  Serial.begin(115200);

  initWiFi();

  if (!initCamera(false)) {
    Serial.println("The camera was not initialised correctly.");
    return;
  }

  startServer();

  initWebSocket();
}

void loop() {
  server.handleClient();
  ws.loop(); 
  delay(1); 

  if (!isStreaming && isQRCodeScanning) {
    QRCodeReader();
  }
}