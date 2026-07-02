#include <Arduino.h>
#include <esp_camera.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "camera.h"

// SDカードのチップセレクトピン
#define SD_CS 9
#define SD_MOSI 38
#define SD_CLK 39
#define SD_MISO 40
#define GPIO_0 0
#define GPIO_LED 14

#define BAURATE 115200

void setup() {
  Serial.begin(BAURATE);

  // IOG0を入力に設定し、プルアップ抵抗を有効にする
  pinMode(GPIO_0, INPUT_PULLUP);
  //青色LEDをOUTPUTに設定
  pinMode(GPIO_LED, OUTPUT);

  //LED消去
  digitalWrite(GPIO_LED, HIGH);

  // WiFiの無効化
  WiFi.mode(WIFI_OFF);
  esp_wifi_stop();
  Serial.println("WiFiを無効化しました");

  // SPIピンの設定
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  // SDカードの初期化
  if (!SD.begin(SD_CS)) {
    Serial.println("SDカードのマウントに失敗しました");
  } else {
    Serial.println("SDカードがマウントされました");
  }

  // カメラ初期化
  if (!cameraInit()) {
    Serial.println("カメラ初期化に失敗しました");
  }
  Serial.println("カメラを初期化しました");

  uint32_t width, height;
  // camera_fb_t* fb = esp_camera_fb_get();
  // if (!fb) {
  //   Serial.println("フレームバッファの取得に失敗しました");
  //   return;
  // }
  // Serial.println("フレームバッファを取得しました");
  // width = (uint32_t)fb->width;
  // height = (uint32_t)fb->height;
  // esp_camera_fb_return(fb);
  // Serial.print(width);
  // Serial.print(" x ");
  // Serial.println(height);

  String folder;
  uint32_t maxJpegSize = captureFrames(&width, &height, folder);
  Serial.println("キャプチャ終了");

  Serial.print(width);
  Serial.print(" x ");
  Serial.println(height);
  Serial.println(folder);
  Serial.println(maxJpegSize);

  ////LED点灯
  //digitalWrite(LED_GPIO_NUM, LOW);
}

bool IsCaptuer = true;
void loop() {
  digitalWrite(GPIO_LED, LOW);
  delay(250);
  digitalWrite(GPIO_LED, HIGH);
  delay(250);

  // if (digitalRead(GPIO_0) == HIGH && IsCaptuer == true) {
  //   digitalWrite(GPIO_LED, HIGH);  //LED消灯

  //   uint32_t width, height;
  //   String folder;
  //   uint32_t maxJpegSize = captureFrames(&width, &height, folder);
  //   Serial.println(folder);
  //   Serial.print(width);
  //   Serial.print(" x ");
  //   Serial.println(height);
  //   Serial.println(maxJpegSize);

  //   // キャプチャしたフレームをAVIファイルに結合
  //   CreateAviFromJpegs(folder, width, height, maxJpegSize);
  //   Serial.println("AVIファイル作成終了");

  //   IsCaptuer = false;  //キャプチャフラグをfalseに
  // } else if (digitalRead(GPIO_0) == LOW) {
  //   IsCaptuer = true;  //キャプチャフラグをtrueに
  // }
}