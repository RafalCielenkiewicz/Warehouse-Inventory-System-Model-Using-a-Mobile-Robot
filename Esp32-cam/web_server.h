#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <WebServer.h>
#include <WiFi.h>
#include "esp_camera.h"
#include "serial_websocket.h"
#include "esp_task_wdt.h"


WebServer server(80);
bool isStreaming = false;
bool isQRCodeScanning = false;

void sendCommand(const char* command) {
  Serial.print(command);
  sendSerialToWebSocket(String(command));
}

void startStream();
void stopStream();

void handleStream() {
  isStreaming = true;
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.client().setNoDelay(true);
  String response = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
                    "\r\n";
  server.client().write(response.c_str(), response.length());

  xTaskCreatePinnedToCore(
    [](void* pvParameters) {
      WiFiClient client = *((WiFiClient*)pvParameters);
      for (;;) {
        if (!client.connected()) {
          break;
        }
        camera_fb_t * fb = esp_camera_fb_get();
        if (!fb) {
          Serial.println("Camera capture failed");
          continue;
        }

        client.write("--frame\r\n");
        client.write("Content-Type: image/jpeg\r\n");
        client.printf("Content-Length: %u\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.write("\r\n");

        esp_camera_fb_return(fb);

        vTaskDelay(10 / portTICK_PERIOD_MS);

        if (!client.connected()) {
          break;
        }
      }
      client.stop();
      isStreaming = false;
      vTaskDelete(NULL);
    },
    "stream_task",
    8192,
    (void*)&server.client(),
    1,
    NULL,
    0
  );
}

void stopStream() {
  isStreaming = false;
  sendSerialToWebSocket("Stream stopped"); 
  server.on("/stream", HTTP_GET, []() {
    server.send(200, "text/html", "Stream stopped");
  });

  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    server.send(200, "image/jpeg", (const char*)fb->buf);
    esp_camera_fb_return(fb);
  } else {
    server.send(500, "text/plain", "Failed to capture frame");
  }
}

void startStream() {
  isStreaming = true;
  server.on("/stream", HTTP_GET, handleStream);
  server.send(200, "text/html", "Stream started");
  sendSerialToWebSocket("Stream started");
}

void startServer() {
  server.on("/", HTTP_GET, []() {
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Warehouse Inventory Robot</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; background-color: #f0f0f0; text-align: center; }";
    html += "h2 { color: #333; }";
    html += "#streamImage { border: 3px solid #333; margin-top: 20px; }";
    html += "button { padding: 10px 20px; margin: 5px; font-size: 16px; cursor: pointer; background-color: #4CAF50; color: white; border: none; }";
    html += "button:hover { background-color: #45a049; }";
    html += "#serialMonitor { position: fixed; top: 10px; right: 10px; width: 300px; height: 200px; overflow-y: scroll; background: #333; color: white; padding: 10px; font-size: 12px; text-align: left; }";
    html += "</style></head><body>";
    html += "<h2>Warehouse Inventory Robot</h2>";
    html += "<img src=\"/stream\" id=\"streamImage\" width=\"320\" height=\"240\"><br><br>";
    html += "<button onclick=\"fetch('/stop_stream').then(() => { document.getElementById('streamImage').src='data:image/jpeg;base64,' + btoa('Stream stopped'); })\">Stop Stream</button>";
    html += "<button onclick=\"fetch('/start_stream').then(() => { document.getElementById('streamImage').src='/stream'; })\">Start Stream</button><br><br>";
    html += "<button onclick=\"fetch('/start_qr_scan')\">Start QR Scan</button><br><br>";
    html += "<button onmousedown=\"fetch('/forward')\" onmouseup=\"fetch('/stop')\">Forward</button><br>";
    html += "<button onmousedown=\"fetch('/left')\" onmouseup=\"fetch('/stop')\">Left</button>";
    html += "<button onmousedown=\"fetch('/right')\" onmouseup=\"fetch('/stop')\">Right</button><br>";
    html += "<button onmousedown=\"fetch('/backward')\" onmouseup=\"fetch('/stop')\">Backward</button>";
    html += "<div id='serialMonitor'>Serial Monitor Output</div>";
    html += "<script>";
    html += "var ws = new WebSocket('ws://' + window.location.hostname + ':81');";
    html += "ws.onmessage = function(event) {";
    html += "  var monitor = document.getElementById('serialMonitor');";
    html += "  monitor.innerHTML += event.data + '<br>';";
    html += "  monitor.scrollTop = monitor.scrollHeight;";
    html += "};";
    html += "</script>";
    html += "</body></html>";
    server.send(200, "text/html", html);
  });

  server.on("/forward", HTTP_GET, []() {
    sendCommand("F");
    server.send(200, "OK");
  });
  server.on("/backward", HTTP_GET, []() {
    sendCommand("B");
    server.send(200, "OK");
  });
  server.on("/left", HTTP_GET, []() {
    sendCommand("L");
    server.send(200, "OK");
  });
  server.on("/right", HTTP_GET, []() {
    sendCommand("R");
    server.send(200, "OK");
  });
  server.on("/stop", HTTP_GET, []() {
    sendCommand("S");
    server.send(200, "OK");
  });

  server.on("/stream", HTTP_GET, handleStream);
  server.on("/stop_stream", HTTP_GET, []() {
    stopStream();
  });
  server.on("/start_stream", HTTP_GET, []() {
    startStream();
  });

  server.on("/start_qr_scan", HTTP_GET, []() {
    if (!isStreaming) {
      isQRCodeScanning = true;
      Serial.println("QR Scan started");
      sendSerialToWebSocket("QR Scan started");
      server.send(200, "text/html", "QR Scan started");
    } else {
      server.send(400, "text/html", "Stop the stream first to start QR scanning");
    }
  });

  server.begin();  
}

#endif 
