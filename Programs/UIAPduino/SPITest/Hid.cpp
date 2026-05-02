#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

Hid hid;

// ── Print プロトコル内部定数 ───────────────────────────────────────────────────
#define _HP_MARKER 0x50
#define _HP_MORE   0x80
#define _HP_NL     0x02
#define _HP_CLEAR  0x04

// ── 送信 ─────────────────────────────────────────────────────────────────────
// busy なら最大 20ms 待つ。USB 未接続時も delay はハングしない。
void Hid::_hpSend(uint8_t flags, const char* s, uint8_t n)
{
    if (WebHID.busy()) delay(20);
    if (WebHID.busy()) return;
    uint8_t buf[8] = { _HP_MARKER, flags, 0, 0, 0, 0, 0, 0 };
    for (uint8_t i = 0; i < n; i++) buf[i + 2] = (uint8_t)s[i];
    WebHID.send(buf, 8);
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

void Hid::Clear()
{
    _hpSend(_HP_CLEAR, "", 0);
}

uint8_t Hid::Recv(uint8_t* buf, uint8_t maxLen)
{
    return WebHID.recv(buf, maxLen);
}
