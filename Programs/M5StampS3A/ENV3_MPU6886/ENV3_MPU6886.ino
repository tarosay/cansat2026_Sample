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

// ==== 測定レンジ設定（学生が変更する箇所） ====
// Claude Fable 5（Anthropic の AI）が作成（2026-07-03）
//
// 加速度の最大測定レンジ [G]。2, 4, 8, 16 のどれかを指定する。
// レンジを小さくすると細かい振動まで良く見えるが、それを超えるGは振り切れて記録できない。
// モデルロケットの発射Gを測るなら 8 か 16 が安全。
static constexpr uint16_t ACCEL_MAX_G = 8;

// ジャイロ（角速度）の最大測定レンジ [度/秒]。250, 500, 1000, 2000 のどれかを指定する。
static constexpr uint16_t GYRO_MAX_DPS = 2000;

// 指定できない値をコンパイル時にエラーにする（実行前に間違いに気づけるようにする）
static_assert(ACCEL_MAX_G == 2 || ACCEL_MAX_G == 4 || ACCEL_MAX_G == 8 || ACCEL_MAX_G == 16,
              "ACCEL_MAX_G は 2, 4, 8, 16 のどれかにしてください");
static_assert(GYRO_MAX_DPS == 250 || GYRO_MAX_DPS == 500 || GYRO_MAX_DPS == 1000 || GYRO_MAX_DPS == 2000,
              "GYRO_MAX_DPS は 250, 500, 1000, 2000 のどれかにしてください");

// レンジ設定をレジスタに書く値（FS_SEL）に変換する。
// MPU6886 のデータシートで ACCEL_CONFIG / GYRO_CONFIG レジスタのビット4:3に入れる値と決まっている。
static constexpr uint8_t ACCEL_FS_SEL =
  (ACCEL_MAX_G == 2) ? 0 : (ACCEL_MAX_G == 4) ? 1
                         : (ACCEL_MAX_G == 8) ? 2
                                              : 3;
static constexpr uint8_t GYRO_FS_SEL =
  (GYRO_MAX_DPS == 250) ? 0 : (GYRO_MAX_DPS == 500)  ? 1
                            : (GYRO_MAX_DPS == 1000) ? 2
                                                     : 3;

// I2C_MPU6886 ライブラリの getAccel()/getGyro() は「±8G・±2000度/秒」前提の
// 固定係数で換算して返してくるため、実際に設定したレンジとの比で補正する。
// （例: ±16G設定なら値を2倍する。±8G・±2000度/秒のままなら補正係数は1.0で何もしない）
static constexpr float ACCEL_SCALE_CORRECTION = ACCEL_MAX_G / 8.0f;
static constexpr float GYRO_SCALE_CORRECTION = GYRO_MAX_DPS / 2000.0f;

// MPU6886 のレジスタ番地
static constexpr uint8_t MPU6886_REG_GYRO_CONFIG = 0x1B;
static constexpr uint8_t MPU6886_REG_ACCEL_CONFIG = 0x1C;
// ==== 測定レンジ設定ここまで ====

static constexpr uint8_t I2C_SDA_PIN = 13;
static constexpr uint8_t I2C_SCL_PIN = 15;
static constexpr uint32_t LOG_INTERVAL_MS = 50;  // 20Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 192;

static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;
static float refAltitude = 0.0f;  // 起動時（地上）の海抜高度 [m]。相対高度の基準点

// 【参考】地上気圧 refPa を基準とした相対高度 [m] を直接計算する式。
// 現在はライブラリの標準メソッド（qmp.altitude）を使うため未使用ですが、
// 高度計算の式を学ぶための参考として残しています。
// qmp.altitude も同じ形の式（ただし基準気圧は 101325 Pa 固定）で計算されています。
// ※標準メソッド化の改修は Claude Fable 5（Anthropic の AI）が実施（2026-07-02）
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

// MPU6886 のレジスタに1バイト書き込む
// Claude Fable 5（Anthropic の AI）が作成（2026-07-03）
static void writeImuRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(I2C_MPU6886_DEFAULT_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
  delay(1);
}

static void setupImu() {
  imu.begin();

  uint8_t whoAmI = imu.whoAmI();

  Serial.print("MPU6886 whoAmI() = 0x");
  Serial.println(whoAmI, HEX);

  if (whoAmI != MPU6886_WHO_AM_I_VALUE) {
    stopWithError("MPU6886 not found. Check I2C address and wiring.");
  }

  // imu.begin() の中で加速度±8G・ジャイロ±2000度/秒に初期化されるので、
  // 冒頭の ACCEL_MAX_G / GYRO_MAX_DPS で選んだレンジに上書きする。
  // FS_SEL の値はレジスタのビット4:3に入れるので3ビット左にずらす。
  writeImuRegister(MPU6886_REG_ACCEL_CONFIG, ACCEL_FS_SEL << 3);
  writeImuRegister(MPU6886_REG_GYRO_CONFIG, GYRO_FS_SEL << 3);

  Serial.print("Accel range: +/-");
  Serial.print(ACCEL_MAX_G);
  Serial.println("G");
  Serial.print("Gyro range: +/-");
  Serial.print(GYRO_MAX_DPS);
  Serial.println("dps");
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  ledBegin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  setupEnv3();
  setupImu();

  // 起動時の海抜高度を基準点として記録（打ち上げ地点 = 0m）
  // qmp.altitude は update() のたびにライブラリが計算する海抜高度 [m]
  for (int i = 0; i < 10; i++) {
    qmp.update();
    delay(100);
  }
  refAltitude = qmp.altitude;
  Serial.printf("Reference altitude: %.2f m\n", refAltitude);

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
    "imu_temperature_c");

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

  // ライブラリは ±8G・±2000度/秒 前提で換算するので、設定したレンジに合わせて補正する
  accelXG *= ACCEL_SCALE_CORRECTION;
  accelYG *= ACCEL_SCALE_CORRECTION;
  accelZG *= ACCEL_SCALE_CORRECTION;
  gyroXDps *= GYRO_SCALE_CORRECTION;
  gyroYDps *= GYRO_SCALE_CORRECTION;
  gyroZDps *= GYRO_SCALE_CORRECTION;

  uint32_t timeMs = millis();

  snprintf(
    csvLine,
    sizeof(csvLine),
    "%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.2f",
    (unsigned long)timeMs,
    (unsigned long)rowCount,
    shtTemp,
    shtHumidity,
    qmp.cTemp,
    qmp.pressure,
    qmp.altitude - refAltitude,  // 海抜高度から基準高度を引いて相対高度に
    accelXG, accelYG, accelZG,
    gyroXDps, gyroYDps, gyroZDps,
    imuTempC);

  csvResult = csvLog.println(csvLine);

  if (!csvResult.ok) {
    stopWithError("CSV write failed.");
  }

  if (rowCount % 20 == 0) {
    Serial.print("CSV row: ");
    Serial.println(csvLine);
  }
}
