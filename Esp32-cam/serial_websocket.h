#ifndef SERIAL_WEBSOCKET_H
#define SERIAL_WEBSOCKET_H

#include <WebSocketsServer.h>

WebSocketsServer ws(81);

void initWebSocket() {
  ws.begin();
  ws.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_TEXT) {
      Serial.printf("[WebSocket] Received: %s\n", payload);
    }
  });
}

void sendSerialToWebSocket(String data) { 
  ws.broadcastTXT(data);
}

#endif 