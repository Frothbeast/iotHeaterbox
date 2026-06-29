#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;
bool data_ready = false; 
unsigned long last_attempt = 0;

char rx_buffer[21]; // Buffer for 18 chars + RSSI + terminator

void setup() {
   Serial.begin(9600, SERIAL_8N1); 
  
  unsigned long serialStart = millis();
  while(!Serial.available() && (millis() - serialStart < 5000)){yield()};
  if (Serial.available() && Serial.read() == 0xAA) {
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
  static int index = 0;
  static bool receiving = false;
  WiFiClient serverClient = server.available(); // Assuming you have a WiFiServer set up
  if (serverClient.connected()) {
    if (serverClient.available() >= 4) {
      char cmd[4];
      serverClient.readBytes(cmd, 4); 
      Serial.write((uint8_t*)cmd, 4);
      serverClient.write(0x06); 
      serverClient.stop();
    }
  }

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
        data_ready = true;
        receiving = false;
        Serial.write(0x06);      
      }
    }
  }  
  if (data_ready && WiFi.status() == WL_CONNECTED) {
    if (client.connect(SERVER_IP, SERVER_PORT)) {
      int rssi = abs(WiFi.RSSI());
      byteToHex((uint8_t)rssi, &rx_buffer[18]); 
      rx_buffer[20] = '\0';
      client.write((uint8_t*)rx_buffer, 20);
      delay(100); 
      client.stop();
    }
    data_ready = false;
  }
}
