#include <Arduino.h>
#include <TinyGPSPlus.h>
#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"


// The TinyGPSPlus object
TinyGPSPlus gps;
unsigned long lastRead = 0;

CsvLogStore csvLog;
CsvLogResult csvResult;

static constexpr uint32_t LOG_INTERVAL_MS = 20;  // 50Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 64;

static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;

static void stopWithError(const char* message) {
  Serial.println(message);
  ledOn(STATUS_LED_COLOR_ERROR_RED);

  while (1) {
    delay(1000);
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 44, 43);  //G44->RXD, G43->TXD
  ledBegin();
  delay(1500);

  if (!csvLog.begin()) {
    stopWithError("CsvLogStore begin failed.");
  }

  csvLog.setFlushEveryRows(25);
  csvLog.setFlushIntervalMs(5000);

  if (!csvLog.openNext()) {
    stopWithError("CSV log open failed.");
  }

  Serial.print("CSV log opened: ");
  Serial.println(csvLog.currentPath());

  lastLogMs = millis();
  Serial.println(F("GPS V1.1 Sample3"));
}

void loop() {
  static char csvLine[CSV_ROW_MAX_LENGTH];

  while (Serial2.available() > 0) {
    gps.encode((char)Serial2.read());
  }

  uint32_t now = millis();

  if (now - lastLogMs < LOG_INTERVAL_MS) {
    return;
  }

  lastLogMs += LOG_INTERVAL_MS;

  if (!gps.location.isValid()) {
    return;
  }

  rowCount++;

  uint32_t timeMs = millis();

  double latitude = gps.location.lat();
  double longitude = gps.location.lng();

  snprintf(
    csvLine,
    sizeof(csvLine),
    "%lu,%lu,%.6f,%.6f",
    (unsigned long)timeMs,
    (unsigned long)rowCount,
    latitude,
    longitude);

  csvResult = csvLog.println(csvLine);

  if (!csvResult.ok) {
    stopWithError("CSV write failed.");
  }

  if (rowCount % 50 == 0) {
    Serial.print("CSV row: ");
    Serial.println(csvLine);
  }
}