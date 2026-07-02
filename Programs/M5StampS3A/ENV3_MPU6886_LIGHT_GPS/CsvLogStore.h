#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "BoardConfig.h"
#include "StatusLed.h"

// ============================================================================
// CsvLogStore.h
//
// SDカードに CSV 形式のログを書き込むためのクラスです。
//
// 【できること】
//  - SDカードを初期化する（begin）
//  - /LOG0001.CSV, /LOG0002.CSV ... のように、まだ使われていない
//    連番のファイル名を自動で探して新しいログファイルを作る（openNext）
//  - 1行ずつ文字列を書き込む（println / writeRow）
//  - 一定の行数ごと・一定の時間ごとに自動でフラッシュ（後述）する
//
// 【フラッシュ（flush）とは？】
//  write() や println() で書いたデータは、すぐには SDカードに
//  記録されず、いったんメモリ上のバッファに溜められます。
//  flush() を呼ぶと、バッファの中身が実際に SDカードへ書き込まれます。
//  フラッシュする前に電源が切れると、バッファに残っていたデータは
//  消えてしまいます。かといって毎回フラッシュすると遅くなるので、
//  「N行ごと」「Nミリ秒ごと」に自動フラッシュする仕組みを持っています。
//
// ※このファイルの日本語コメントは Claude Fable 5（Anthropic の AI）が
//   作成しました（2026-07-02）。
// ============================================================================

// print / println / write の戻り値として使う構造体。
// 「書き込みが成功したか」と「そのとき自動フラッシュされたか」の
// 2つの情報をまとめて返します。
struct CsvLogResult {
  bool ok;       // true = 書き込み成功
  bool flushed;  // true = この書き込みのタイミングで自動フラッシュが実行された
};

class CsvLogStore {
public:
  // ログファイルの連番の範囲。
  // /LOG0001.CSV から /LOG9999.CSV まで作れます。
  static constexpr uint16_t LOG_START_NO = 1;
  static constexpr uint16_t LOG_MAX_NO = 9999;

  // コンストラクタ。メンバ変数を初期値にそろえるだけで、
  // まだ SDカードには何もしません（実際の初期化は begin() で行う）。
  CsvLogStore()
    : _opened(false),
      _begun(false),
      _flushEveryRows(0),
      _flushIntervalMs(0),
      _rowsSinceFlush(0),
      _lastFlushMs(0) {
    _currentPath[0] = '\0';
  }

  // SDカードを初期化します。setup() の中で最初に1回呼びます。
  // 成功したら true、SDカードが認識できなければ false を返します。
  bool begin() {
    _opened = false;
    _currentPath[0] = '\0';

    // SPI通信を開始します。SDカードとは SPI という方式で通信します。
    //
    // 【ピンを自分で指定している理由】
    // M5Stamp S3A には「SPIはこの端子」という固定の専用ピンがありません。
    // 搭載マイコン ESP32-S3 は「GPIOマトリクス」という仕組みを持っていて、
    // チップ内蔵のハードウェアSPIコントローラを、ほぼどのGPIOピンにでも
    // 自由につなぎ替えることができます。
    // そのため、ここでは配線に合わせて選んだピンを明示的に指定しています。
    // 使うピン番号は BoardConfig.h に定義されています。
    SPI.begin(
      SD_SPI_SCK_PIN,   // クロック（通信のタイミングを合わせる信号）
      SD_SPI_MISO_PIN,  // SDカード → マイコン のデータ線
      SD_SPI_MOSI_PIN,  // マイコン → SDカード のデータ線
      SD_SPI_CS_PIN);   // チップセレクト（通信相手を選ぶ信号）

    // SDライブラリを起動。カードが挿さっていない・接触不良などで失敗する。
    if (!SD.begin(SD_SPI_CS_PIN, SPI, SD_SPI_FREQ)) {
      _begun = false;
      return false;
    }

    // カードの種類が「なし」= カードを認識できていないのでエラー。
    if (SD.cardType() == CARD_NONE) {
      _begun = false;
      return false;
    }

    _begun = true;
    return true;
  }

  // 「何行書いたら自動フラッシュするか」を設定します。
  // 0 のままなら行数によるフラッシュはしません。
  void setFlushEveryRows(uint16_t rows) {
    _flushEveryRows = rows;
  }

  // 「何ミリ秒たったら自動フラッシュするか」を設定します。
  // 0 のままなら時間によるフラッシュはしません。
  void setFlushIntervalMs(uint32_t intervalMs) {
    _flushIntervalMs = intervalMs;
  }

