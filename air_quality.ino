#include <TFT_eSPI.h>
#include <DHT.h>
#include "SparkFun_SGP30_Arduino_Library.h"
#include <Wire.h>
#include "sf_pro.h"                // SF Pro font
#include "gifs/lily_happy_crop.h"  // lily gif

TFT_eSPI tft = TFT_eSPI();
const int screenWidth = 120;
const int screenHeight = 240;

DHT dht(26, DHT11);
SGP30 mySensor;

float temp;
float humidity;
float co2;
float voc;

// DHT11
unsigned long lastUpdateTime = 0;  // Variable to store the last update time
const unsigned long updateInterval = 5000;

String oldValues[4];

void setup() {
  Serial.begin(115200);

  // Initialize TFT
  Serial.println("Initializing TFT...");
  tft.begin();
  tft.setRotation(0);
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.loadFont(sfProFont);
  Serial.println("TFT initialized.");

  // Initialize DHT
  Serial.println("Initializing DHT...");
  dht.begin();
  Serial.println("DHT initialized.");

  // Initialize I2C and SGP30
  Serial.println("Initializing I2C...");
  Wire.begin();
  Serial.println("I2C initialized.");
  
  Serial.println("Initializing SGP30...");
  if (!mySensor.begin()) {
    Serial.println("No SGP30 Detected. Check connections.");
    while (1);
  }
  mySensor.initAirQuality();
  Serial.println("SGP30 initialized.");

  // Allow sensor warm-up time
  Serial.println("Allowing sensor warm-up time...");
  delay(15000); // 15 seconds warm-up time
  Serial.println("Warm-up complete.");
}

float calculateAbsoluteHumidity(float temp, float humidity) {
  // Constants for the calculation
  const float mw = 18.016;       // Molecular weight of water g/mol
  const float r = 8.31447215;    // Universal gas constant J/(mol*K)

  // Calculate absolute humidity
  float absHumidity = (6.112 * exp((17.67 * temp) / (temp + 243.5)) * humidity * mw) / ((273.15 + temp) * r);
  return absHumidity * 1000; // convert to g/m^3
}

void updateSensorReadings() {
  // Read humidity and temperature
  temp = dht.readTemperature();
  humidity = dht.readHumidity();

  // Set humidity for SGP30 sensor
  if (!isnan(humidity) && !isnan(temp)) {
    uint32_t absoluteHumidity = static_cast<uint32_t>(calculateAbsoluteHumidity(temp, humidity));
    mySensor.setHumidity(absoluteHumidity);
  }

  // Read air quality
  mySensor.measureAirQuality();
  co2 = mySensor.CO2;
  voc = mySensor.TVOC;
}

void displayInfoText() {
  const char *texts[] = { "CO2 | ", "T | ", "VOC | ", "H | " };
  String values[] = { String(co2, 0) + "ppm", String(temp, 1) + "C", String(voc, 0) + "ppb", String(humidity, 1) + "%" };
  int numRows = 2;
  int numCols = 2;
  int cellWidth = 180 / numCols;
  int cellHeight = 20;
  int totalWidth = cellWidth * numCols;
  int startX = (240 - totalWidth) / 2 - 5;
  int yOffset = 35;

  tft.setTextDatum(BC_DATUM);

  for (int row = 0; row < numRows; row++) {
    for (int col = 0; col < numCols; col++) {
      int index = row * numCols + col;
      int x = startX + col * cellWidth + cellWidth / 2;
      int y = 240 - (numRows - row) * cellHeight + cellHeight / 2 - yOffset;

      if (values[index] != oldValues[index]) {
        // Erase old text
        tft.setTextColor(TFT_BLACK, TFT_BLACK);
        tft.drawString(texts[index] + oldValues[index], x, y);

        // Draw new text
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.drawString(texts[index] + values[index], x, y);

        // Update old value
        oldValues[index] = values[index];
      }
    }
  }
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastUpdateTime >= updateInterval) {
    updateSensorReadings();
    displayInfoText();
    lastUpdateTime = currentTime;
  }

  for (int i = 0; i < frames; i++) {
    tft.pushImage(screenWidth / 4 - 5, 0, lily_happy_cropWidth, lily_happy_cropHeight, lily_happy_crop[i]);
    delay(200);
  }
}
