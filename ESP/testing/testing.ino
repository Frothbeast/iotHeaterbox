#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include "config.h" // Ensure WIFI_SSID, WIFI_PASS, SERVER_IP, and SERVER_PORT are defined here

WiFiClient client;
unsigned long last_send_time = 0;

void setup() {
  // Initialize Serial for debugging purposes
  Serial.begin(9600); 

  // Connect to WiFi using credentials from config.h [cite: 3]
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  // Wait for connection [cite: 4, 5]
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void loop() {
  // Send test packet every 10,000 milliseconds (10 seconds)
  if (millis() - last_send_time >= 10000) {
    
    if (WiFi.status() == WL_CONNECTED) {
      if (client.connect(SERVER_IP, SERVER_PORT)) {
        // Send a simple static test string to prove server communication [cite: 11]
        client.print("TEST_DATA_PACKET");
        client.stop(); // Properly close the connection [cite: 12]
      }
    }
    
    last_send_time = millis(); // Reset the timer
  }
}