#ifndef QR_CODE_READER_H
#define QR_CODE_READER_H

#include "esp_camera.h"
#include "quirc.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "quirc.h"

#ifdef __cplusplus
}
#endif

#include "Arduino.h" 

struct quirc *q = NULL;
uint8_t *image = NULL;
camera_fb_t *fb = NULL;
struct quirc_code code;
struct quirc_data data;
quirc_decode_error_t err;
String QRCodeResult = "";

void dumpData(const struct quirc_data *data) {
  Serial.printf("Payload: %s\n", data->payload);
  QRCodeResult = (const char *)data->payload;
  sendSerialToWebSocket(QRCodeResult); 

  sendQRCodeToServer(QRCodeResult);
}


bool detectQRCode() {
  q = quirc_new();
  if (q == NULL) {
    Serial.println("Unable to create quirc object");
    return false;
  }

  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Image capture error");
    quirc_destroy(q);
    return false;
  }

  if (quirc_resize(q, fb->width, fb->height) < 0) {
    Serial.println("Error resizing quirc object");
    esp_camera_fb_return(fb);
    quirc_destroy(q);
    return false;
  }

  image = quirc_begin(q, NULL, NULL);
  memcpy(image, fb->buf, fb->len);
  quirc_end(q);

  bool qrDetected = (quirc_count(q) > 0);

  esp_camera_fb_return(fb);
  quirc_destroy(q);

  return qrDetected;
}

void QRCodeReader() {

  esp_camera_deinit();
  gpio_uninstall_isr_service(); 

  if (!initCamera(true)) {
    Serial.println("Camera not correctly initialised for QR scanning");
    return;
  }

  unsigned long startTime = millis();
  bool qrFound = false;

  while (isQRCodeScanning) {
    if (millis() - startTime > 5000) {
      Serial.println("Scanning after 5 seconds failed");
      sendSerialToWebSocket("Scanning after 5 seconds failed");
      isQRCodeScanning = false;
      break;
    }


    if (detectQRCode()) {
      q = quirc_new();
      if (q == NULL) {
        Serial.println("Unable to create quirc object");
        return;
      }

      fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Image capture error");
        quirc_destroy(q);
        return;
      }

      if (quirc_resize(q, fb->width, fb->height) < 0) {
        Serial.println("Error resizing quirc object");
        esp_camera_fb_return(fb);
        quirc_destroy(q);
        return;
      }

      image = quirc_begin(q, NULL, NULL);
      memcpy(image, fb->buf, fb->len);
      quirc_end(q);

      if (quirc_count(q) > 0) {
        quirc_extract(q, 0, &code);
        err = quirc_decode(&code, &data);
        if (err) {
          Serial.println("Decoding unsuccessful");
        } else {
          Serial.println("Decoding successful:");
          dumpData(&data);
          isQRCodeScanning = false; 
          qrFound = true;
        }
      } else {
        Serial.println("QR code not found");
      }

      esp_camera_fb_return(fb);
      quirc_destroy(q);
    } else {
      Serial.println("QR code not found");
    }

    delay(500); 
  }

  if (!qrFound) {
   Serial.println("Scanning completed without success");
   sendSerialToWebSocket("Scanning completed without success");
}


  esp_camera_deinit();
  gpio_uninstall_isr_service(); 
  if (!initCamera(false)) {
    Serial.println("The camera was not correctly initialized after QR scanning");
  }
}

#endif 