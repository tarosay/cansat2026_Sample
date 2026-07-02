#include <Arduino.h>
#include <esp_camera.h>
#include <SD.h>
#include <SPI.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include "camera.h"
//#include <esp_system.h>

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
  delay(500);
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
    return;
  }

  uint32_t width, height;
  String folder;
  uint32_t maxJpegSize = captureFrames(&width, &height, folder);
  Serial.println("キャプチャ終了");

  Serial.print(width);
  Serial.print(" x ");
  Serial.println(height);
  Serial.println(folder);
  Serial.println(maxJpegSize);
}

void loop() {
  for (int i = 0; i < 5; i++) {
    digitalWrite(GPIO_LED, LOW);  //点灯
    delay(500);
    digitalWrite(GPIO_LED, HIGH); //消灯
    delay(500);
  }

  Serial.println("再起動します");
  esp_restart();
}