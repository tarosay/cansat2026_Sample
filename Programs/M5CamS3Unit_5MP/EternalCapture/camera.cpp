#include <Arduino.h>
#include <esp_camera.h>
#include <SD.h>
#include <SPI.h>
#include <esp_system.h>

#define CAMERA_MODEL_M5STACK_CAMS3_UNIT  // Has PSRAM
#include "camera_pins.h"
#include "camera.h"
#define CAMERA_MODEL_M5STACK_CAMS3_UNIT  // Has PSRAM

bool cameraInit() {
  // カメラ初期化
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  //config.frame_size = FRAMESIZE_XGA;

  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
#ifdef CAMERA_MODEL_M5STACK_CAMS3_UNIT
      config.fb_count = 1;
#else
      config.fb_count = 2;
#endif
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  Serial.println("カメラの初期化をします");
  while (true) {
    if (esp_camera_init(&config) != ESP_OK) {
      Serial.println("カメラの初期化に失敗しました");
    } else {
      Serial.println("カメラが初期化されました");
      break;
    }
    delay(1000);
  }

  // カメラセンサーの取得
  Serial.println("カメラセンサーを取得します");
  sensor_t* s = esp_camera_sensor_get();
  if (s == NULL) {
    Serial.println("センサーの取得に失敗しました");
    return false;
  }

  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    Serial.println("OV3660_PIDです");
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }

  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    Serial.println("PIXFORMAT_JPEGです");
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  Serial.println("CAMERA_MODEL_M5STACK_WIDE || CAMERA_MODEL_M5STACK_ESP32CAM です");
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
  Serial.println("CAMERA_MODEL_ESP32S3_EYE です");
  s->set_vflip(s, 1);
#endif

  // ホワイトバランスの設定をすると暴走します
  //s->set_whitebal(s, 1);  // 0 = Off, 1 = On
  //自動ホワイトバランスゲインの設定をすると暴走します
  //s->set_awb_gain(s, 1);  // 自動ホワイトバランスゲインを有効にする

  Serial.println("ホワイトバランスのモード設定");
  s->set_wb_mode(s, 0);  // 0	Auto	自動ホワイトバランス, 1	Sunny	晴天, 2	Cloudy	曇り, 3	Office	オフィス照明（蛍光灯）, 4	Home	家庭照明（白熱灯）

  //s->set_vflip(s, 0);  // 0 = 正常, 1 = 上下反転
  //s->set_hmirror(s, 0);  // 0 = 正常, 1 = 左右反転

  // 他の設定例
  // s->set_brightness(s, 1);  // -2 to 2
  // s->set_contrast(s, 1);    // -2 to 2
  // s->set_saturation(s, 1);  // -2 to 2

  //4枚を空撮影、ホワイトバランスを安定されるため
  Serial.println("4枚を空撮影、ホワイトバランスを安定されるため");
  for (int i = 0; i < 4; i++) {
    // カメラから画像を取得
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("画像の取得に失敗しました");
      delay(1000);
      continue;
    }
    esp_camera_fb_return(fb);  // フレームバッファを解放
    delay(100);
  }
  return true;
}

uint32_t getMaxFolderNumber() {
  uint32_t maxNumber = 0;

  File root = SD.open("/");
  if (!root) {
    Serial.println("ルートディレクトリのオープンに失敗しました");
    return maxNumber;
  }

  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      // これ以上のファイルまたはフォルダがない場合
      break;
    }

    if (entry.isDirectory()) {
      String folderName = entry.name();
      bool isNumeric = true;
      for (int i = 0; i < folderName.length(); i++) {
        if (!isDigit(folderName[i])) {
          isNumeric = false;
          break;
        }
      }

      if (isNumeric) {
        uint32_t folderNumber = folderName.toInt();
        if (folderNumber > maxNumber) {
          maxNumber = folderNumber;
        }
      }
    }

    entry.close();
  }

  root.close();
  return maxNumber;
}

uint32_t captureFrames(uint32_t* width, uint32_t* height, String& folder) {
  return captureFrames(FRAME_COUNT, FPS, width, height, folder);
}
uint32_t captureFrames(uint32_t frameCount, uint32_t fps, uint32_t* width, uint32_t* height, String& folder) {
  uint32_t maxsize = 0;

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("フレームバッファの取得に失敗しました");
    return maxsize;
  }
  *width = (uint32_t)fb->width;
  *height = (uint32_t)fb->height;
  esp_camera_fb_return(fb);

  //フォルダ番号の最大値を求める
  Serial.println("フォルダ番号最大値検索");
  uint32_t foldernum = getMaxFolderNumber() + 1;
  //フォルダの作成
  char foldername[8];
  snprintf(foldername, 8, "/%06lu", foldernum);
  if (!SD.mkdir(foldername)) {
    Serial.println("フォルダの作成に失敗しました");
    return maxsize;
  }

  folder = String(foldername);

  Serial.println("キャプチャ開始");
  unsigned long nextPhotoCapture = millis();
  uint32_t delayMs = 1000 / fps;  // フレームごとの遅延
  uint32_t capturedFrames = 0;
  while (capturedFrames < frameCount) {
    // 次回の写真撮影時間が経過したかを確認
    if (millis() >= nextPhotoCapture) {
      // 次回の写真撮影時間を設定
      nextPhotoCapture = millis() + delayMs;

      // カメラから画像を取得
      fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("フレームバッファの取得に失敗しました");
        continue;
      }
      digitalWrite(LED_GPIO_NUM, LOW);  //点灯


      // フレームを個別ファイルに保存
      String fileName = folder + "/frame" + String(capturedFrames) + ".jpg";
      File file = SD.open(fileName.c_str(), FILE_WRITE);
      if (!file) {
        Serial.println("ファイルの作成に失敗しました");
        esp_camera_fb_return(fb);
        continue;
      }

      file.write(fb->buf, fb->len);
      file.close();

      maxsize = maxsize < fb->len ? fb->len : maxsize;
      esp_camera_fb_return(fb);

      //撮影枚数が、再起動カウントになったら、自分を再起動する
      if (capturedFrames + 1 == MY_REBOOT) {
        int rebootCount = MY_REBOOT;
        if (rebootCount > 0) {
          esp_restart();
        }
      }
      capturedFrames++;
      digitalWrite(LED_GPIO_NUM, HIGH);  //消灯
    }
  }
  return maxsize;
}
