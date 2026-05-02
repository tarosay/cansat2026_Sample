/**
 * SPIDiag.ino — SD カード SPI 初期化 (v22c)
 *
 * v21  結果 (bit-bang, arg=0x40000000, delay=10ms): FF@=04, RAF@=03
 * v22a 結果 (bit-bang, arg=0x40300000, delay=10ms): FF@=02, RAF@=02 → 悪化
 *   → 電圧窓ビット追加は逆効果。arg は 0x40000000 に戻す。
 *
 * v22c の変更：bit-bang + arg=0x40000000 + delay 10ms → 50ms
 *   ACMD41 間隔を伸ばしてカード内部初期化に余裕を持たせる
 *   64回 × 50ms = 3.2 秒（SD 仕様最大 1 秒を十分カバー）
 *   RAF@ が増える → タイミング不足が原因
 *   RAF@ 変わらず → 物理層（電源・配線・MISO）の問題
 *
 * ボード : HID ProMicro CH32V003 SD+WebHID  (Board Version: V1.4)
 *          FQBN: UIAP_HID:ch32v:CH32V00x_SD:opt=oslto
 */

#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN 2
/*
SPIとSDメモリカード
A2 CS  - DAT3/CS
8 MOSI - CMD/DI
3V3    - VDD
7 SCK  - CLK
GND    - VSS
9 MISO - DAT0/DO
*/

/* v21: GPIOC のみ使用（SPI1 ペリフェラル不使用）
   PC4=CS(out PP), PC5=SCK(out PP), PC6=MOSI(out PP), PC7=MISO(in pull-up) */
static void spi_init(void) {
  RCC->APB2PCENR |= (1u << 4);  /* GPIOC クロックのみ（SPI1 クロック不要）*/
  GPIOC->CFGLR = (GPIOC->CFGLR
                  & ~((0xFu << 16) | (0xFu << 20) | (0xFu << 24) | (0xFu << 28)))
                 | (0x3u << 16)   /* PC4 CS   : output PP 50MHz */
                 | (0x3u << 20)   /* PC5 SCK  : output PP 50MHz (v21: AF→PP) */
                 | (0x3u << 24)   /* PC6 MOSI : output PP 50MHz (v21: AF→PP) */
                 | (0x8u << 28);  /* PC7 MISO : input pull-up */
  GPIOC->BSHR = (1u << 4) | (1u << 7); /* CS=HIGH, MISO pull-up */
  GPIOC->BCR  = (1u << 5);             /* SCK=LOW（アイドル）*/
}

/* bit-bang SPI Mode 0 (CPOL=0, CPHA=0) — 約 100kHz */
static uint8_t xfer(uint8_t b) {
  uint8_t r = 0;
  for (uint8_t i = 0; i < 8; i++) {
    /* MOSI セット（SCK 立ち上がり前に確定）*/
    if (b & 0x80) GPIOC->BSHR = (1u << 6);
    else          GPIOC->BCR  = (1u << 6);
    b <<= 1;
    delayMicroseconds(5);
    GPIOC->BSHR = (1u << 5);  /* SCK HIGH（立ち上がりエッジ）*/
    delayMicroseconds(5);
    r = (uint8_t)((r << 1) | ((GPIOC->INDR >> 7) & 1u)); /* MISO 読み取り */
    GPIOC->BCR = (1u << 5);   /* SCK LOW（立ち下がりエッジ）*/
  }
  return r;
}

static void cs_low(void) {
  GPIOC->BCR = (1u << 4);
}
static void cs_high(void) {
  GPIOC->BSHR = (1u << 4);
}

static void cs_cycle(void) {
  cs_high();
  for (uint8_t i = 0; i < 8; i++) xfer(0xFF);
  cs_low();
}

static uint8_t last_wait; /* 直前の send_cmd で応答が返ったバイト位置 (0xFF=タイムアウト) */

static uint8_t send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {
  xfer(0xFF);
  xfer(cmd | 0x40);
  xfer((uint8_t)(arg >> 24));
  xfer((uint8_t)(arg >> 16));
  xfer((uint8_t)(arg >> 8));
  xfer((uint8_t)(arg));
  xfer(crc);
  uint8_t r = 0xFF;
  last_wait = 0xFF;
  for (uint8_t i = 0; i < 64; i++) {  /* v21a: 20 → 64 バイト */
    r = xfer(0xFF);
    if (!(r & 0x80)) { last_wait = i; break; }
  }
  return r;
}

