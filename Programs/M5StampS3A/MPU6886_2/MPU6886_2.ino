#include <Arduino.h>
#include <Wire.h>
#include <I2C_MPU6886.h>

#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"

CsvLogStore csvLog;
CsvLogResult csvResult;

static constexpr uint8_t MPU6886_I2C_ADDRESS = I2C_MPU6886_DEFAULT_ADDRESS;
static constexpr uint8_t MPU6886_WHO_AM_I_VALUE = 0x19;
static constexpr uint8_t I2C_SDA_PIN = 13;
static constexpr uint8_t I2C_SCL_PIN = 15;
static constexpr uint32_t LOG_INTERVAL_MS = 20;  // 50Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 160;

I2C_MPU6886 imu(MPU6886_I2C_ADDRESS, Wire);

static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;

static void stopWithError(const char* message) {
  Serial.println(message);
  ledOn(STATUS_LED_COLOR_ERROR_RED);

  while (1) {
    delay(1000);
  }
}

static void setupImu() {
  imu.begin();

  uint8_t whoAmI = imu.whoAmI();

  Serial.print("MPU6886 whoAmI() = 0x");
  Serial.println(whoAmI, HEX);

  if (whoAmI != MPU6886_WHO_AM_I_VALUE) {
    stopWithError("MPU6886 not found. Check I2C address and wiring.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  ledBegin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  setupImu();

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

  csvResult = csvLog.writeHeader(
    "time_ms,row,"
    "accel_x_g,accel_y_g,accel_z_g,"
    "gyro_x_dps,gyro_y_dps,gyro_z_dps,"
    "temperature_c");

  if (!csvResult.ok) {
    stopWithError("CSV header write failed.");
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
  float accelXG = 0.0f;
  float accelYG = 0.0f;
  float accelZG = 0.0f;
  float gyroXDps = 0.0f;
  float gyroYDps = 0.0f;
  float gyroZDps = 0.0f;
  float temperatureC = 0.0f;

  imu.getAccel(&accelXG, &accelYG, &accelZG);
  imu.getGyro(&gyroXDps, &gyroYDps, &gyroZDps);
  imu.getTemp(&temperatureC);

  snprintf(
    csvLine,
    sizeof(csvLine),
    "%lu,%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f",
    (unsigned long)timeMs,
    (unsigned long)rowCount,
    accelXG,
    accelYG,
    accelZG,
    gyroXDps,
    gyroYDps,
    gyroZDps,
    temperatureC);

  csvResult = csvLog.println(csvLine);

  if (!csvResult.ok) {
    stopWithError("CSV write failed.");
  }

  if (rowCount % 50 == 0) {
    Serial.print("CSV row: ");
    Serial.println(csvLine);
  }
}
