#include <Arduino.h>
#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"

CsvLogStore csvLog;
CsvLogResult csvResult;

static constexpr size_t GPS_READ_BUFFER_SIZE = 256;

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
  delay(1500);

  ledBegin();

  if (!csvLog.begin()) {
    stopWithError("CsvLogStore begin failed.");
  }

  csvLog.setFlushIntervalMs(1000);

  if (!csvLog.openNext()) {
    stopWithError("CSV log open failed.");
  }

  Serial.print("GPS raw log opened: ");
  Serial.println(csvLog.currentPath());
}

void loop() {
  static uint8_t buffer[GPS_READ_BUFFER_SIZE];

  size_t len = 0;

  while (Serial2.available() > 0 && len < sizeof(buffer)) {
    buffer[len++] = (uint8_t)Serial2.read();
  }

  if (len == 0) {
    return;
  }

  Serial.write(buffer, len);

  csvResult = csvLog.write(buffer, len);

  if (!csvResult.ok) {
    stopWithError("SD write failed.");
  }
}