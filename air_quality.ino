#include <Arduino.h>
#include <HTTPClient.h>
#include <Wifi.h>
#include <TFT_eSPI.h>
#include <DHT.h>
#include "SparkFun_SGP30_Arduino_Library.h"
#include <Wire.h>           
#include "gifs/lily_happy_crop.h"  // lily gif
#include "sf_pro.h"


const char* ssid = "( ͡° ͜ʖ ͡°)";
const char* password = "OnePingToRuleThemAll";

TFT_eSPI tft = TFT_eSPI();
const int screenWidth = 120;
const int screenHeight = 240;

DHT dht(26, DHT11);
SGP30 mySensor;

float temp;
float humidity;
float co2;
float voc;

unsigned long lastUpdateTime = 0;
const unsigned long updateInterval = 60000;

String oldValues[4];

void sendDataToServer(float temp, float humidity, float co2, float voc) {
  if (WiFi.status() == WL_CONNECTED) { 
    HTTPClient http; 

    String serverPath = "https://air-control-25b7322984ae.herokuapp.com/sensor-data"; 

    String jsonPayload = "{\"temp\":" + String(temp) + 
                     ",\"hum\":" + String(humidity) + 
                     ",\"co2\":" + String(co2) + 
                     ",\"voc\":" + String(voc) + 
                     "}";

    http.begin(serverPath); 
    http.addHeader("Content-Type", "application/json");

    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}

void displayInfoText() {
  const char *texts[] = { "CO2 | ", "T | ", "VOC | ", "H | " };
  String values[] = { String(co2, 0) + "ppm", String(temp, 1) + "C", String(voc, 0) + "ppb", String(humidity, 1) + "%" };
  int numRows = 2;
  int numCols = 2;
  int cellWidth = 200 / numCols;
  int cellHeight = 20;
  int totalWidth = cellWidth * numCols;
  int startX = (240 - totalWidth) / 2 + 5;
  int yOffset = 45;

  tft.setTextDatum(BC_DATUM);

  for (int row = 0; row < numRows; row++) {
    for (int col = 0; col < numCols; col++) {
      int index = row * numCols + col;
      int x = startX + col * cellWidth + cellWidth / 2;
      int y = 240 - (numRows - row) * cellHeight + cellHeight / 2 - yOffset;

      if (values[index] != oldValues[index]) {
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawString(texts[index] + oldValues[index], x, y);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(texts[index] + values[index], x, y);
        oldValues[index] = values[index];
      }
    }
  }
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  tft.begin();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.loadFont(sfProFont);
  dht.begin();
  Wire.begin();
  
  Serial.println("Initializing SGP30...");
  if (!mySensor.begin()) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }
  mySensor.initAirQuality();
  Serial.println("SGP30 initialized.");
  delay(15000);
  displayInfoText();
}

float calculateAbsoluteHumidity(float temp, float humidity) {
  const float mw = 18.016;       // Molecular weight of water g/mol
  const float r = 8.31447215;    // Universal gas constant J/(mol*K)

  float absHumidity = (6.112 * exp((17.67 * temp) / (temp + 243.5)) * humidity * mw) / ((273.15 + temp) * r);
  return absHumidity * 1000;
}

void updateSensorReadings() {
  temp = dht.readTemperature();
  humidity = dht.readHumidity();

  if (!isnan(humidity) && !isnan(temp)) {
    uint32_t absoluteHumidity = static_cast<uint32_t>(calculateAbsoluteHumidity(temp, humidity));
    mySensor.setHumidity(absoluteHumidity);
  }

  mySensor.measureAirQuality();
  co2 = mySensor.CO2;
  voc = mySensor.TVOC;
}


void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime >= updateInterval) {
    updateSensorReadings();
    displayInfoText();
    sendDataToServer(temp, humidity, co2, voc);

    lastUpdateTime = currentTime;
  }

  for (int i = 0; i < frames; i++) {
    tft.pushImage(screenWidth / 2 - 13, 7, lily_happy_cropWidth, lily_happy_cropHeight, lily_happy_crop[i]);
    delay(200);
  }
}