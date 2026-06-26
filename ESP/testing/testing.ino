#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;
unsigned long last_send_time = 0;

void byteToHex(uint8_t val, char* buf) {
    const char hex_chars[] = "0123456789ABCDEF";
    buf[0] = hex_chars[(val >> 4) & 0x0F];
    buf[1] = hex_chars[val & 0x0F];
}

void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  if (millis() - last_send_time >= 10000) {
    
    // Create buffer: 18 chars for data + 2 chars for RSSI + 1 for null terminator
    char tx_buffer[21] = "000000000000000000"; 
    
    // Get and append RSSI
    uint8_t rssi_val = (uint8_t)abs(WiFi.RSSI());
    byteToHex(rssi_val, &tx_buffer[18]);
    tx_buffer[20] = '\0';
    
    if (WiFi.status() == WL_CONNECTED) {
      if (client.connect(SERVER_IP, SERVER_PORT)) {
        client.print(tx_buffer); // Sends 20 characters
        client.stop();
      }
    }
    
    last_send_time = millis();
  }
}