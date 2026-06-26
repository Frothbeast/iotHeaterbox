#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;

char rx_buffer[21]; // Buffer for 18 chars + RSSI + terminator
bool data_ready = false;

void setup() {
  Serial.begin(115200); 

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
  if (Serial.available()) {
    if (Serial.read() == 0x02) {
      char rx_buffer[19]; 
      
      // REPLACE THE OLD WHILE LOOP WITH THIS FOR LOOP
      for (int i = 0; i < 18; i++) {
        while (!Serial.available()); // Wait until the byte arrives
        rx_buffer[i] = Serial.read();
      }
      rx_buffer[18] = '\0'; // Ensure it's null-terminated
      
      // ACK immediately after buffer is filled
      Serial.write(0x06); 
      
      // Perform network operations
      if (WiFi.status() == WL_CONNECTED) {
        if (client.connect(SERVER_IP, SERVER_PORT)) {
          client.print(rx_buffer); 
          client.stop();
        }
      }
    }
  }
}
