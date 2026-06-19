#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

#define SD_SPI_MOSI_PIN 1
#define SD_SPI_MISO_PIN 3
#define SD_SPI_SCK_PIN 5
#define SD_SPI_CS_PIN 7

void setup() {
  Serial.begin(115200);
  //Serial.setRxBufferSize(512);  //default 128
  delay(1500);
  Serial.println("SPI_SD Test");

  // SD Card Initialization
  SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

  if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
    // Print a message if the SD card initialization
    // fails orif the SD card does not exist.
    // 如果SD卡初始化失败或者SD卡不存在，则打印消息.
    Serial.println("Card failed, or not present");
    while (1)
      ;
  }

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.print("SD Card Size: ");
  Serial.print(cardSize);
  Serial.println("MB");

  // 既存のファイルを削除
  if (SD.exists("/sample.txt")) {
    SD.remove("/sample.txt");
    Serial.println("既存のファイルを削除しました: /sample.txt");
  }

  // Write to the file
  File file = SD.open("/sample.txt", FILE_WRITE);
  if (file) {
    file.println("Hello World.");
    file.close();
    Serial.println("File written successfully.");
  } else {
    Serial.println("Failed to open file for writing.");
  }

  // seekを使ってスペースをアンダースコアに置き換える
  file = SD.open("/sample.txt", "r+");
  if (file) {
    uint32_t size = file.size();
    if (file.seek(5)) {
      file.write('_');  // スペースをアンダースコアに置き換え
      Serial.println("スペースをアンダースコアに置き換えました: /sample.txt");
    } else {
      Serial.println("指定された位置にシークできませんでした: /sample.txt");
    }
    file.seek(size);
    file.close();
  } else {
    Serial.println("ファイルのオープンに失敗しました: /sample.txt");
  }

  // ディレクトリF000を作成
  if (SD.exists("/F000")) {
    Serial.println("ディレクトリは既に存在します: /F000");
  } else {
    if (SD.mkdir("/F000")) {
      Serial.println("ディレクトリを作成しました: /F000");
    } else {
      Serial.println("ディレクトリの作成に失敗しました: /F000");
    }
  }

  // 既存のファイルを削除
  if (SD.exists("/F000/sample.txt")) {
    SD.remove("/F000/sample.txt");
    Serial.println("既存のファイルを削除しました: /F000/sample.txt");
  }

  // sample.txtを/F000/sample.txtにコピー
  File sourceFile = SD.open("/sample.txt");
  File destFile = SD.open("/F000/sample.txt", FILE_WRITE);

  if (sourceFile && destFile) {
    while (sourceFile.available()) {
      destFile.write(sourceFile.read());
    }
    sourceFile.close();
    destFile.close();
    Serial.println("ファイルをコピーしました: /F000/sample.txt");
  } else {
    Serial.println("ファイルのコピーに失敗しました: /F000/sample.txt");
  }
}

void loop() {
  // Read the file from the new location
  File file = SD.open("/F000/sample.txt");
  if (file) {
    Serial.println("Reading from /F000/sample.txt:");
    while (file.available()) {
      Serial.write(file.read());
    }
    file.close();
  } else {
    Serial.println("Failed to open /F000/sample.txt for reading.");
  }

  // Add a delay to avoid flooding the serial output
  delay(5000);  // Read the file every 5 seconds
}