static void printhex2(const char *label, uint8_t v) {
  char buf[16];
  uint8_t p = 0;
  while (label[p]) {
    buf[p] = label[p];
    p++;
  }
  buf[p] = (v >> 4) < 10 ? ('0' + (v >> 4)) : ('A' + (v >> 4) - 10);
  buf[p + 1] = (v & 0xF) < 10 ? ('0' + (v & 0xF)) : ('A' + (v & 0xF) - 10);
  buf[p + 2] = '\0';
  hid.Println(buf);
}

/* 最初の 5 回分の W（応答バイト位置）記録用 */
static uint8_t call_cnt;
static uint8_t rws[5], rwa[5];

/* ACMD41 1回 — v20 構造維持（CMD55/CMD41 同一 CS） */
static void do_acmd41(uint32_t arg, uint8_t *r55_out, uint8_t *r41_out) {
  cs_high();                                   /* CS=HIGH（非選択）にしてから待つ */
  delay(50);                                   /* v22c: 10ms → 50ms */
  for (uint8_t i = 0; i < 8; i++) xfer(0xFF); /* ダミークロック（CS=HIGH）*/
  cs_low();
  *r55_out = send_cmd(0x37, 0, 0xFF);
  if (call_cnt < 5) rws[call_cnt] = last_wait;
  *r41_out = send_cmd(0x29, arg, 0xFF);
  if (call_cnt < 5) rwa[call_cnt] = last_wait;
  call_cnt++;
  /* cs_high() は次の呼び出し先頭で実行される */
}

