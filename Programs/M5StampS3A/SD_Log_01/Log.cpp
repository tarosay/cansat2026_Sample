#include "Log.h"
#include <SPI.h>
#include <SD.h>
#include <stdarg.h>

// ── コンストラクタ ───────────────────────────────────────────────────────────
Log::Log()
  : _bufLen(0), _lastWriteTime(0), _lastFlushTime(0),
    _dirty(false), _ready(false), _ledCb(nullptr) {
  _filename[0] = '\0';
}

// ── begin ────────────────────────────────────────────────────────────────────
bool Log::begin(uint8_t ssPin, int8_t sckPin, int8_t misoPin, int8_t mosiPin) {
  if (sckPin >= 0 && misoPin >= 0 && mosiPin >= 0) {
    SPI.begin(sckPin, misoPin, mosiPin, ssPin);
  }

  delay(1500);  // SD カード電源安定待ち (SD_SPI_01 と同じ)

  if (!SD.begin(ssPin, SPI, 25000000)) return false;

  // 次の空きファイル番号を探して空ファイルとして確保
  // (再起動時に同じ番号を使わないための "予約" として機能する)
  for (uint32_t i = 0; i <= 99999UL; i++) {
    makeFilename(_filename, i);
    if (!SD.exists(_filename)) {
      File f = SD.open(_filename, FILE_WRITE);
      if (!f) return false;
      f.close();
      _ready = true;
      return true;
    }
  }
  return false;  // 99999 まで埋まっている
}

// ── print / println ──────────────────────────────────────────────────────────
size_t Log::print(const char* str) {
  size_t n = 0;
  while (*str) {
    writeChar(*str++);
    n++;
  }
  return n;
}

size_t Log::println(const char* str) {
  size_t n = print(str);
  writeChar('\r');
  writeChar('\n');
  return n + 2;
}

size_t Log::print(int val) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%d", val);
  return print(buf);
}

size_t Log::println(int val) {
  size_t n = print(val);
  writeChar('\r');
  writeChar('\n');
  return n + 2;
}

size_t Log::print(long val) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%ld", val);
  return print(buf);
}

size_t Log::println(long val) {
  size_t n = print(val);
  writeChar('\r');
  writeChar('\n');
  return n + 2;
}

size_t Log::print(unsigned long val) {
  char buf[12];
  snprintf(buf, sizeof(buf), "%lu", val);
  return print(buf);
}

size_t Log::println(unsigned long val) {
  size_t n = print(val);
  writeChar('\r');
  writeChar('\n');
  return n + 2;
}

size_t Log::print(float val, int digits) {
  char buf[20];
  snprintf(buf, sizeof(buf), "%.*f", digits, val);
  return print(buf);
}

size_t Log::println(float val, int digits) {
  size_t n = print(val, digits);
  writeChar('\r');
  writeChar('\n');
  return n + 2;
}

size_t Log::print(const String& str) {
  return print(str.c_str());
}

size_t Log::println(const String& str) {
  return println(str.c_str());
}

size_t Log::printf(const char* fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  if (n > 0) print(buf);
  return (n > 0) ? (size_t)n : 0;
}

// ── 制御 ────────────────────────────────────────────────────────────────────
void Log::flush() {
  if (_dirty) flushBuffer();
}

void Log::update() {
  if (!_dirty) return;
  uint32_t now = millis();
  // アイドル検出 or 最大フラッシュ間隔超過 → SD へ書き込む
  if ((now - _lastWriteTime) >= LOG_IDLE_MS ||
      (now - _lastFlushTime) >= LOG_MAX_FLUSH_MS) {
    flushBuffer();
  }
}

// ── private ──────────────────────────────────────────────────────────────────
void Log::makeFilename(char* buf, uint32_t n) {
  buf[0] = '/';
  buf[1] = '0' + (n / 10000) % 10;
  buf[2] = '0' + (n /  1000) % 10;
  buf[3] = '0' + (n /   100) % 10;
  buf[4] = '0' + (n /    10) % 10;
  buf[5] = '0' + (n        ) % 10;
  buf[6] = '.'; buf[7] = 'T'; buf[8] = 'X'; buf[9] = 'T'; buf[10] = '\0';
}

void Log::writeChar(char c) {
  if (!_ready) return;
  if (!_dirty && _ledCb) _ledCb(true);  // 書き込み開始 → LED ON
  _buf[_bufLen++] = (uint8_t)c;
  _dirty = true;
  _lastWriteTime = millis();
  if (_bufLen >= LOG_BUF_SIZE) {
    flushBuffer();
  }
}

/**
 * バッファを SD に書き込む
 *
 * open → write → flush → close を1回で完結させることで、
 * 電源断時に FAT ディレクトリエントリ (ファイルサイズ) の破損を防ぐ。
 * SD ライブラリの flush() だけでもデータブロックは書かれるが、
 * close() まで呼ぶことでサイズ情報も確実に更新される。
 */
void Log::flushBuffer() {
  if (_bufLen == 0) {
    _dirty = false;
    return;
  }

  File f = SD.open(_filename, FILE_APPEND);
  if (f) {
    f.write(_buf, _bufLen);
    f.flush();
    f.close();
  }

  _bufLen     = 0;
  _dirty      = false;
  _lastFlushTime = millis();
  if (_ledCb) _ledCb(false);  // 書き込み完了 → LED OFF
}
