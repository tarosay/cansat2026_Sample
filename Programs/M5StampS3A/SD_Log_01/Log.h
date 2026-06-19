#pragma once
#include <Arduino.h>

// ── 設定 ────────────────────────────────────────────────────────────────────
#define LOG_BUF_SIZE       512   // RAM バッファサイズ (bytes)
#define LOG_IDLE_MS        500   // 最後の書き込みから閉じるまでのアイドル時間 (ms)
#define LOG_MAX_FLUSH_MS  2000   // 連続書き込み中でも強制フラッシュする最大間隔 (ms)
// ────────────────────────────────────────────────────────────────────────────

/**
 * Log クラス — SPI 接続 SD カードへの電源断対策ロガー
 *
 * 電源断対策の仕組み:
 *   - データは RAM バッファに蓄積
 *   - バッファ満杯 or LOG_IDLE_MS 間無書き込みで SD へフラッシュ
 *   - フラッシュは open → write → flush → close で完結させ、
 *     途中で電源断しても FAT エントリが破損しない
 *
 * 基本的な使い方:
 *   Log sdLog;
 *   sdLog.begin(PIN_SD_CS, PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI);
 *   sdLog.println("time_ms,temp,pres");
 *   // loop() 内で必ず呼ぶ
 *   sdLog.update();
 */
class Log {
public:
  Log();

  /**
     * SD 初期化とログファイル確保
     * @param ssPin   CS ピン
     * @param sckPin  SCK ピン (-1 = SPI.begin() を呼ばずデフォルト SPI を使用)
     * @param misoPin MISO ピン
     * @param mosiPin MOSI ピン
     * @return true: 成功 / false: SD 初期化失敗またはファイル作成失敗
     */
  bool begin(uint8_t ssPin,
             int8_t sckPin = -1,
             int8_t misoPin = -1,
             int8_t mosiPin = -1);

  // ── 書き込み API ────────────────────────────────────────────────────────
  size_t print(const char* str);
  size_t println(const char* str = "");

  size_t print(int val);
  size_t println(int val);

  size_t print(long val);
  size_t println(long val);

  size_t print(unsigned long val);
  size_t println(unsigned long val);

  size_t print(float val, int digits = 2);
  size_t println(float val, int digits = 2);

  size_t print(const String& str);
  size_t println(const String& str);

  /** printf 形式で書き込む (最大 255 文字) */
  size_t printf(const char* fmt, ...);

  // ── 制御 API ────────────────────────────────────────────────────────────
  /**
     * バッファをすぐに SD へ書き込む (電源断前や重要データ直後に呼ぶ)
     */
  void flush();

  /**
     * loop() から毎回呼ぶ。アイドル検出時にバッファを SD へ書き込む。
     */
  void update();

  /**
     * LED コールバックを登録する
     * 書き込みデータが溜まると on=true、SD へ書き終えると on=false で呼ばれる
     * 例: sdLog.setLedCallback([](bool on){ neopixel.setPixelColor(0, on ? BLUE : 0); neopixel.show(); });
     */
  typedef void (*LedCallback)(bool on);
  void setLedCallback(LedCallback cb) { _ledCb = cb; }

  /** 現在のログファイル名を返す (例: "/00003.TXT") */
  const char* filename() const {
    return _filename;
  }

  /** SD が正常に初期化されているか */
  bool ready() const {
    return _ready;
  }

private:
  char        _filename[12];  // "/00000.TXT\0"
  uint8_t     _buf[LOG_BUF_SIZE];
  uint16_t    _bufLen;
  uint32_t    _lastWriteTime;
  uint32_t    _lastFlushTime;
  bool        _dirty;
  bool        _ready;
  LedCallback _ledCb;

  static void makeFilename(char* buf, uint32_t n);
  void writeChar(char c);
  void flushBuffer();
};
