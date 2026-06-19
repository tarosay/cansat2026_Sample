/**
 * SD_Log_01.ino — M5StampS3A SD ロガー
 *
 * 機能:
 *   - 電源オンで XXXXX.TXT (00000.TXT からインクリメント) を新規作成
 *   - Log クラスで log.println() / log.printf() 等により SD へ書き込む
 *   - 電源断に強い設計: open → write → flush → close の 1 サイクル完結
 *   - SD 書き込み中は WS2812 LED を青点灯 (OpenLog 互換)
 *
 * 電源断対策の概要:
 *   データは RAM バッファに蓄積し、LOG_IDLE_MS 間書き込みがないとき
 *   (または flush() を明示呼び出しとき) に SD へ書く。
 *   書き込みは open → write → flush → close で完結するため、
 *   どのタイミングで電源断してもファイルシステムが壊れにくい。
 *
 * ボード設定 (Arduino IDE):
 *   Board : M5StampS3A (または ESP32S3 Dev Module)
 * ライブラリ:
 *   Adafruit NeoPixel (Library Manager からインストール)
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "Log.h"

// ── SPI / SD ピン設定 ─────────────────────────────────────────────────────
#define PIN_SD_MOSI  2
#define PIN_SD_MISO  4
#define PIN_SD_SCK   6
#define PIN_SD_CS    8
// ────────────────────────────────────────────────────────────────────────────

// ── WS2812 LED 設定 ──────────────────────────────────────────────────────────
// M5Stamp S3A: データ=GPIO21, 電源イネーブル=GPIO38 (HIGH で点灯可能)
// M5Stamp S3 にはイネーブルピンがないため GPIO38 の操作は不要
#define PIN_LED      21
#define PIN_LED_EN   38   // S3A 固有の LED 電源イネーブルピン
#define LED_COUNT    1
// ────────────────────────────────────────────────────────────────────────────

static Adafruit_NeoPixel neopixel(LED_COUNT, PIN_LED, NEO_GRB + NEO_KHZ800);

// SD 書き込み中: 青点灯 / 完了: 消灯
static void onSdLed(bool on) {
  neopixel.setPixelColor(0, on ? neopixel.Color(0, 0, 128) : 0);
  neopixel.show();
}

Log sdLog;

// ── setup ────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(PIN_LED_EN, OUTPUT);
  digitalWrite(PIN_LED_EN, HIGH);  // S3A: LED 電源イネーブル

  neopixel.begin();
  neopixel.setBrightness(255);
  neopixel.clear();
  neopixel.show();

  Serial.begin(115200);

  Serial.println("SD Log init ...");

  sdLog.setLedCallback(onSdLed);

  if (!sdLog.begin(PIN_SD_CS, PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI)) {
    // エラー: 赤点滅
    while (true) {
      neopixel.setPixelColor(0, neopixel.Color(128, 0, 0));
      neopixel.show();
      delay(200);
      neopixel.clear();
      neopixel.show();
      delay(200);
    }
  }

  Serial.printf("Log file: %s\n", sdLog.filename());

  // ヘッダー行を書いてすぐにフラッシュ
  // (起動直後の電源断でもヘッダーだけは残る)
  sdLog.println("time_ms,sensor1,sensor2");
  sdLog.flush();

  Serial.println("Logging started.");
}

// ── loop ─────────────────────────────────────────────────────────────────────
void loop() {
  static uint32_t prevMs = 0;
  const uint32_t INTERVAL_MS = 250;   // ログ間隔 (ms) = 4Hz

  uint32_t now = millis();
  if (now - prevMs >= INTERVAL_MS) {
    prevMs = now;

    // ── ここにセンサ読み取りを書く ──────────────────────────────────
    float sensor1 = 25.0f;     // 例: 温度 (°C)
    float sensor2 = 1013.25f;  // 例: 気圧 (hPa)
    // ────────────────────────────────────────────────────────────────

    // CSV 形式で 1 行書き込む
    sdLog.printf("%lu,%.2f,%.2f\r\n", now, sensor1, sensor2);

    // シリアルにも出力（デバッグ用）
    Serial.printf("%lu,%.2f,%.2f\n", now, sensor1, sensor2);
  }

  // アイドル検出 → バッファを SD へ書く (電源断対策の中心)
  // LOG_IDLE_MS (500ms) 間書き込みがなければ flush + close
  sdLog.update();
}
