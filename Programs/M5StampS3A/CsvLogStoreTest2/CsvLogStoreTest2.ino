#include <Arduino.h>
#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"

CsvLogStore csvLog;
CsvLogResult csvResult;

static constexpr uint32_t LOG_INTERVAL_MS = 20;  // 50Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 96;

static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  ledBegin();

  if (!csvLog.begin()) {
    Serial.println("CsvLogStore begin failed.");
    ledOn(STATUS_LED_COLOR_ERROR_RED);
    while (1) {
      delay(1000);
    }
  }

  csvLog.setFlushEveryRows(10);
  csvLog.setFlushIntervalMs(5000);

  if (!csvLog.openNext()) {
    Serial.println("CSV log open failed.");
    ledOn(STATUS_LED_COLOR_ERROR_RED);
    while (1) {
      delay(1000);
    }
  }

  Serial.print("CSV log opened: ");
  Serial.println(csvLog.currentPath());

  csvResult = csvLog.writeHeader("time_ms,row,value1,value2,message");
  if (!csvResult.ok) {
    Serial.println("CSV header write failed.");
    ledOn(STATUS_LED_COLOR_ERROR_RED);
    while (1) {
      delay(1000);
    }
  }

  csvResult = csvLog.writeRow("0,0,123,456,起動");
  if (!csvResult.ok) {
    Serial.println("CSV first row write failed.");
    ledOn(STATUS_LED_COLOR_ERROR_RED);
    while (1) {
      delay(1000);
    }
  }

  if (!csvLog.flush()) {
    Serial.println("CSV initial flush failed.");
    ledOn(STATUS_LED_COLOR_ERROR_RED);
    while (1) {
      delay(1000);
    }
  }

  lastLogMs = millis();
}

void loop() {
  static char csvLine[CSV_ROW_MAX_LENGTH];

  uint32_t now = millis();

  if (now - lastLogMs < LOG_INTERVAL_MS) {
    return;
  }

  lastLogMs += LOG_INTERVAL_MS;
  rowCount++;

  uint32_t timeMs = millis();

  long value1 = (long)(rowCount * 10);
  long value2 = (long)(rowCount * 100);

  snprintf(
    csvLine,
    sizeof(csvLine),
    "%lu,%lu,%ld,%ld,%s",
    (unsigned long)timeMs,
    (unsigned long)rowCount,
    value1,
    value2,
    "測定中");

  csvResult = csvLog.writeRow(csvLine);

  if (!csvResult.ok) {
    Serial.println("CSV write failed.");
    ledOn(STATUS_LED_COLOR_ERROR_RED);
    while (1) {
      delay(1000);
    }
  }

  Serial.print("CSV row: ");
  Serial.println(csvLine);
}