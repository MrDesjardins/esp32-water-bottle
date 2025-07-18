#include <WiFi.h>
#include <esp_now.h>
#include "../shared.h"
#include "esp_sleep.h"

float emptyWeight = 50.0;     // Weight when bottle is empty (adjust during calibration)
float fullWeight = 550.0;     // Weight when bottle is full (adjust during calibration)

const int sensorPin = 34;  // Analog pin connected to voltage divider
const int sensorPin2 = 35;  // Analog pin connected to voltage divider
const float vcc = 3.3;     // ESP32 operating voltage
const int adcMax = 4095;   // 12-bit ADC on ESP32

// Calibration parameters (adjust after testing)
const float minPressure = 0.0;     // Analog value when no weight
const float maxPressure = 3000.0;  // Analog value under known max weight
const float maxWeight = 1000.0;    // Max weight in grams corresponding to maxPressure
const int debug = 1; // 1 = debug, 0 = production mode
const int sleepInSeconds = 60; // Sleep duration in seconds

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
    delay(2000); // Quick delay for debugging
  } 
}

void readData() {
  int analogValue = analogRead(sensorPin);
  int analogValue2 = analogRead(sensorPin2);
  
  // Convert to voltage if needed
  float voltage = (analogValue / float(adcMax)) * vcc;
  float voltage2 = (analogValue2 / float(adcMax)) * vcc;

  // Simple linear mapping from pressure to weight (requires calibration!)
  float weight1 = mapWeight(analogValue);
  float percentage1 = calculatePercentage(weight1);

  float weight2 = mapWeight(analogValue2);
  float percentage2 = calculatePercentage(weight2);

  Serial.print("Analog Value1: ");
  Serial.println(analogValue);
  Serial.print(" | Voltage1: ");
  Serial.print(voltage, 3);
  Serial.print(" V | Approx. Weight1: ");
  Serial.print(weight1, 1);
  Serial.print(" g | Percentage1: ");
  Serial.print(percentage1, 1);
  Serial.println(" %");

  Serial.print("Analog Value2: ");
  Serial.print(analogValue2);
  Serial.print(" | Voltage2: ");
  Serial.print(voltage2, 3);
  Serial.print(" V | Approx. Weight2: ");
  Serial.print(weight2, 1);
  Serial.print(" g | Percentage2: ");
  Serial.print(percentage2, 1);
  Serial.println(" %");

  sendData(weight1, percentage1, voltage, weight2, percentage2);
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

void sendData(float weight1, float percentage1, float voltage, float weight2, float percentage2) {

  snprintf(outgoingData.json, sizeof(outgoingData.json), "{\"weight1\":%.2f,\"percentage1\":%.2f,\"voltage\":%.2f,\"weight2\":%.2f,\"percentage2\":%.2f}", weight1, percentage1, voltage, weight2, percentage2);

  Serial.println("Sending JSON:");
  Serial.println(outgoingData.json);

  esp_err_t result = esp_now_send(receiverMAC, (uint8_t *)&outgoingData, sizeof(outgoingData));
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  } else {
    Serial.println("Error sending data");
  }

}