#include <WiFi.h>
#include <WebServer.h>
#include "secrets.h"

WebServer server(80); // HTTP server on port 80
float latestWeight = 0.0;
float emptyWeight = 50.0;     // Weight when bottle is empty (adjust during calibration)
float fullWeight = 550.0;     // Weight when bottle is full (adjust during calibration)

const int sensorPin = 34;  // Analog pin connected to voltage divider
const float vcc = 3.3;     // ESP32 operating voltage
const int adcMax = 4095;   // 12-bit ADC on ESP32

// Calibration parameters (adjust after testing)
const float minPressure = 0.0;     // Analog value when no weight
const float maxPressure = 3000.0;  // Analog value under known max weight
const float maxWeight = 1000.0;    // Max weight in grams corresponding to maxPressure

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // Set resolution to 12 bits (0–4095)

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to Wi-Fi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to Wi-Fi!");
    // You might want to handle this error condition
  }

  // Define HTTP endpoints
  server.on("/weight", HTTP_GET, []() {
    float percentage = calculatePercentage(latestWeight);
    String response = "{\"weight\":" + String(latestWeight, 2) + 
                     ",\"percentage\":" + String(percentage, 1) + 
                     ",\"empty_weight\":" + String(emptyWeight, 1) + 
                     ",\"full_weight\":" + String(fullWeight, 1) + "}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", response);
  });

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", 
      "<h1>Water Bottle Sensor</h1>"
      "<p>Current weight: " + String(latestWeight, 2) + "g</p>"
      "<p>Fullness: " + String(calculatePercentage(latestWeight), 1) + "%</p>"
      "<p><a href='/weight'>JSON API</a></p>");
  });

  server.begin(); // Start HTTP server
}

void loop() {
  int analogValue = analogRead(sensorPin);

  // Convert to voltage if needed
  float voltage = (analogValue / float(adcMax)) * vcc;

  // Simple linear mapping from pressure to weight (requires calibration!)
  float weight = mapWeight(analogValue);
  latestWeight = weight; // Update the global variable

  Serial.print("Analog Value: ");
  Serial.print(analogValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V | Approx. Weight: ");
  Serial.print(weight, 1);
  Serial.println(" g");

  server.handleClient();  // Handle any incoming HTTP request

  delay(500);
}

float mapWeight(int analogValue) {
  if (analogValue < minPressure) return 0;
  if (analogValue > maxPressure) return maxWeight;

  // Linear interpolation
  return (analogValue - minPressure) * (maxWeight / (maxPressure - minPressure));
}

float calculatePercentage(float currentWeight) {
  if (currentWeight <= emptyWeight) return 0.0;
  if (currentWeight >= fullWeight) return 100.0;
  
  // Calculate percentage between empty and full
  return ((currentWeight - emptyWeight) / (fullWeight - emptyWeight)) * 100.0;
}