/* 60 回分の出力変数（すべて別アドレス） */
#define CALLS 60
static uint8_t rs[CALLS], ra[CALLS];

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  WebHID.begin();
  delay(10000);
  hid.Clear();

  spi_init();
  cs_high();
  for (uint8_t i = 0; i < 10; i++) xfer(0xFF);

  /* 初期化シーケンス */
  cs_low();
  uint8_t r0 = send_cmd(0x00, 0, 0x95);
  cs_cycle();
  cs_low();
  uint8_t r55p = send_cmd(0x37, 0, 0xFF);
  cs_cycle();
  cs_low();
  uint8_t r8 = send_cmd(0x08, 0x1AA, 0x87);
  uint8_t r8t = 0;
  for (uint8_t i = 0; i < 4; i++) r8t = xfer(0xFF);
  cs_cycle();
  /* v20: CMD8 後の再 CMD0 を削除（非標準手順を排除） */

  uint32_t arg = (r8 == 0x01) ? 0x40000000UL : 0UL;  /* v22c: 0x40300000 → 0x40000000 に戻す */

  /* ── 60 回個別展開（ループなし）──────────────────────────────── */
  do_acmd41(arg, &rs[0], &ra[0]);
  do_acmd41(arg, &rs[1], &ra[1]);
  do_acmd41(arg, &rs[2], &ra[2]);
  do_acmd41(arg, &rs[3], &ra[3]);
  do_acmd41(arg, &rs[4], &ra[4]);
  do_acmd41(arg, &rs[5], &ra[5]);
  do_acmd41(arg, &rs[6], &ra[6]);
  do_acmd41(arg, &rs[7], &ra[7]);
  do_acmd41(arg, &rs[8], &ra[8]);
  do_acmd41(arg, &rs[9], &ra[9]);
  do_acmd41(arg, &rs[10], &ra[10]);
  do_acmd41(arg, &rs[11], &ra[11]);
  do_acmd41(arg, &rs[12], &ra[12]);
  do_acmd41(arg, &rs[13], &ra[13]);
  do_acmd41(arg, &rs[14], &ra[14]);
  do_acmd41(arg, &rs[15], &ra[15]);
  do_acmd41(arg, &rs[16], &ra[16]);
  do_acmd41(arg, &rs[17], &ra[17]);
  do_acmd41(arg, &rs[18], &ra[18]);
  do_acmd41(arg, &rs[19], &ra[19]);
  do_acmd41(arg, &rs[20], &ra[20]);
  do_acmd41(arg, &rs[21], &ra[21]);
  do_acmd41(arg, &rs[22], &ra[22]);
  do_acmd41(arg, &rs[23], &ra[23]);
  do_acmd41(arg, &rs[24], &ra[24]);
  do_acmd41(arg, &rs[25], &ra[25]);
  do_acmd41(arg, &rs[26], &ra[26]);
  do_acmd41(arg, &rs[27], &ra[27]);
  do_acmd41(arg, &rs[28], &ra[28]);
  do_acmd41(arg, &rs[29], &ra[29]);
  do_acmd41(arg, &rs[30], &ra[30]);
  do_acmd41(arg, &rs[31], &ra[31]);
  do_acmd41(arg, &rs[32], &ra[32]);
  do_acmd41(arg, &rs[33], &ra[33]);
  do_acmd41(arg, &rs[34], &ra[34]);
  do_acmd41(arg, &rs[35], &ra[35]);
  do_acmd41(arg, &rs[36], &ra[36]);
  do_acmd41(arg, &rs[37], &ra[37]);
  do_acmd41(arg, &rs[38], &ra[38]);
  do_acmd41(arg, &rs[39], &ra[39]);
  do_acmd41(arg, &rs[40], &ra[40]);
  do_acmd41(arg, &rs[41], &ra[41]);
  do_acmd41(arg, &rs[42], &ra[42]);
  do_acmd41(arg, &rs[43], &ra[43]);
  do_acmd41(arg, &rs[44], &ra[44]);
  do_acmd41(arg, &rs[45], &ra[45]);
  do_acmd41(arg, &rs[46], &ra[46]);
  do_acmd41(arg, &rs[47], &ra[47]);
  do_acmd41(arg, &rs[48], &ra[48]);
  do_acmd41(arg, &rs[49], &ra[49]);
  do_acmd41(arg, &rs[50], &ra[50]);
  do_acmd41(arg, &rs[51], &ra[51]);
  do_acmd41(arg, &rs[52], &ra[52]);
  do_acmd41(arg, &rs[53], &ra[53]);
  do_acmd41(arg, &rs[54], &ra[54]);
  do_acmd41(arg, &rs[55], &ra[55]);
  do_acmd41(arg, &rs[56], &ra[56]);
  do_acmd41(arg, &rs[57], &ra[57]);
  do_acmd41(arg, &rs[58], &ra[58]);
  do_acmd41(arg, &rs[59], &ra[59]);

  cs_high();

  /* 結果表示 */
  printhex2("R0  =", r0);
  printhex2("R55p=", r55p);
  printhex2("R8  =", r8);
  printhex2("R8T =", r8t);
  /* R0b は v20 で削除 */

  /* インデックス検索 */
  uint8_t first_ff    = 0xFF; /* CMD55 が初めて FF になった回 */
  uint8_t first_ra_ff = 0xFF; /* CMD41 が初めて FF になった回 */
  uint8_t first_ok    = 0xFF; /* CMD41 が初めて 00 になった回 */
  for (uint8_t i = 0; i < CALLS; i++) {
    if (rs[i] == 0xFF && first_ff    == 0xFF) first_ff    = i;
    if (ra[i] == 0xFF && first_ra_ff == 0xFF) first_ra_ff = i;
    if (ra[i] == 0x00 && first_ok    == 0xFF) first_ok    = i;
  }

  /* 最初の 5 回（S/WS/A/WA） */
  for (uint8_t i = 0; i < 5; i++) {
    printhex2("S=",  rs[i]);
    printhex2("WS=", rws[i]); /* CMD55 応答バイト位置 */
    printhex2("A=",  ra[i]);
    printhex2("WA=", rwa[i]); /* CMD41 応答バイト位置 */
  }

  printhex2("FF@ =", first_ff);    /* CMD55 初 FF (0xFF=なし) */
  printhex2("RAF@=", first_ra_ff); /* CMD41 初 FF (0xFF=なし) */
  printhex2("OK@ =", first_ok);    /* CMD41 初 00 (0xFF=なし) */

  if (first_ok != 0xFF) {
    hid.Println("INIT OK");
    digitalWrite(LED_BUILTIN, HIGH);
  } else if (first_ff == 0) {
    hid.Println("FF FROM 1ST");
  } else if (first_ff != 0xFF) {
    hid.Println("FF PARTWAY");
  } else {
    hid.Println("ALL 01");
  }
}

void loop() {}
