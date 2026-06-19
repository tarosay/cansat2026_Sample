#include <Arduino.h>
#include <Wire.h>

#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"

CsvLogStore csvLog;
CsvLogResult csvResult;

static constexpr uint8_t MPU6880_ADDRESS_PRIMARY = 0x68;
static constexpr uint8_t MPU6880_ADDRESS_SECONDARY = 0x69;
static constexpr uint8_t MPU6880_WHO_AM_I_VALUE = 0x19;

static constexpr uint8_t MPU6880_REG_SMPLRT_DIV = 0x19;
static constexpr uint8_t MPU6880_REG_CONFIG = 0x1A;
static constexpr uint8_t MPU6880_REG_GYRO_CONFIG = 0x1B;
static constexpr uint8_t MPU6880_REG_ACCEL_CONFIG = 0x1C;
static constexpr uint8_t MPU6880_REG_ACCEL_CONFIG2 = 0x1D;
static constexpr uint8_t MPU6880_REG_ACCEL_XOUT_H = 0x3B;
static constexpr uint8_t MPU6880_REG_PWR_MGMT_1 = 0x6B;
static constexpr uint8_t MPU6880_REG_PWR_MGMT_2 = 0x6C;
static constexpr uint8_t MPU6880_REG_WHO_AM_I = 0x75;

static constexpr float MPU6880_ACCEL_SCALE = 4096.0f;  // +/-8g
static constexpr float MPU6880_GYRO_SCALE = 65.5f;     // +/-500 dps

static constexpr uint32_t LOG_INTERVAL_MS = 20;  // 50Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 160;

static uint8_t mpuAddress = MPU6880_ADDRESS_PRIMARY;
static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;

struct Mpu6880Data {
  float accelXG;
  float accelYG;
  float accelZG;
  float gyroXDps;
  float gyroYDps;
  float gyroZDps;
  float temperatureC;
};

static int16_t readInt16Be(const uint8_t* data) {
  return (int16_t)((data[0] << 8) | data[1]);
}

static void stopWithError(const char* message) {
  Serial.println(message);
  ledOn(STATUS_LED_COLOR_ERROR_RED);

  while (1) {
    delay(1000);
  }
}

static bool writeRegister(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

static bool readRegisters(uint8_t address, uint8_t startReg, uint8_t* buffer, size_t length) {
  if (buffer == nullptr || length == 0 || length > 32) {
    return false;
  }

  Wire.beginTransmission(address);
  Wire.write(startReg);

  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  size_t requested = Wire.requestFrom(address, (uint8_t)length);

  if (requested != length) {
    return false;
  }

  for (size_t i = 0; i < length; i++) {
    if (!Wire.available()) {
      return false;
    }

    buffer[i] = Wire.read();
  }

  return true;
}

static bool readRegister(uint8_t address, uint8_t reg, uint8_t* value) {
  return readRegisters(address, reg, value, 1);
}

static bool findMpu6880() {
  const uint8_t addresses[] = {
    MPU6880_ADDRESS_PRIMARY,
    MPU6880_ADDRESS_SECONDARY,
  };

  for (uint8_t i = 0; i < sizeof(addresses); i++) {
    uint8_t whoAmI = 0;

    if (!readRegister(addresses[i], MPU6880_REG_WHO_AM_I, &whoAmI)) {
      continue;
    }

    Serial.print("I2C device at 0x");
    Serial.print(addresses[i], HEX);
    Serial.print(" WHO_AM_I=0x");
    Serial.println(whoAmI, HEX);

    if (whoAmI == MPU6880_WHO_AM_I_VALUE) {
      mpuAddress = addresses[i];
      return true;
    }
  }

  return false;
}

static void setupMpu6880() {
  if (!findMpu6880()) {
    stopWithError("MPU6880 not found. Check I2C address and wiring.");
  }

  if (!writeRegister(mpuAddress, MPU6880_REG_PWR_MGMT_1, 0x80)) {
    stopWithError("MPU6880 reset failed.");
  }

  delay(100);

  bool ok = true;

  ok = ok && writeRegister(mpuAddress, MPU6880_REG_PWR_MGMT_1, 0x01);     // Auto select clock source
  ok = ok && writeRegister(mpuAddress, MPU6880_REG_PWR_MGMT_2, 0x00);     // Enable accel and gyro
  ok = ok && writeRegister(mpuAddress, MPU6880_REG_SMPLRT_DIV, 0x13);     // 1kHz / (19 + 1) = 50Hz
  ok = ok && writeRegister(mpuAddress, MPU6880_REG_CONFIG, 0x03);         // Gyro DLPF
  ok = ok && writeRegister(mpuAddress, MPU6880_REG_GYRO_CONFIG, 0x08);    // +/-500 dps
  ok = ok && writeRegister(mpuAddress, MPU6880_REG_ACCEL_CONFIG, 0x10);   // +/-8g
  ok = ok && writeRegister(mpuAddress, MPU6880_REG_ACCEL_CONFIG2, 0x03);  // Accel DLPF

  if (!ok) {
    stopWithError("MPU6880 setup failed.");
  }

  Serial.print("MPU6880 initialized at 0x");
  Serial.println(mpuAddress, HEX);
}

static bool readMpu6880(Mpu6880Data* data) {
  if (data == nullptr) {
    return false;
  }

  uint8_t buffer[14];

  if (!readRegisters(mpuAddress, MPU6880_REG_ACCEL_XOUT_H, buffer, sizeof(buffer))) {
    return false;
  }

  int16_t rawAccelX = readInt16Be(&buffer[0]);
  int16_t rawAccelY = readInt16Be(&buffer[2]);
  int16_t rawAccelZ = readInt16Be(&buffer[4]);
  int16_t rawTemp = readInt16Be(&buffer[6]);
  int16_t rawGyroX = readInt16Be(&buffer[8]);
  int16_t rawGyroY = readInt16Be(&buffer[10]);
  int16_t rawGyroZ = readInt16Be(&buffer[12]);

  data->accelXG = rawAccelX / MPU6880_ACCEL_SCALE;
  data->accelYG = rawAccelY / MPU6880_ACCEL_SCALE;
  data->accelZG = rawAccelZ / MPU6880_ACCEL_SCALE;
  data->gyroXDps = rawGyroX / MPU6880_GYRO_SCALE;
  data->gyroYDps = rawGyroY / MPU6880_GYRO_SCALE;
  data->gyroZDps = rawGyroZ / MPU6880_GYRO_SCALE;
  data->temperatureC = rawTemp / 326.8f + 25.0f;

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  ledBegin();

  Wire.begin();
  Wire.setClock(400000);

  setupMpu6880();

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
  Mpu6880Data data;

  if (!readMpu6880(&data)) {
    stopWithError("MPU6880 read failed.");
  }

  snprintf(
    csvLine,
    sizeof(csvLine),
    "%lu,%lu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f",
    (unsigned long)timeMs,
    (unsigned long)rowCount,
    data.accelXG,
    data.accelYG,
    data.accelZG,
    data.gyroXDps,
    data.gyroYDps,
    data.gyroZDps,
    data.temperatureC);

  csvResult = csvLog.println(csvLine);

  if (!csvResult.ok) {
    stopWithError("CSV write failed.");
  }

  if (rowCount % 50 == 0) {
    Serial.print("CSV row: ");
    Serial.println(csvLine);
  }
}
