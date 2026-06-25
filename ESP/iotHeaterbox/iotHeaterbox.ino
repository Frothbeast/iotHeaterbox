#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;

void setup() {
  Serial.begin(9600); 

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
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  if (Serial.available()) {
    if (Serial.read() == 0x02) {
      Serial.write(0x06); // Send ACK to PIC
      
      char payload[21]; // 18 from PIC + 2 for RSSI + null terminator
      uint8_t i = 0;
      unsigned long start = millis();
      
      // Read the 18 chars from PIC
      while(i < 18 && (millis() - start < 500)) {
        if(Serial.available()) {
            payload[i++] = Serial.read();
        }
      }
      
      // Get RSSI (convert negative int to absolute byte)
      int32_t rssi = WiFi.RSSI();
      uint8_t rssi_val = (uint8_t)abs(rssi); 
      
      // Append RSSI as 2 hex chars
      byteToHex(rssi_val, &payload[18]);
      payload[20] = '\0';
      
      // Send total 20 chars to server
      client.setTimeout(2000);
      if (client.connect(SERVER_IP, SERVER_PORT)) {
        client.print(payload);
        client.stop();
      }
    }
  }
}