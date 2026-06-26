#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;

char rx_buffer[21]; // Buffer for 18 chars + RSSI + terminator
bool data_ready = false;

void setup() {
  Serial.begin(9600); 

  while(!Serial.available());
  if (Serial.read() == 0xAA) {
    Serial.write(0x55); // Respond
  }

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start < 15000)) {
    delay(500);
  }
}

void byteToHex(uint8_t val, char* buf) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = hex_chars[(val >> 4) & 0x0F];
    buf[1] = hex_chars[val & 0x0F];
}

void loop() {
  // 1. NON-BLOCKING SERIAL READ
  if (Serial.available()) {
    if (Serial.read() == 0x02) {
      // Read 18 bytes from PIC immediately
      uint8_t i = 0;
      while (i < 18) {
        if (Serial.available()) {
          rx_buffer[i++] = Serial.read();
        }
      }
      
      // Append RSSI
      uint8_t rssi_val = (uint8_t)abs(WiFi.RSSI());
      byteToHex(rssi_val, &rx_buffer[18]);
      rx_buffer[20] = '\0';
      
      // 2. ACK IMMEDIATELY
      Serial.write(0x06);
      data_ready = true;
    }
  }

  // 3. PROCESS NETWORK AFTER ACK
  if (data_ready) {
    if (WiFi.status() == WL_CONNECTED) {
      if (client.connect(SERVER_IP, SERVER_PORT)) {
        client.print(rx_buffer);
        client.stop();
      }
    }
    data_ready = false; // Reset for next packet
  }
}