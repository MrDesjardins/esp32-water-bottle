#include <time.h>
#include <esp_now.h>
#include <WiFi.h>
#include <../shared.h>
#include <ArduinoJson.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "GxEPD2_display_selection_new_style.h"
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"

struct_message incomingData;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 10000;
float latestWeight = 0.0;
float latestPercentage = 0.0;

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, incomingData.json);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  latestWeight = doc["weight"];
  latestPercentage = doc["percentage"];

  Serial.printf("Weight: %.2f g\n", latestWeight);
  Serial.printf("Percentage: %.2f%%\n", latestPercentage);
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
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse

  initScreen();
}

void initScreen()
{
  display.setRotation(0);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  const char initText[] = "Guinea Pig Water Dashboard";
  display.getTextBounds(initText, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center the bounding box by transposition of the origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.setPartialWindow(0, 0, 50, 50);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(initText);
  }
  while (display.nextPage());
}

void updateDisplay()
{
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Header
    display.setCursor(5, 20);
    display.print("Guinea Pig Water");

    // Time
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);
    char timeStr[20];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", t);
    display.setCursor(5, 40);
    display.print("Time: ");
    display.print(timeStr);

    // Sensor values
    display.setCursor(5, 60);
    display.printf("Weight: %.2f g", latestWeight);
    display.setCursor(5, 80);
    display.printf("Fill: %.2f%%", latestPercentage);

  } while (display.nextPage());
}

void loop() {
  unsigned long now = millis();
  if (now - lastUpdate > updateInterval) {
    lastUpdate = now;
    updateDisplay();
  }
}
