/**
 * SPILoopback.ino — SPI1 ループバックテスト
 *
 * PC6(MOSI) と PC7(MISO) を物理的に直結して実行する。
 * SD カード不要。SPI1 ハードウェアそのものを確認する。
 *
 * 期待出力:
 *   AA OK  … 0xAA を送って 0xAA が返ってきた → SPI1 正常
 *   55 OK  … 0x55 を送って 0x55 が返ってきた → SPI1 正常
 *   PASS   … 全テスト合格
 *
 * 失敗例:
 *   AA NG xx … 送った値と違う値が返ってきた → SPI1 異常
 *
 * ボード : HID ProMicro CH32V003 SD+WebHID  (Board Version: V1.4)
 *          FQBN: UIAP_HID:ch32v:CH32V00x_SD:opt=oslto
 */

#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN 2

static void spi_init(void) {
  RCC->APB2PCENR |= (1u<<4) | (1u<<12);
  GPIOC->CFGLR = (GPIOC->CFGLR
    & ~((0xFu<<16)|(0xFu<<20)|(0xFu<<24)|(0xFu<<28)))
    | (0x3u<<16)   // PC4 GP PP (未使用だが設定)
    | (0xBu<<20)   // PC5 AF PP 50MHz (SCK)
    | (0xBu<<24)   // PC6 AF PP 50MHz (MOSI)
    | (0x8u<<28);  // PC7 input pull-up (MISO)
  GPIOC->BSHR = (1u<<4) | (1u<<7);
  SPI1->CTLR2 = 0;
  SPI1->CTLR1 = (7u<<3)|(1u<<2)|(1u<<9)|(1u<<8)|(1u<<6); // 0x037C
}

static uint8_t spi_xfer(uint8_t b) {
  while (!(SPI1->STATR & (1u<<1)));
  SPI1->DATAR = b;
  while (!(SPI1->STATR & (1u<<0)));
  return (uint8_t)SPI1->DATAR;
}

// "AA OK" または "AA NG xx" を表示
static bool test_byte(uint8_t send) {
  uint8_t recv = spi_xfer(send);

  char buf[9];
  // 送信値 2桁16進
  buf[0] = (send>>4) < 10 ? ('0'+(send>>4)) : ('A'+(send>>4)-10);
  buf[1] = (send&0xF) < 10 ? ('0'+(send&0xF)) : ('A'+(send&0xF)-10);
  buf[2] = ' ';
  if (recv == send) {
    buf[3]='O'; buf[4]='K'; buf[5]='\0';
  } else {
    buf[3]='N'; buf[4]='G'; buf[5]=' ';
    buf[6] = (recv>>4) < 10 ? ('0'+(recv>>4)) : ('A'+(recv>>4)-10);
    buf[7] = (recv&0xF) < 10 ? ('0'+(recv&0xF)) : ('A'+(recv&0xF)-10);
    buf[8] = '\0';
  }
  hid.Println(buf);
  return (recv == send);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  WebHID.begin();

  delay(10000);
  hid.Clear();

  spi_init();

  bool ok = true;
  ok &= test_byte(0xAA);
  ok &= test_byte(0x55);
  ok &= test_byte(0xFF);
  ok &= test_byte(0x00);
  ok &= test_byte(0xA5);

  if (ok) {
    hid.Println("PASS");
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    hid.Println("FAIL");
  }
}

void loop() {}
