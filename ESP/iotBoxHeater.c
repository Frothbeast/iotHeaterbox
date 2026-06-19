#include <WiFi.h>
#include <WiFiManager.h> 
#include <WiFiClient.h>
#include "config.h"

WiFiClient client;
char static_ip[16] = SERVER_IP;
char static_port[6] = SERVER_PORT;

void setup() {
  Serial.begin(115200); // Debug
  Serial1.begin(9600, SERIAL_8N1, 20, 21); // PIC Connection

  WiFiManager wm;
  WiFiManagerParameter custom_ip("ip", "Target IP", static_ip, 16);
  WiFiManagerParameter custom_port("port", "Target Port", static_port, 6);
  wm.addParameter(&custom_ip);
  wm.addParameter(&custom_port);

  if (!wm.autoConnect("ESP32-Heater-Setup")) {
    ESP.restart();
  }

  strcpy(static_ip, custom_ip.getValue());
  strcpy(static_port, custom_port.getValue());
  Serial.println("System Ready.");
}

void loop() {
  // Handle PIC -> ESP-01S 
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    // 1. Intercept AT commands and reply immediately to satisfy the PIC code
    if (command.startsWith("AT+CIPSTART")) {
      Serial.println("CONNECT");
      Serial.println("OK");
    } 
    else if (command.startsWith("AT+CIPSEND")) {
      Serial.println("OK");
      Serial.print("> ");
    }
    // 2. Process the actual 10-character data payload from the PIC
    else if (command.length() >= 10) { 
      int32_t rssi_raw = WiFi.RSSI();
      uint8_t rssi_scaled = map(constrain(rssi_raw, -100, -40), -100, -40, 0, 100);
      
      char final_payload[14];
      sprintf(final_payload, "%s%02X", command.c_str(), rssi_scaled);
      
      // 1. Connection Timeout (Keep it short: 4-5 seconds)
      // If the server is offline, this stops the ESP from hanging.
      client.setTimeout(4000);
      
      
      // Attempt transactional connection to the collector server
      if (client.connect(static_ip, atoi(static_port))) {
        client.print(final_payload); // Send the 12-character payload
        
        // Wait up to 3 seconds for the Python collector to reply with "ACK"
        unsigned long timeout = millis();
        while (client.connected() && !client.available() && (millis() - timeout < 10000)) {
          delay(10);
        }
        
        // Pass the ACK or incoming bytes back to the PIC UART buffer
        if (client.available()) {
          while (client.available()) {
            Serial.write(client.read());
          }
        } else {
          // Optional: Send indicator to PIC that transmission succeeded but no ACK arrived
          Serial.print("NO ACK\r\n");
        }
        
        client.stop(); // Force close the connection to reset the server socket
        Serial.println("SEND OK");
      } else {
        // Connection failed or timed out
        Serial.println("SEND FAIL");
      }
    }
    else {
      Serial.println("OK");
    }
  }
}