/**
 * UIAPLog2.ino — OpenLog 互換ファームウェア
 *
 * 機能:
 *   - SD の CONFIG.TXT からボーレートを読み取る
 *   - UART RX (PD6) から受信したデータを SD に記録
 *
 * CONFIG.TXT 形式 (OpenLog 互換):
 *   9600,26,3,0
 *   先頭のボーレートのみ使用。ファイルがなければ 9600bps。
 *
 * ログファイル: LOG00001.TXT, LOG00002.TXT, ...
 *
 * LED:
 *   高速点滅 = エラー（SD 初期化失敗等）
 *   点灯     = 書き込み中
 *   消灯     = 待機中
 *
 * ボード : HID ProMicro CH32V003  (Board Version: V1.4)
 *          FQBN: UIAP_HID:ch32v:CH32V003:opt=oslto
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "UIAPSerial.h"

#define LED_BUILTIN 2
static const uint8_t  PIN_SS   = 6;
static const uint8_t  BUF_SIZE = 64;
static const uint32_t IDLE_MS  = 200;

static Sd2Card  card;
static SdVolume volume;
static SdFile   root;
static SdFile   logFile;

static uint8_t  rxBuf[BUF_SIZE];
static uint8_t  rxLen      = 0;
static uint32_t lastRxTime = 0;
static bool     fileOpen   = false;
static char     logFilename[13];   // "LOG00001.TXT\0"

// ── CONFIG.TXT 読み込み（なければデフォルト値で新規作成）─────────────────────
static const char CONFIG_DEFAULT[] = "9600,26,3,0\r\n";

static uint32_t load_config(void) {
  SdFile cfg;

  // ファイルが存在しない → デフォルト値で作成して 9600 を返す
  if (!cfg.open(root, "CONFIG.TXT", O_READ)) {
    if (cfg.open(root, "CONFIG.TXT", O_WRITE | O_CREAT)) {
      cfg.write((const uint8_t*)CONFIG_DEFAULT, sizeof(CONFIG_DEFAULT) - 1);
      cfg.close();
    }
    return 9600UL;
  }

  // ファイルが存在する → 先頭のボーレートを読む
  char buf[8];
  uint8_t n = 0;
  int c;
  while (n < (uint8_t)(sizeof(buf) - 1)) {
    c = cfg.read();
    if (c < 0) break;
    if ((char)c == ',' || (char)c == '\r' || (char)c == '\n') break;
    buf[n++] = (char)c;
  }
  cfg.close();
  buf[n] = '\0';

  uint32_t baud = 0;
  for (uint8_t i = 0; i < n; i++) {
    if (buf[i] >= '0' && buf[i] <= '9')
      baud = baud * 10 + (uint32_t)(buf[i] - '0');
  }
  return (baud >= 300UL) ? baud : 9600UL;
}

// ── ユーティリティ ─────────────────────────────────────────────────────────────
static void error_blink(void) {
  while (1) {
    digitalWrite(LED_BUILTIN, HIGH); delay(200);
    digitalWrite(LED_BUILTIN, LOW);  delay(200);
  }
}

static void make_logname(char *buf, uint16_t n) {
  buf[0]='L'; buf[1]='O'; buf[2]='G';
  buf[3] = '0' + (n / 10000) % 10;
  buf[4] = '0' + (n /  1000) % 10;
  buf[5] = '0' + (n /   100) % 10;
  buf[6] = '0' + (n /    10) % 10;
  buf[7] = '0' + (n        ) % 10;
  buf[8]='.'; buf[9]='T'; buf[10]='X'; buf[11]='T'; buf[12]='\0';
}

// ── setup ─────────────────────────────────────────────────────────────────────
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  pinMode(PIN_SS, OUTPUT);
  digitalWrite(PIN_SS, HIGH);

  if (!card.init(SPI_HALF_SPEED, PIN_SS)) error_blink();
  if (!volume.init(card))                 error_blink();
  if (!root.openRoot(volume))             error_blink();

  uint32_t baud = load_config();

  for (uint16_t i = 1; i <= 65534u; i++) {
    make_logname(logFilename, i);
    if (logFile.open(root, logFilename, O_WRITE | O_CREAT | O_EXCL)) break;
  }
  if (logFile.isOpen()) { logFile.close(); }
  else { error_blink(); }
  fileOpen = false;

  uart.begin(baud);
}

// ── loop: UART → SD ───────────────────────────────────────────────────────────
void loop() {
  bool got = false;

  while (uart.available()) {
    if (rxLen < BUF_SIZE) {
      rxBuf[rxLen++] = uart.read();
      got = true;
    } else {
      break;
    }
  }

  if (got) {
    lastRxTime = millis();
    if (!fileOpen) {
      if (logFile.open(root, logFilename, O_WRITE | O_CREAT)) {
        logFile.seekSet(logFile.fileSize());
        fileOpen = true;
        digitalWrite(LED_BUILTIN, HIGH);
      }
    }
  }

  if (!fileOpen) return;

  if (rxLen >= BUF_SIZE) {
    logFile.write(rxBuf, rxLen); logFile.sync(); rxLen = 0;
    return;
  }

  if ((uint32_t)(millis() - lastRxTime) >= IDLE_MS) {
    if (rxLen > 0) { logFile.write(rxBuf, rxLen); rxLen = 0; }
    logFile.sync();
    logFile.close();
    fileOpen = false;
    digitalWrite(LED_BUILTIN, LOW);
  }
}