  // 新しいログファイルを開きます。
  // SDカードの中を調べて、まだ存在しない番号（例: /LOG0003.CSV）を
  // 自動で見つけて、そのファイルを作成して書き込み用に開きます。
  bool openNext() {
    // begin() が成功していなければ何もできない。
    if (!_begun) {
      return false;
    }

    // すでに別のファイルを開いていたら、先にそれを閉じる。
    if (_opened) {
      close();
    }

    char path[16];

    // 未使用の連番ファイル名を探す。
    if (!findNextPath(path, sizeof(path))) {
      return false;
    }

    // ファイルを書き込みモードで開く（なければ新規作成される）。
    _file = SD.open(path, FILE_WRITE);
    if (!_file) {
      _opened = false;
      _currentPath[0] = '\0';
      return false;
    }

    // 開いたファイル名を覚えておく（currentPath() で確認できる）。
    copyPath(_currentPath, sizeof(_currentPath), path);

    // 新規ファイルなら先頭に BOM を書き込む（詳細は writeBomIfEmpty 参照）。
    if (!writeBomIfEmpty()) {
      _file.close();
      _opened = false;
      _currentPath[0] = '\0';
      return false;
    }

    _opened = true;
    _rowsSinceFlush = 0;      // フラッシュ後の行数カウンタをリセット
    _lastFlushMs = millis();  // 最後にフラッシュした時刻を今にする

    return true;
  }

  // ファイルを閉じます。閉じる前に必ずフラッシュするので、
  // 書き込み済みのデータが失われることはありません。
  bool close() {
    if (!_opened) {
      return true;  // もともと開いていなければ「閉じた」扱いで成功
    }

    flush();
    _file.close();

    _opened = false;
    _rowsSinceFlush = 0;

    return true;
  }

  // バッファに溜まっているデータを、実際に SDカードへ書き込みます。
  // 書き込み中は LED を緑色に点灯させて、動作が目で見えるようにしています。
  bool flush() {
    if (!_opened) {
      return false;
    }

    ledOn(STATUS_LED_COLOR_WRITE_GREEN);  // LED点灯（書き込み中の合図）
    _file.flush();                        // ここで本当にSDカードに書かれる
    ledOff();                             // LED消灯

    _rowsSinceFlush = 0;
    _lastFlushMs = millis();

    return true;
  }

  // 文字列を「改行なし」で書き込みます。
  // 1行を少しずつ組み立てたいときに使います。
  CsvLogResult print(const char* text) {
    CsvLogResult result = { false, false };

    // ファイルが開いていない、または text が無効なら失敗。
    if (!_opened || text == nullptr) {
      return result;
    }

    size_t len = strlen(text);

    // 文字列をバッファに書き込む。書けたバイト数が一致しなければ失敗。
    if (_file.write((const uint8_t*)text, len) != len) {
      return result;
    }

    result.ok = true;
    // 改行していないので「行が増えた」とは数えない（引数 false）。
    result.flushed = flushIfNeeded(false);

    return result;
  }

  // 文字列を書き込み、最後に改行（CR+LF）を付けます。
  // CSVの1行を書くときはこれを使います。
  CsvLogResult println(const char* text) {
    CsvLogResult result = { false, false };

    if (!_opened || text == nullptr) {
      return result;
    }

    size_t len = strlen(text);

    if (_file.write((const uint8_t*)text, len) != len) {
      return result;
    }

    // "\r\n" は Windows 標準の改行コード。
    // Excel やメモ帳で開いたときに正しく改行されるようにしています。
    if (_file.write((const uint8_t*)"\r\n", 2) != 2) {
      return result;
    }

    result.ok = true;
    // 1行書き終わったので「行が増えた」と数える（引数 true）。
    result.flushed = flushIfNeeded(true);

    return result;
  }

  // 文字列ではなく、バイト列（バイナリデータ）をそのまま書き込みます。
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

  // CSVのヘッダ行（1行目の項目名）を書き込みます。
  // 中身は println() と同じですが、呼び出す側のコードが
  // 読みやすくなるように名前を分けています。
  CsvLogResult writeHeader(const char* line) {
    return println(line);
  }

  // CSVのデータ行を1行書き込みます。中身は println() と同じです。
  CsvLogResult writeRow(const char* line) {
    return println(line);
  }

  // 現在開いているファイルのパス（例: "/LOG0003.CSV"）を返します。
  const char* currentPath() const {
    return _currentPath;
  }

