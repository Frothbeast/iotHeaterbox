#include <WiFi.h>
#include <WebServer.h>

// Replace with your network credentials
const char* ssid = "YOUR_SSID";
const char* password = "YOUR_PASSWORD";

WebServer server(80);

// Function to read the internal die temperature
extern "C" {
  uint8_t temprature_sens_read(); 
}

float getChipTemp() {
  return (temprature_sens_read() - 32) / 1.8;
}

String getHTML() {
  float temp = getChipTemp();
  long rssi = WiFi.RSSI();
  
  String ptr = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr += "<meta http-equiv='refresh' content='1'>"; // Auto-refresh every second
  ptr += "<style>body{font-family:sans-serif; text-align:center; padding:20px;} ";
  ptr += ".gauge{font-size:48px; font-weight:bold;} .label{color:#666;}</style></head><body>";
  ptr += "<h1>Antenna Tuner</h1>";
  
  ptr += "<div class='label'>Signal Strength (RSSI)</div>";
  ptr += "<div class='gauge' style='color:" + String(rssi > -65 ? "green" : "orange") + "'>" + String(rssi) + " dBm</div>";
  
  ptr += "<br><div class='label'>Internal Chip Temp</div>";
  ptr += "<div class='gauge' style='color:" + String(temp > 70 ? "red" : "blue") + "'>" + String(temp, 1) + " &deg;C</div>";
  
  ptr += "<p>Move the dish until RSSI is highest and Temp is lowest.</p>";
  ptr += "</body></html>";
  return ptr;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  server.on("/", []() {
    server.send(200, "text/html", getHTML());
  });
  
  server.begin();
  Serial.println("Web server started at: " + WiFi.localIP().toString());
}

void loop() {
  server.handleClient();
}