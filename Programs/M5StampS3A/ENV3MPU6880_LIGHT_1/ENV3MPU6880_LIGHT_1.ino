#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <M5UnitENV.h>
#include <I2C_MPU6886.h>

#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"

CsvLogStore csvLog;
CsvLogResult csvResult;

Adafruit_SHT31 sht31(&Wire);
QMP6988 qmp;
I2C_MPU6886 imu(I2C_MPU6886_DEFAULT_ADDRESS, Wire);

static constexpr uint8_t MPU6886_WHO_AM_I_VALUE = 0x19;
static constexpr uint8_t I2C_SDA_PIN = 13;
static constexpr uint8_t I2C_SCL_PIN = 15;
static constexpr uint8_t LIGHT_A_PIN = 5;
static constexpr uint8_t LIGHT_D_PIN = 7;
static constexpr uint32_t LOG_INTERVAL_MS = 50;  // 20Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 208;

static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;
static float groundPressurePa = 101325.0f;

// 地上気圧を基準とした相対高度 [m]（打ち上げ地点 = 0m）
static float calcAltitude(float pressurePa, float tempC, float refPa) {
  return (powf(refPa / pressurePa, 1.0f / 5.257f) - 1.0f) * (tempC + 273.15f) / 0.0065f;
}

static void stopWithError(const char* message) {
  Serial.println(message);
  ledOn(STATUS_LED_COLOR_ERROR_RED);

  while (1) {
    delay(1000);
  }
}

static void setupEnv3() {
  if (!sht31.begin(SHT31_DEFAULT_ADDR)) {
    stopWithError("Failed to initialize SHT31 at 0x44.");
  }

  if (!qmp.begin(&Wire, QMP6988_SLAVE_ADDRESS_L, I2C_SDA_PIN, I2C_SCL_PIN, 400000U)) {
    stopWithError("Failed to initialize QMP6988 at 0x70.");
  }

  Serial.println("ENV III initialized: SHT31 at 0x44, QMP6988 at 0x70.");
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

  pinMode(LIGHT_A_PIN, INPUT);
  pinMode(LIGHT_D_PIN, INPUT);

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  setupEnv3();
  setupImu();

  // 地上気圧を記録（高度基準点 = 0m）
  for (int i = 0; i < 10; i++) {
    qmp.update();
    delay(100);
  }
  groundPressurePa = qmp.pressure;
  Serial.printf("Ground pressure: %.2f Pa\n", groundPressurePa);

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
    "sht_temperature_c,humidity_rh,"
    "qmp_temperature_c,pressure_pa,altitude_m,"
    "accel_x_g,accel_y_g,accel_z_g,"
    "gyro_x_dps,gyro_y_dps,gyro_z_dps,"
    "imu_temperature_c,"
    "light_v,light_d");

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

  float shtTemp, shtHumidity;
  if (!sht31.readBoth(&shtTemp, &shtHumidity)) {
    stopWithError("SHT31 read failed.");
  }

  if (!qmp.update()) {
    stopWithError("QMP6988 update failed.");
  }

  float accelXG = 0.0f, accelYG = 0.0f, accelZG = 0.0f;
  float gyroXDps = 0.0f, gyroYDps = 0.0f, gyroZDps = 0.0f;
  float imuTempC = 0.0f;

  imu.getAccel(&accelXG, &accelYG, &accelZG);
  imu.getGyro(&gyroXDps, &gyroYDps, &gyroZDps);
  imu.getTemp(&imuTempC);

  float lightV = 3.3f * analogRead(LIGHT_A_PIN) / 4095.0f;
  int lightD = digitalRead(LIGHT_D_PIN);

  uint32_t timeMs = millis();

  snprintf(
    csvLine,
    sizeof(csvLine),
    "%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f,%.2f,%d",
    (unsigned long)timeMs,
    (unsigned long)rowCount,
    shtTemp,
    shtHumidity,
    qmp.cTemp,
    qmp.pressure,
    calcAltitude(qmp.pressure, qmp.cTemp, groundPressurePa),
    accelXG, accelYG, accelZG,
    gyroXDps, gyroYDps, gyroZDps,
    imuTempC,
    lightV,
    lightD);

  csvResult = csvLog.println(csvLine);

  if (!csvResult.ok) {
    stopWithError("CSV write failed.");
  }

  if (rowCount % 20 == 0) {
    Serial.print("CSV row: ");
    Serial.println(csvLine);
  }
}