  // ファイルが開いているかどうかを返します。
  bool isOpen() const {
    return _opened;
  }

  // begin()（SDカードの初期化）が成功しているかどうかを返します。
  bool isBegun() const {
    return _begun;
  }

  // 最後にフラッシュしてから書き込んだ行数を返します。
  uint16_t rowsSinceFlush() const {
    return _rowsSinceFlush;
  }

  // 最後にフラッシュした時刻（millis() の値）を返します。
  uint32_t lastFlushMs() const {
    return _lastFlushMs;
  }

private:
  File _file;  // 現在開いているファイルの実体

  bool _opened;  // ファイルを開いているか
  bool _begun;   // SDカードの初期化に成功しているか

  uint16_t _flushEveryRows;   // 自動フラッシュする行数（0 = 無効）
  uint32_t _flushIntervalMs;  // 自動フラッシュする間隔ミリ秒（0 = 無効）

  uint16_t _rowsSinceFlush;  // 最後のフラッシュから書いた行数
  uint32_t _lastFlushMs;     // 最後にフラッシュした時刻（millis() の値）

  char _currentPath[16];  // 開いているファイルのパス（"/LOG0001.CSV" など）

  // まだ存在しないログファイル名を探します。
  // LOG0001.CSV から順番に SD.exists() で「もうあるか」を調べ、
  // 見つからなかった（= 未使用の）名前を out に入れて true を返します。
  // 9999 まで全部使われていたら false を返します。
  bool findNextPath(char* out, size_t outSize) {
    // "/LOG0001.CSV" は13文字+終端文字なので、バッファが14未満だと入らない。
    if (out == nullptr || outSize < 14) {
      return false;
    }

    for (uint16_t n = LOG_START_NO; n <= LOG_MAX_NO; n++) {
      makePath(out, outSize, n);

      if (!SD.exists(out)) {
        return true;  // まだ存在しない番号が見つかった
      }
    }

    out[0] = '\0';
    return false;  // 9999個すべて使用済み
  }

  // 番号からファイルパスを作ります。例: 3 → "/LOG0003.CSV"
  // %04u は「4桁になるように0で埋めた数字」という意味です。
  void makePath(char* out, size_t outSize, uint16_t number) {
    snprintf(
      out,
      outSize,
      "/LOG%04u.CSV",
      (unsigned int)number);
  }

  // ファイルが空（新規作成直後）なら、先頭に UTF-8 の BOM を書き込みます。
  // BOM（Byte Order Mark）は「このファイルは UTF-8 です」という目印の
  // 3バイトで、これがあると Excel で開いたときに日本語が文字化けしません。
  bool writeBomIfEmpty() {
    if (!_file) {
      return false;
    }

    // すでに中身があるファイルなら BOM は書かない。
    if (_file.size() != 0) {
      return true;
    }

    const uint8_t bom[3] = { 0xEF, 0xBB, 0xBF };
    return _file.write(bom, 3) == 3;
  }

  // 自動フラッシュの条件を満たしているか調べて、必要ならフラッシュします。
  // rowWritten: 直前の書き込みで1行増えたなら true（println から呼ばれたとき）。
  // 戻り値: 実際にフラッシュしたら true。
  bool flushIfNeeded(bool rowWritten) {
    if (!_opened) {
      return false;
    }

    if (rowWritten) {
      // カウンタがあふれない（65535を超えない）ようにガードしつつ加算。
      if (_rowsSinceFlush < 65535) {
        _rowsSinceFlush++;
      }
    }

    bool shouldFlush = false;

    // 条件1: 設定した行数に達したらフラッシュ。
    if (_flushEveryRows > 0 && _rowsSinceFlush >= _flushEveryRows) {
      shouldFlush = true;
    }

    // 条件2: 設定した時間が経過していたらフラッシュ。
    if (_flushIntervalMs > 0) {
      uint32_t now = millis();

      // 「now - _lastFlushMs」という引き算で経過時間を求めると、
      // millis() が約50日で一周（オーバーフロー）しても正しく動きます。
      if (now - _lastFlushMs >= _flushIntervalMs) {
        shouldFlush = true;
      }
    }

    if (!shouldFlush) {
      return false;
    }

    return flush();
  }

  // 文字列を安全にコピーします。
  // strncpy はコピー先がいっぱいになると終端文字 '\0' を付けないことが
  // あるので、最後に必ず '\0' を入れて文字列が壊れないようにしています。
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
