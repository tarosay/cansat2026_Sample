/**
 * UIAPLog.ino — WebHID→SD テスト版
 *
 * ブラウザ (hid-console.html) から送ったテキストを SD に書き込む。
 * UART は未使用。SD 書き込みが確認できたら SD_SPI_03 へ。
 *
 * ボード : HID ProMicro CH32V003 SD+WebHID  (Board Version: V1.4)
 *          FQBN: UIAP_HID:ch32v:CH32V00x_SD:opt=oslto
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WebHID.h>
#include "Hid.h"

/*
SPIとSDメモリカード
A2 CS  - DAT3/CS
8 MOSI - CMD/DI
3V3    - VDD
7 SCK  - CLK
GND    - VSS
9 MISO - DAT0/DO
*/
#define LED_BUILTIN 2
static const uint8_t PIN_SS = 6;  // SD CS = A2 = PC4 = ピン6
static const uint8_t BUF_SIZE = 64;
static const uint32_t IDLE_MS = 200;

static Sd2Card card;
static SdVolume volume;
static SdFile root;
static SdFile logFile;

static uint8_t rxBuf[BUF_SIZE];
static uint8_t rxLen = 0;
static uint32_t lastRxTime = 0;
static bool fileOpen = false;
static char logFilename[10];

static void error_blink(void) {
  while (1) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
  }
}

static void make_logname(char *buf, uint16_t n) {
  buf[0] = '0' + (n / 10000) % 10;
  buf[1] = '0' + (n / 1000) % 10;
  buf[2] = '0' + (n / 100) % 10;
  buf[3] = '0' + (n / 10) % 10;
  buf[4] = '0' + n % 10;
  buf[5] = '.';
  buf[6] = 'T';
  buf[7] = 'X';
  buf[8] = 'T';
  buf[9] = '\0';
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  WebHID.begin();
  pinMode(PIN_SS, OUTPUT);
  digitalWrite(PIN_SS, HIGH);

  delay(10000);
  hid.Clear();

  if (!card.init(SPI_HALF_SPEED, PIN_SS)) {
    hid.Println("SD FAIL");
    error_blink();
  }
  if (!volume.init(card)) {
    hid.Println("EV");
    error_blink();
  }
  if (!root.openRoot(volume)) {
    hid.Println("ER");
    error_blink();
  }

  for (uint16_t i = 0; i < 65535u; i++) {
    make_logname(logFilename, i);
    if (logFile.open(root, logFilename, O_WRITE | O_CREAT | O_EXCL)) break;
  }
  if (logFile.isOpen()) {
    logFile.close();
  } else {
    hid.Println("EF");
    error_blink();
  }
  fileOpen = false;

  hid.Println(logFilename);
  hid.Println("Ready. Send text from browser.");
}

// ── loop: WebHID → SD ─────────────────────────────────────────────────────────
void loop() {
  // ブラウザから受信したデータを rxBuf に積む
  bool got = false;
  {
    uint8_t hbuf[16];
    uint8_t hlen = hid.Recv(hbuf, sizeof(hbuf));
    // HID パケット末尾のパディング 0x00 を除去する
    while (hlen > 0 && hbuf[hlen - 1] == '\0') hlen--;
    if (hlen > 0) {
      for (uint8_t j = 0; j < hlen && rxLen < BUF_SIZE; j++)
        rxBuf[rxLen++] = hbuf[j];
      got = true;
    }
  }

  if (got) {
    lastRxTime = millis();
    if (!fileOpen) {
      if (logFile.open(root, logFilename, O_WRITE | O_CREAT)) {
        logFile.seekSet(logFile.fileSize());
        fileOpen = true;
        digitalWrite(LED_BUILTIN, HIGH);
        hid.Println("Writing...");
      }
    }
  }

  if (!fileOpen) return;

  if (rxLen >= BUF_SIZE) {
    logFile.write(rxBuf, rxLen);
    logFile.sync();
    rxLen = 0;
    return;
  }

  if ((uint32_t)(millis() - lastRxTime) >= IDLE_MS) {
    if (rxLen > 0) {
      logFile.write(rxBuf, rxLen);
      rxLen = 0;
    }
    logFile.sync();
    logFile.close();
    fileOpen = false;
    digitalWrite(LED_BUILTIN, LOW);
    hid.Println("Saved.");
  }
}
