#include <esp_now.h>
#include <WiFi.h>
#include <../shared.h>
#include <ArduinoJson.h>

struct_message incomingData;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  char macStr[18];
  snprintf(macStr, sizeof(macStr),
           "%02X:%02X:%02X:%02X:%02X:%02X",
           info->src_addr[0], info->src_addr[1], info->src_addr[2],
           info->src_addr[3], info->src_addr[4], info->src_addr[5]);

  Serial.printf("Received data from %s:\n", macStr);
  
  memcpy(&incomingData, data, sizeof(incomingData));

  Serial.println("Raw JSON:");
  Serial.println(incomingData.json);

  // Parse the JSON
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, incomingData.json);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract values
  float weight = doc["weight"];
  float percentage = doc["percentage"];

  Serial.printf("Weight: %.2f g\n", weight);
  Serial.printf("Percentage: %.2f%%\n", percentage);
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  Serial.print("My MAC address: ");
  Serial.println(WiFi.macAddress());

  esp_now_register_recv_cb(OnDataRecv);

  Serial.println("ESP-NOW receiver ready");
}

void loop() {
  // No need to do anything here
}
