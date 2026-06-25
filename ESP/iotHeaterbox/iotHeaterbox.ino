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

void loop() {
  // Ensure Wi-Fi remains connected; if not, attempt to restart
  if (WiFi.status() != WL_CONNECTED) {
    ESP.restart();
  }

  if (Serial.available()) {
    uint8_t c = Serial.read();
    
    if (c == 0x02) {
      Serial.write(0x06);
      
      char payload[19];
      uint8_t i = 0;
      unsigned long start = millis();
      
      while(i < 18 && (millis() - start < 500)) {
        if(Serial.available()) {
            payload[i++] = Serial.read();
        }
      }
      payload[18] = '\0';
      
      // Connection with short timeout
      client.setTimeout(2000);
      if (client.connect(SERVER_IP, SERVER_PORT)) {
        client.print(payload);
        client.stop();
      }
    }
  }
}