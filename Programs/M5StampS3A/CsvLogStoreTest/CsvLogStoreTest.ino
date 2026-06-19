#include <Arduino.h>
#include "SdPins.h"
#include "SdSpiStorage.h"
#include "CsvLogStore.h"

SdSpiStorage sd;
CsvLogStore csvLog;

static uint32_t lastLogMs = 0;
static uint32_t rowCount = 0;

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("SPI SD CSV Log Test");

  if (!sd.begin(
        SD_SPI_SCK_PIN,
        SD_SPI_MISO_PIN,
        SD_SPI_MOSI_PIN,
        SD_SPI_CS_PIN,
        25000000)) {
    Serial.println("SD begin failed.");
    while (1) {
      delay(1000);
    }
  }

  sd.printCardInfo(Serial);

  // 既存テスト: sample.txt 作成
  if (sd.exists("/sample.txt")) {
    sd.remove("/sample.txt");
    Serial.println("Removed: /sample.txt");
  }

  if (sd.writeText("/sample.txt", "Hello World.1 2 3\r\n")) {
    Serial.println("Written: /sample.txt");
  } else {
    Serial.println("Write failed: /sample.txt");
  }

  // seek テスト: 位置5を書き換え
  if (sd.overwriteByte("/sample.txt", 5, '_')) {
    Serial.println("Overwrite byte succeeded: /sample.txt");
  } else {
    Serial.println("Overwrite byte failed: /sample.txt");
  }

  // ディレクトリ作成
  if (!sd.exists("/F000")) {
    if (sd.mkdir("/F000")) {
      Serial.println("Created directory: /F000");
    } else {
      Serial.println("Create directory failed: /F000");
    }
  } else {
    Serial.println("Directory already exists: /F000");
  }

  // コピー先削除
  if (sd.exists("/F000/sample.txt")) {
    sd.remove("/F000/sample.txt");
    Serial.println("Removed: /F000/sample.txt");
  }

  // コピー
  if (sd.copy("/sample.txt", "/F000/sample.txt")) {
    Serial.println("Copied: /sample.txt -> /F000/sample.txt");
  } else {
    Serial.println("Copy failed.");
  }

  Serial.println("Read /F000/sample.txt:");
  sd.readTo("/F000/sample.txt", Serial);
  Serial.println();

  // CSVログ開始
  if (!csvLog.begin()) {
    Serial.println("CsvLogStore begin failed.");
    while (1) {
      delay(1000);
    }
  }

  if (!csvLog.openNext()) {
    Serial.println("CSV log open failed.");
    while (1) {
      delay(1000);
    }
  }

  Serial.print("CSV log opened: ");
  Serial.println(csvLog.currentPath());

  csvLog.writeHeader("time_ms,row,value1,value2,message");
  csvLog.writeRow("0,0,123,456,起動");
  csvLog.flush();

  lastLogMs = millis();
}

void loop() {
  uint32_t now = millis();

  if (now - lastLogMs >= 1000) {
    lastLogMs = now;
    rowCount++;

    char line[96];

    long value1 = (long)(rowCount * 10);
    long value2 = (long)(rowCount * 100);

    snprintf(
      line,
      sizeof(line),
      "%lu,%lu,%ld,%ld,%s",
      (unsigned long)now,
      (unsigned long)rowCount,
      value1,
      value2,
      "測定中");

    if (csvLog.writeRow(line)) {
      Serial.print("CSV row: ");
      Serial.println(line);
    } else {
      Serial.println("CSV write failed.");
    }

    csvLog.flush();
  }
}