#include <time.h>
#include <esp_now.h>
#include <WiFi.h>
#include <../shared.h>
#include <../secrets.h>
#include <ArduinoJson.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>

#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

#include "GxEPD2_display_selection_new_style.h"
#include "GxEPD2_display_selection.h"
#include "GxEPD2_display_selection_added.h"

const int debug = 1; // 1 = debug, 0 = production mode
struct_message incomingData;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = debug == 1 ? 3000 : 30000;

float latestWeight1 = -1;
float latestPercentage1 = -1;
float latestWeight2 = -1;
float latestPercentage2 = -1;
bool hasChanged = true;

void connectToWiFi() {
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");
}

void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  memcpy(&incomingData, data, sizeof(incomingData));

  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, incomingData.json);

  if (error) {
    Serial.print("JSON parsing failed: ");
    Serial.println(error.c_str());
    return;
  }

  float threshold = 2.0; // Allows small variation to not update the display
  if( doc["percentage1"] >= latestPercentage1+threshold
    || doc["percentage1"] <= latestPercentage1-threshold
    || doc["percentage2"] >= latestPercentage2+threshold
    || doc["percentage2"] <= latestPercentage2-threshold){
    hasChanged = true;
    latestWeight1 = doc["weight1"];
    latestPercentage1 = doc["percentage1"];
    latestWeight2 = doc["weight2"];
    latestPercentage2 = doc["percentage2"];
  }
  Serial.printf("Data has changed: %d\n", hasChanged);
  Serial.printf("Weight 1: %.2f g\n", latestWeight1);
  Serial.printf("Percentage 1: %.2f%%\n", latestPercentage1);
  Serial.printf("Weight 2: %.2f g\n", latestWeight2);
  Serial.printf("Percentage 2: %.2f%%\n", latestPercentage2);
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
  if (!hasChanged){
    return;
  }
  hasChanged = false;
  int heightGauge = 200;
  int widthGauge = 70;
  int column1 = 5;
  int column2 = 200;

  int row1 = 30;
  int row2 = 290;

  int middle1 = ((column2-column1-widthGauge)/2);
  int middle2 = column2+((400-column2-widthGauge)/2);

  display.setFont(&FreeMonoBold18pt7b);  // Or your normal font
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Header
    display.setCursor(column1, row1);
    display.print("Bottle 1");

    display.setCursor(column2, row1);
    display.print("Bottle 2");

    // Sensor values
    display.setCursor(middle1, row2);
    display.printf("%.0f%%", latestPercentage1);  

    display.setCursor(middle2, row2);
    display.printf("%.0f%%", latestPercentage2);  

    // Gauges
    drawGaugeRect(middle1, row1+30, heightGauge, widthGauge, latestPercentage1);
    drawGaugeRect(middle2, row1+30, heightGauge, widthGauge, latestPercentage2);
  } while (display.nextPage());
}

void drawGaugeRect(int x, int y, int heightGauge, int widthGauge, int percentage) {
  int filled = map(percentage, 0, 100, 0, heightGauge);
  display.drawRect(x, y, widthGauge, heightGauge, GxEPD_BLACK);  // Gauge outline
  display.fillRect(x + 1, y + heightGauge - filled, widthGauge - 2, filled, GxEPD_BLACK);  // Fill bar
}


void loop() {
  unsigned long now = millis();
  if (now - lastUpdate > updateInterval) {
    lastUpdate = now;
    updateDisplay();
  }
}
