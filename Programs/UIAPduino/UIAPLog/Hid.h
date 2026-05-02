#pragma once
#include <stdint.h>

/**
 * Hid.h  —  WebHID コンソール出力クラス (UIAPLog 用)
 *
 * UIAPLog で使うメソッドのみ定義。
 *   - Println(const char*) : 文字列出力 + 改行
 *   - Clear()              : コンソールクリア
 */
class Hid {
public:
  void Println(const char* s);
  void Clear();
  uint8_t Recv(uint8_t* buf, uint8_t maxLen);

private:
  void _hpSend(uint8_t flags, const char* s, uint8_t n);
};

extern Hid hid;
