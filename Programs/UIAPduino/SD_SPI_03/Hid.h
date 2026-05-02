#pragma once
#include <stdint.h>

/**
 * Hid.h  —  WebHID コンソール出力クラス (SD_SPI_03 用)
 *
 * HidPrint の Hid.h と同じインタフェース。
 * 内部実装の違い:
 *   - tiny_itoa を使用 (ltoa を引き込まない)
 *   - _hpSend が非ブロッキング (USB未接続時にハングしない)
 */
class Hid {
public:
  void Print(const char* s);
  void Print(int v);
  void Println(const char* s = "");
  void Println(int v);
  void Clear();
  uint8_t Recv(uint8_t* buf, uint8_t maxLen);

private:
  void _hpSend(uint8_t flags, const char* s, uint8_t n);
};

extern Hid hid;
