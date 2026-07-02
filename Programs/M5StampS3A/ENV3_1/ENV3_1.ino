#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT31.h>
#include <M5UnitENV.h>

#include "BoardConfig.h"
#include "StatusLed.h"
#include "CsvLogStore.h"

CsvLogStore csvLog;
CsvLogResult csvResult;

Adafruit_SHT31 sht31(&Wire);
QMP6988 qmp;

static constexpr uint8_t I2C_SDA_PIN = 13;
static constexpr uint8_t I2C_SCL_PIN = 15;
static constexpr uint32_t LOG_INTERVAL_MS = 100;  // 10Hz
static constexpr size_t CSV_ROW_MAX_LENGTH = 128;

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

void setup() {
  Serial.begin(115200);
  delay(1500);

  ledBegin();

  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);

  setupEnv3();

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
    "qmp_temperature_c,pressure_pa,altitude_m");

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

  uint32_t timeMs = millis();

  snprintf(csvLine, sizeof(csvLine), "%lu,%lu,%.2f,%.2f,%.2f,%.2f,%.2f",
           (unsigned long)timeMs, (unsigned long)rowCount, shtTemp, shtHumidity,
           qmp.cTemp, qmp.pressure, qmp.altitude - refAltitude);  // 海抜高度から基準高度を引いて相対高度に

  csvResult = csvLog.println(csvLine);

  if (!csvResult.ok) {
    stopWithError("CSV write failed.");
  }

  if (rowCount % 5 == 0) {
    Serial.print("CSV row: ");
    Serial.println(csvLine);

    //Serial.println(String(shtTemp));
  }
}
