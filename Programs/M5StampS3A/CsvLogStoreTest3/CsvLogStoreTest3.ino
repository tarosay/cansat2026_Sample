#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <SparkFun_BMI270_Arduino_Library.h>

#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"

#define SEALEVELPRESSURE_HPA 1009.0

CsvLogStore csvLog;
CsvLogResult csvResult;

Adafruit_BMP280 bmp;
BMI270 imu;

static constexpr uint32_t LOG_INTERVAL_MS = 20;  // 50Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 192;

static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;

static uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR;  // 0x68

static float calculateAltitude(float pressurePa, float temperatureC) {
  float T = temperatureC + 273.15f;
  float P0 = SEALEVELPRESSURE_HPA * 100.0f;
  float L = 0.0065f;
  float R = 8.3144598f;
  float g = 9.80665f;
  float M = 0.0289644f;

  return (T / L) * (pow((P0 / pressurePa), (R * L) / (g * M)) - 1.0f);
}

static void stopWithError(const char* message) {
  Serial.println(message);
  ledOn(STATUS_LED_COLOR_ERROR_RED);

  while (1) {
    delay(1000);
  }
}

static void setupImu() {
  while (imu.beginI2C(i2cAddress) != BMI2_OK) {
    Serial.println("Error: BMI270 not connected, check wiring and I2C address!");
    delay(1000);
  }

  int8_t err = BMI2_OK;

  bmi2_sens_config accelConfig;
  accelConfig.type = BMI2_ACCEL;
  accelConfig.cfg.acc.odr = BMI2_ACC_ODR_50HZ;
  accelConfig.cfg.acc.bwp = BMI2_ACC_OSR4_AVG1;
  accelConfig.cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;
  accelConfig.cfg.acc.range = BMI2_ACC_RANGE_8G;
  err = imu.setConfig(accelConfig);

  bmi2_sens_config gyroConfig;
  gyroConfig.type = BMI2_GYRO;
  gyroConfig.cfg.gyr.odr = BMI2_GYR_ODR_50HZ;
  gyroConfig.cfg.gyr.bwp = BMI2_GYR_OSR4_MODE;
  gyroConfig.cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;
  gyroConfig.cfg.gyr.ois_range = BMI2_GYR_OIS_250;
  gyroConfig.cfg.gyr.range = BMI2_GYR_RANGE_125;
  gyroConfig.cfg.gyr.noise_perf = BMI2_PERF_OPT_MODE;
  err = imu.setConfig(gyroConfig);

  while (err != BMI2_OK) {
    if (err == BMI2_E_ACC_INVALID_CFG) {
      Serial.println("Accelerometer config not valid!");
    } else if (err == BMI2_E_GYRO_INVALID_CFG) {
      Serial.println("Gyroscope config not valid!");
    } else if (err == BMI2_E_ACC_GYR_INVALID_CFG) {
      Serial.println("Both configs not valid!");
    } else {
      Serial.print("Unknown BMI270 config error: ");
      Serial.println(err);
    }

    ledOn(STATUS_LED_COLOR_ERROR_RED);
    delay(1000);
  }
}

static void setupBmp280() {
  if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
    stopWithError("Failed to initialize BMP280!");
  }

  bmp.setSampling(
    Adafruit_BMP280::MODE_NORMAL,
    Adafruit_BMP280::SAMPLING_X2,
    Adafruit_BMP280::SAMPLING_X16,
    Adafruit_BMP280::FILTER_X16,
    Adafruit_BMP280::STANDBY_MS_500);
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  ledBegin();

  Wire.begin();

  setupImu();
  setupBmp280();

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
    "accel_x,accel_y,accel_z,"
    "gyro_x,gyro_y,gyro_z,"
    "temperature_c,pressure_pa,altitude_m");

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

  imu.getSensorData();

  float temperatureC = bmp.readTemperature();
  float pressurePa = bmp.readPressure();
  float altitudeM = calculateAltitude(pressurePa, temperatureC);

  snprintf(
    csvLine,
    sizeof(csvLine),
    "%lu,%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f,%.2f,%.2f",
    (unsigned long)timeMs,
    (unsigned long)rowCount,
    imu.data.accelX,
    imu.data.accelY,
    imu.data.accelZ,
    imu.data.gyroX,
    imu.data.gyroY,
    imu.data.gyroZ,
    temperatureC,
    pressurePa,
    altitudeM);

  csvResult = csvLog.println(csvLine);

  if (!csvResult.ok) {
    stopWithError("CSV write failed.");
  }

  if (rowCount % 50 == 0) {
    Serial.print("CSV row: ");
    Serial.println(csvLine);
  }
}