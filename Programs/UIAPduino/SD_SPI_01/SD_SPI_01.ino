#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#define LED_BUILTIN 2

static const uint8_t PIN_SS = 6;

// delay() を使わないビジーウェイト
// 48MHz で約 500ms（目安）
static void wait() {
  for (volatile uint32_t i = 0; i < 5000000UL; i++);
}

// delay() を使わない点滅
static void blink(uint8_t count) {
  for (uint8_t i = 0; i < count; i++) {
    digitalWrite(LED_BUILTIN, HIGH); wait();
    digitalWrite(LED_BUILTIN, LOW);  wait();
  }
  wait(); wait();  // グループ間の間隔
}

void setup() {
  // PD4(D14) LOW → USB Code43 回避（最優先）
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // LED 動作確認: 3回ゆっくり点滅（delay不使用）
  // これが見えなければ D2 がLEDピンではない or 時計が想定外
  blink(3);

  // ── SPI.begin ────────────────────────────────────────
  SPI.begin();
  blink(1);  // SPI通過 → 1回

  // ── SD.begin ─────────────────────────────────────────
  if (!SD.begin(PIN_SS)) {
    // SD失敗 → 高速点滅ループ
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH); for(volatile uint32_t i=0;i<500000UL;i++);
      digitalWrite(LED_BUILTIN, LOW);  for(volatile uint32_t i=0;i<500000UL;i++);
    }
  }
  blink(2);  // SD通過 → 2回

  // ── ファイル書き込み ──────────────────────────────────
  File f = SD.open("/test.txt", FILE_WRITE);
  if (!f) {
    while (1) {
      digitalWrite(LED_BUILTIN, HIGH); for(volatile uint32_t i=0;i<500000UL;i++);
      digitalWrite(LED_BUILTIN, LOW);  for(volatile uint32_t i=0;i<500000UL;i++);
      for(volatile uint32_t i=0;i<1500000UL;i++);
    }
  }
  f.println("OK");
  f.close();
  blink(3);  // 書き込み完了 → 3回

  // 完了 → 点灯
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {}
