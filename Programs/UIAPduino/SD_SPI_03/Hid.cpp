#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

Hid hid;

// ── Print プロトコル内部定数 ───────────────────────────────────────────────────
#define _HP_MARKER 0x50
#define _HP_MORE   0x80
#define _HP_NL     0x02
#define _HP_CLEAR  0x04

// ── 基数10専用の最小整数変換 ──────────────────────────────────────────────────
// ltoa (汎用・任意基数) を引き込まないため独自実装。
// buf は 12 bytes 以上必要。変換後の文字列先頭ポインタを返す。
static char *tiny_itoa(int v, char *buf)
{
    char *p = buf + 11;
    *p = '\0';
    unsigned u = (v < 0) ? (unsigned)(-(v + 1)) + 1u : (unsigned)v;
    if (u == 0) {
        *--p = '0';
    } else {
        while (u) { *--p = '0' + (u % 10); u /= 10; }
    }
    if (v < 0) *--p = '-';
    return p;
}

// ── 非ブロッキング送信 ────────────────────────────────────────────────────────
// USB ホストが未接続でも絶対にハングしない。
// 前の送信がまだ完了していない場合はこのパケットを破棄する。
void Hid::_hpSend(uint8_t flags, const char* s, uint8_t n)
{
    if (WebHID.busy()) return;   // 送信中 → 破棄 (ブロックしない)
    uint8_t buf[8] = { _HP_MARKER, flags, 0, 0, 0, 0, 0, 0 };
    for (uint8_t i = 0; i < n; i++) buf[i + 2] = (uint8_t)s[i];
    WebHID.send(buf, 8);
    // delay なし: OpenLog はリアルタイム性を優先
}

void Hid::Print(const char* s)
{
    int len = strlen(s), off = 0;
    if (!len) return;
    while (off < len) {
        int n = len - off;
        if (n > 6) n = 6;
        _hpSend((off + n < len) ? _HP_MORE : 0, s + off, n);
        off += n;
    }
}

void Hid::Print(int v)
{
    char b[12];
    Print(tiny_itoa(v, b));
}

void Hid::Println(const char* s)
{
    int len = strlen(s), off = 0;
    if (!len) {
        _hpSend(_HP_NL, "", 0);
        return;
    }
    while (off < len) {
        int n = len - off;
        if (n > 6) n = 6;
        bool last = (off + n >= len);
        _hpSend(last ? _HP_NL : _HP_MORE, s + off, n);
        off += n;
    }
}

void Hid::Println(int v)
{
    char b[12];
    Println(tiny_itoa(v, b));
}

void Hid::Clear()
{
    _hpSend(_HP_CLEAR, "", 0);
}

uint8_t Hid::Recv(uint8_t* buf, uint8_t maxLen)
{
    return WebHID.recv(buf, maxLen);
}
