#include <WiFi.h>
#include "secrets.h"
#include "esp_sleep.h"
#include <HTTPClient.h>

float emptyWeight = 50.0;     // Weight when bottle is empty (adjust during calibration)
float fullWeight = 550.0;     // Weight when bottle is full (adjust during calibration)

const int sensorPin = 34;  // Analog pin connected to voltage divider
const float vcc = 3.3;     // ESP32 operating voltage
const int adcMax = 4095;   // 12-bit ADC on ESP32

// Calibration parameters (adjust after testing)
const float minPressure = 0.0;     // Analog value when no weight
const float maxPressure = 3000.0;  // Analog value under known max weight
const float maxWeight = 1000.0;    // Max weight in grams corresponding to maxPressure
const int debug = 1; // 1 = debug, 0 = production mode
const int sleepInSeconds = 60; // Sleep duration in seconds

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // Set resolution to 12 bits (0–4095)
  btStop(); // Disable Bluetooth

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
    if (debug == 0) {
      Serial.println("No Wi-Fi in production mode, entering deep sleep anyway...");
      esp_sleep_enable_timer_wakeup(sleepInSeconds * 1000000);
      esp_deep_sleep_start();
    }
    return; // Exit setup if no Wi-Fi in debug mode
  }

  if(debug == 0) {
    // Production mode: send data once then sleep
    readData();
    Serial.println("Entering deep sleep...");
    esp_sleep_enable_timer_wakeup(sleepInSeconds * 1000000); // Microseconds
    esp_deep_sleep_start();
  } else {
    Serial.println("Debug mode: continuous operation");
  }
}

void loop() {
  if(debug == 1) {
    readData();
    delay(500); // Quick delay for debugging
  } 
}

void readData() {
  int analogValue = analogRead(sensorPin);
  
  // Convert to voltage if needed
  float voltage = (analogValue / float(adcMax)) * vcc;

  // Simple linear mapping from pressure to weight (requires calibration!)
  float weight = mapWeight(analogValue);
  float percentage = calculatePercentage(weight);

  Serial.print("Analog Value: ");
  Serial.print(analogValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 3);
  Serial.print(" V | Approx. Weight: ");
  Serial.print(weight, 1);
  Serial.print(" g | Percentage: ");
  Serial.print(percentage, 1);
  Serial.println(" %");

  sendData(weight, percentage);
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

void sendData(float weight, float percentage) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping data send");
    return;
  }

  HTTPClient http;
  http.begin(targetUrl);
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(10000); // 10 second timeout

  String payload = "{\"weight\":" + String(weight, 1) + ",\"percentage\":" + String(percentage, 1) + "}";
  
  int attempts = 0;
  int httpCode = -1;
  
  while (attempts < 3 && httpCode <= 0) {
    httpCode = http.POST(payload);
    
    if (httpCode > 0) {
      Serial.printf("HTTP Response code: %d\n", httpCode);
      String response = http.getString();
      if (response.length() > 0) {
        Serial.println("Response: " + response);
      }
      break;
    } else {
      attempts++;
      Serial.printf("HTTP POST failed (attempt %d/3): %s\n", attempts, http.errorToString(httpCode).c_str());
      if (attempts < 3) {
        delay(1000); // Wait 1 second before retry
      }
    }
  }
  
  if (httpCode <= 0) {
    Serial.println("All HTTP attempts failed");
  }

  http.end();
}