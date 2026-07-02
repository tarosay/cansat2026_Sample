#include <Arduino.h>
#include <esp_camera.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>

#include "mic.h"

#define GPIO_0_NUM 0
#define BAURATE 115200

void setup() {
  Serial.begin(BAURATE);

  // IOG0を入力に設定し、プルアップ抵抗を有効にする
  pinMode(GPIO_0_NUM, INPUT_PULLUP);
  //青色LEDをOUTPUTに設定
  pinMode(LED_GPIO_NUM, OUTPUT);

  //LED消去
  digitalWrite(LED_GPIO_NUM, HIGH);

  // WiFiの無効化
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  Serial.println("WiFiを無効化しました");

  // SPIピンの設定
  SPI.begin(SD_CLK_GPIO_NUM, SD_MISO_GPIO_NUM, SD_MOSI_GPIO_NUM, SD_CS_GPIO_NUM);
  // SDカードの初期化
  if (!SD.begin(SD_CS_GPIO_NUM)) {
    Serial.println("SDカードのマウントに失敗しました");
    while (1) {
      digitalWrite(LED_GPIO_NUM, LOW);
      delay(150);
      digitalWrite(LED_GPIO_NUM, HIGH);
      delay(150);
    }
  }
  Serial.println("SDカードがマウントされました");

  // マイク初期化
  if (!micInit()) {
    Serial.println("マイクの初期化に失敗しました");
    while (1) {
      digitalWrite(LED_GPIO_NUM, LOW);
      delay(150);
      digitalWrite(LED_GPIO_NUM, HIGH);
      delay(150);
    }
  }
  Serial.println(F("マイク初期化完了、録音を開始します"));

  // ===== AGC（突発・大音量用の推奨設定） =====
  AgcConfig agc = getDefaultAgc();
  agc.targetPeakDbFS = -9.0f;  // しっかりヘッドルーム
  agc.minGainDb = -18.0f;      // 下げを許可
  agc.maxGainDb = +12.0f;      // 上げ過ぎ抑止
  agc.attackMs = 10.0f;        // 立ち上がり素早く抑制
  agc.releaseMs = 1300.0f;     // ゆっくり戻す
  agc.noiseGateDbFS = -60.0f;
  agc.gateReleaseMs = 1200.0f;
  setDefaultAgc(agc);

  // // ===== AGC（机上テスト用の設定） =====
  // AgcConfig agc = getDefaultAgc();
  // agc.targetPeakDbFS = -3.0f;  // 出力目標を上げる（ヘッドルーム少なめ）
  // agc.minGainDb = 0.0f;        // 下げを禁止
  // agc.maxGainDb = +30.0f;      // 上げを許可（テスト用に大きく）
  // agc.attackMs = 10.0f;
  // agc.releaseMs = 1000.0f;
  // agc.noiseGateDbFS = -80.0f;  // ゲートをほぼ無効に
  // setDefaultAgc(agc);

  // ===== セッション（噴射の頭を残す / 反応速く） =====
  SessionConfig s = getDefaultSession();
  s.sampleRate = 16000;
  s.blockSamples = 256;
  s.dropHeadMs = 0;  // ★超重要：起動直後の捨てを無効化
  // s.dir はデフォルト "/audio"。装置A/Bで分けたいなら "/audioA", "/audioB" に変更
  // s.dir = "/audio";
  setDefaultSession(s);

  RecResult r = recordingAutoSegmented(0, 37, &s, &agc);
}

void loop() {
  digitalWrite(LED_GPIO_NUM, LOW);
  delay(350);
  digitalWrite(LED_GPIO_NUM, HIGH);
  delay(350);
}
