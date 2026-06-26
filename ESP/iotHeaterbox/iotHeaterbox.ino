#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;

char rx_buffer[21]; // Buffer for 18 chars + RSSI + terminator
bool data_ready = false;

void setup() {
   Serial.begin(9600, SERIAL_8N1); 

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
  static char rx_buffer[19];
  static int index = 0;
  static bool receiving = false;

  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == 0x02) { // Start bit
      index = 0;
      receiving = true;
    } 
    else if (receiving) {
      rx_buffer[index++] = c;
      
      if (index >= 18) { // Buffer full
        rx_buffer[18] = '\0';
        receiving = false;

        Serial.write(0x06);
        
        // Network task
       if (WiFi.status() == WL_CONNECTED) {
          if (client.connect(SERVER_IP, SERVER_PORT)) {
            // Send the 18-byte buffer
            client.print(rx_buffer);
            
            // Get RSSI (it is negative, cast to absolute uint8_t for hex)
            int rssi = abs(WiFi.RSSI());
            char rssi_hex[2];
            byteToHex((uint8_t)rssi, rssi_hex);
            
            // Send the 2-char hex RSSI
            client.write(rssi_hex, 2);
            
            client.stop();
          }
        }
      }
    }
  }
}
