#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>

class SdSpiStorage {
public:
  SdSpiStorage()
    : _begun(false),
      _sck(0),
      _miso(0),
      _mosi(0),
      _cs(0),
      _freq(25000000) {
  }

  bool begin(
    uint8_t sck,
    uint8_t miso,
    uint8_t mosi,
    uint8_t cs,
    uint32_t freq = 25000000) {
    _sck = sck;
    _miso = miso;
    _mosi = mosi;
    _cs = cs;
    _freq = freq;

    SPI.begin(_sck, _miso, _mosi, _cs);

    if (!SD.begin(_cs, SPI, _freq)) {
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

  bool isBegun() const {
    return _begun;
  }

  bool printCardInfo(Stream& out) {
    if (!_begun) {
      out.println("SD not begun.");
      return false;
    }

    uint8_t cardType = SD.cardType();

    out.print("SD Card Type: ");
    if (cardType == CARD_MMC) {
      out.println("MMC");
    } else if (cardType == CARD_SD) {
      out.println("SDSC");
    } else if (cardType == CARD_SDHC) {
      out.println("SDHC");
    } else if (cardType == CARD_NONE) {
      out.println("NONE");
      return false;
    } else {
      out.println("UNKNOWN");
    }

    uint64_t cardSizeMb = SD.cardSize() / (1024ULL * 1024ULL);

    out.print("SD Card Size: ");
    out.print((unsigned long)cardSizeMb);
    out.println(" MB");

    return true;
  }

  bool exists(const char* path) {
    if (!_begun || path == nullptr) {
      return false;
    }

    return SD.exists(path);
  }

  bool remove(const char* path) {
    if (!_begun || path == nullptr) {
      return false;
    }

    if (!SD.exists(path)) {
      return true;
    }

    return SD.remove(path);
  }

  bool mkdir(const char* path) {
    if (!_begun || path == nullptr) {
      return false;
    }

    if (SD.exists(path)) {
      return true;
    }

    return SD.mkdir(path);
  }

  bool writeText(const char* path, const char* text) {
    if (!_begun || path == nullptr || text == nullptr) {
      return false;
    }

    File file = SD.open(path, FILE_WRITE);
    if (!file) {
      return false;
    }

    size_t len = strlen(text);
    size_t written = file.write((const uint8_t*)text, len);
    file.close();

    return written == len;
  }

  bool appendText(const char* path, const char* text) {
    if (!_begun || path == nullptr || text == nullptr) {
      return false;
    }

    File file = SD.open(path, FILE_APPEND);
    if (!file) {
      return false;
    }

    size_t len = strlen(text);
    size_t written = file.write((const uint8_t*)text, len);
    file.close();

    return written == len;
  }

  bool appendLine(const char* path, const char* line) {
    if (!_begun || path == nullptr || line == nullptr) {
      return false;
    }

    File file = SD.open(path, FILE_APPEND);
    if (!file) {
      return false;
    }

    size_t len = strlen(line);
    bool ok = true;

    if (file.write((const uint8_t*)line, len) != len) {
      ok = false;
    }

    if (file.write((const uint8_t*)"\r\n", 2) != 2) {
      ok = false;
    }

    file.close();
    return ok;
  }

  bool readTo(const char* path, Stream& out) {
    if (!_begun || path == nullptr) {
      return false;
    }

    File file = SD.open(path, FILE_READ);
    if (!file) {
      return false;
    }

    while (file.available()) {
      out.write(file.read());
    }

    file.close();
    return true;
  }

  bool copy(const char* srcPath, const char* dstPath) {
    if (!_begun || srcPath == nullptr || dstPath == nullptr) {
      return false;
    }

    File src = SD.open(srcPath, FILE_READ);
    if (!src) {
      return false;
    }

    File dst = SD.open(dstPath, FILE_WRITE);
    if (!dst) {
      src.close();
      return false;
    }

    uint8_t buffer[128];

    while (src.available()) {
      size_t n = src.read(buffer, sizeof(buffer));
      if (n == 0) {
        break;
      }

      if (dst.write(buffer, n) != n) {
        src.close();
        dst.close();
        return false;
      }
    }

    src.close();
    dst.close();

    return true;
  }

  bool overwriteByte(const char* path, uint32_t pos, uint8_t value) {
    if (!_begun || path == nullptr) {
      return false;
    }

    File file = SD.open(path, "r+");
    if (!file) {
      return false;
    }

    if (pos >= file.size()) {
      file.close();
      return false;
    }

    if (!file.seek(pos)) {
      file.close();
      return false;
    }

    size_t written = file.write(value);
    file.close();

    return written == 1;
  }

  bool fileSize(const char* path, uint32_t& sizeOut) {
    sizeOut = 0;

    if (!_begun || path == nullptr) {
      return false;
    }

    File file = SD.open(path, FILE_READ);
    if (!file) {
      return false;
    }

    sizeOut = (uint32_t)file.size();
    file.close();

    return true;
  }

private:
  bool _begun;

  uint8_t _sck;
  uint8_t _miso;
  uint8_t _mosi;
  uint8_t _cs;
  uint32_t _freq;
};