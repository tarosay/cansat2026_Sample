#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

static const uint8_t PIN_SS = 6;

static File logFile;

// sprintf を避けるため手動でファイル名を生成
static void makeFilename(char* buf, uint16_t n) {
  // "LOG00000.TXT"
  buf[0]='L'; buf[1]='O'; buf[2]='G';
  buf[3]='0'+(n/10000)%10;
  buf[4]='0'+(n/1000)%10;
  buf[5]='0'+(n/100)%10;
  buf[6]='0'+(n/10)%10;
  buf[7]='0'+(n)%10;
  buf[8]='.'; buf[9]='T'; buf[10]='X'; buf[11]='T'; buf[12]=0;
}

void setup() {
  SPI.begin();

  if (!SD.begin(PIN_SS)) {
    while (1);  // SD 初期化失敗
  }

  // 既存ファイルと被らない番号を探す
  char filename[13];
  uint16_t i;
  for (i = 0; i < 65535; i++) {
    makeFilename(filename, i);
    if (!SD.exists(filename)) break;
  }

  logFile = SD.open(filename, FILE_WRITE);

  Serial.begin(9600);  // ピン15=TX, 16=RX
}

void loop() {
  while (Serial.available()) {
    logFile.write((uint8_t)Serial.read());
  }
  // 一定間隔でフラッシュ（電源断対策）
  static uint32_t last = 0;
  if (millis() - last > 1000) {
    logFile.flush();
    last = millis();
  }
}