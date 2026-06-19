#pragma once

#include <Arduino.h>
#include <SD.h>

class CsvLogStore {
public:
  static constexpr uint16_t LOG_START_NO = 1;
  static constexpr uint16_t LOG_MAX_NO = 9999;

  CsvLogStore()
    : _opened(false) {
    _currentPath[0] = '\0';
  }

  bool begin() {
    _opened = false;
    _currentPath[0] = '\0';
    return true;
  }

  bool openNext() {
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
    return true;
  }

  bool openExistingAppend(const char* path) {
    if (path == nullptr) {
      return false;
    }

    if (_opened) {
      close();
    }

    _file = SD.open(path, FILE_APPEND);
    if (!_file) {
      _opened = false;
      _currentPath[0] = '\0';
      return false;
    }

    copyPath(_currentPath, sizeof(_currentPath), path);
    _opened = true;

    return true;
  }

  bool close() {
    if (!_opened) {
      return true;
    }

    _file.flush();
    _file.close();

    _opened = false;
    return true;
  }

  bool flush() {
    if (!_opened) {
      return false;
    }

    _file.flush();
    return true;
  }

  bool print(const char* text) {
    if (!_opened || text == nullptr) {
      return false;
    }

    size_t len = strlen(text);
    return _file.write((const uint8_t*)text, len) == len;
  }

  bool println(const char* text) {
    if (!_opened || text == nullptr) {
      return false;
    }

    size_t len = strlen(text);

    if (_file.write((const uint8_t*)text, len) != len) {
      return false;
    }

    return _file.write((const uint8_t*)"\r\n", 2) == 2;
  }

  bool write(const uint8_t* data, size_t len) {
    if (!_opened || data == nullptr) {
      return false;
    }

    return _file.write(data, len) == len;
  }

  bool writeHeader(const char* line) {
    return println(line);
  }

  bool writeRow(const char* line) {
    return println(line);
  }

  const char* currentPath() const {
    return _currentPath;
  }

  bool isOpen() const {
    return _opened;
  }

private:
  File _file;
  bool _opened;
  char _currentPath[16];

  bool findNextPath(char* out, size_t outSize) {
    if (out == nullptr || outSize < 13) {
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
    // 8.3形式: /LOG0001.CSV
    // LOG_MAX_NO=9999 なのでファイル名本体は LOG + 4桁 = 7文字
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