#include <WiFi.h>
#include <esp_now.h>
#include "../shared.h"
#include "esp_sleep.h"

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
const int sleepInSeconds = 60*60; // Sleep duration in seconds (every 60 minutes)

// ESP-NOW setup
uint8_t receiverMAC[] = {0x08, 0xB6, 0x1F, 0xB8, 0x64, 0xE8};  // Replace with your receiver's MAC
struct_message outgoingData;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // Set resolution to 12 bits (0–4095)
  btStop(); // Disable Bluetooth

  Serial.println(WiFi.macAddress());
  WiFi.mode(WIFI_STA);  // Must be in station mode
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, receiverMAC, 6);
  peerInfo.channel = 0;  // Auto
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
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

  snprintf(outgoingData.json, sizeof(outgoingData.json), "{\"weight\":%.2f,\"percentage\":%.2f}", weight, percentage);

  Serial.println("Sending JSON:");
  Serial.println(outgoingData.json);

  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending data");
  }

}