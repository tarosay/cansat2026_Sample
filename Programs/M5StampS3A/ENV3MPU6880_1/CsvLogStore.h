#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "BoardConfig.h"
#include "StatusLed.h"

struct CsvLogResult {
  bool ok;
  bool flushed;
};

class CsvLogStore {
public:
  static constexpr uint16_t LOG_START_NO = 1;
  static constexpr uint16_t LOG_MAX_NO = 9999;

  CsvLogStore()
    : _opened(false),
      _begun(false),
      _flushEveryRows(0),
      _flushIntervalMs(0),
      _rowsSinceFlush(0),
      _lastFlushMs(0) {
    _currentPath[0] = '\0';
  }

  bool begin() {
    _opened = false;
    _currentPath[0] = '\0';

    SPI.begin(
      SD_SPI_SCK_PIN,
      SD_SPI_MISO_PIN,
      SD_SPI_MOSI_PIN,
      SD_SPI_CS_PIN);

    if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQ)) {
      _begun = false;
      return false;
    }

    if (SD.cardType() == CARD_NONE) {
      _begun = false;
      return false;
    }

    _begun = true;
    return true;
  }

  void setFlushEveryRows(uint16_t rows) {
    _flushEveryRows = rows;
  }

  void setFlushIntervalMs(uint32_t intervalMs) {
    _flushIntervalMs = intervalMs;
  }

  bool openNext() {
    if (!_begun) {
      return false;
    }

    if (_opened) {
      close();
    }

    char path[16];

    if (!findNextPath(path, sizeof(path))) {
      return false;
    }

    _file = SD.open(path, FILE_WRITE);
    if (!_file) {
      _opened = false;
      _currentPath[0] = '\0';
      return false;
    }

    copyPath(_currentPath, sizeof(_currentPath), path);

    if (!writeBomIfEmpty()) {
      _file.close();
      _opened = false;
      _currentPath[0] = '\0';
      return false;
    }

    _opened = true;
    _rowsSinceFlush = 0;
    _lastFlushMs = millis();

    return true;
  }

  bool close() {
    if (!_opened) {
      return true;
    }

    flush();
    _file.close();

    _opened = false;
    _rowsSinceFlush = 0;

    return true;
  }

  bool flush() {
    if (!_opened) {
      return false;
    }

    ledOn(STATUS_LED_COLOR_WRITE_GREEN);
    _file.flush();
    ledOff();

    _rowsSinceFlush = 0;
    _lastFlushMs = millis();

    return true;
  }

  CsvLogResult print(const char* text) {
    CsvLogResult result = { false, false };

    if (!_opened || text == nullptr) {
      return result;
    }

    size_t len = strlen(text);

    if (_file.write((const uint8_t*)text, len) != len) {
      return result;
    }

    result.ok = true;
    result.flushed = flushIfNeeded(false);

    return result;
  }

  CsvLogResult println(const char* text) {
    CsvLogResult result = { false, false };

    if (!_opened || text == nullptr) {
      return result;
    }

    size_t len = strlen(text);

    if (_file.write((const uint8_t*)text, len) != len) {
      return result;
    }

    if (_file.write((const uint8_t*)"\r\n", 2) != 2) {
      return result;
    }

    result.ok = true;
    result.flushed = flushIfNeeded(true);

    return result;
  }

  CsvLogResult write(const uint8_t* data, size_t len) {
    CsvLogResult result = { false, false };

    if (!_opened || data == nullptr) {
      return result;
    }

    if (_file.write(data, len) != len) {
      return result;
    }

    result.ok = true;
    result.flushed = flushIfNeeded(false);

    return result;
  }

  CsvLogResult writeHeader(const char* line) {
    return println(line);
  }

  CsvLogResult writeRow(const char* line) {
    return println(line);
  }

  const char* currentPath() const {
    return _currentPath;
  }

  bool isOpen() const {
    return _opened;
  }

  bool isBegun() const {
    return _begun;
  }

  uint16_t rowsSinceFlush() const {
    return _rowsSinceFlush;
  }

  uint32_t lastFlushMs() const {
    return _lastFlushMs;
  }

private:
  File _file;

  bool _opened;
  bool _begun;

  uint16_t _flushEveryRows;
  uint32_t _flushIntervalMs;

  uint16_t _rowsSinceFlush;
  uint32_t _lastFlushMs;

  char _currentPath[16];

  bool findNextPath(char* out, size_t outSize) {
    if (out == nullptr || outSize < 14) {
      return false;
    }

    for (uint16_t n = LOG_START_NO; n <= LOG_MAX_NO; n++) {
      makePath(out, outSize, n);

      if (!SD.exists(out)) {
        return true;
      }
    }

    out[0] = '\0';
    return false;
  }

  void makePath(char* out, size_t outSize, uint16_t number) {
    snprintf(
      out,
      outSize,
      "/LOG%04u.CSV",
      (unsigned int)number);
  }

  bool writeBomIfEmpty() {
    if (!_file) {
      return false;
    }

    if (_file.size() != 0) {
      return true;
    }

    const uint8_t bom[3] = { 0xEF, 0xBB, 0xBF };
    return _file.write(bom, 3) == 3;
  }

  bool flushIfNeeded(bool rowWritten) {
    if (!_opened) {
      return false;
    }

    if (rowWritten) {
      if (_rowsSinceFlush < 65535) {
        _rowsSinceFlush++;
      }
    }

    bool shouldFlush = false;

    if (_flushEveryRows > 0 && _rowsSinceFlush >= _flushEveryRows) {
      shouldFlush = true;
    }

    if (_flushIntervalMs > 0) {
      uint32_t now = millis();

      if (now - _lastFlushMs >= _flushIntervalMs) {
        shouldFlush = true;
      }
    }

    if (!shouldFlush) {
      return false;
    }

    return flush();
  }

  void copyPath(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
      return;
    }

    if (src == nullptr) {
      dst[0] = '\0';
      return;
    }

    strncpy(dst, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
  }
};