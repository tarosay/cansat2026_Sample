/**
 * SPITest.ino — SD カード SPI 最小テスト
 *
 * SD.h / SPI.h を一切使わず、SPI1 レジスタを直接叩いて CMD0 を送る。
 * 期待出力:
 *   S037C  … SPI1 初期化成功
 *   R01    … CMD0 応答 0x01 = IDLE（正常）
 *   Done
 *
 * ボード : HID ProMicro CH32V003 SD+WebHID  (Board Version: V1.4)
 *          FQBN: UIAP_HID:ch32v:CH32V00x_SD:opt=oslto
 */

#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN 2
static const uint8_t PIN_SS = 6;   // SD CS = A2 = PC4

static void spi_init(void) {
  RCC->APB2PCENR |= (1u<<4) | (1u<<12);
  GPIOC->CFGLR = (GPIOC->CFGLR
    & ~((0xFu<<16)|(0xFu<<20)|(0xFu<<24)|(0xFu<<28)))
    | (0x3u<<16)|(0xBu<<20)|(0xBu<<24)|(0x8u<<28);
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

// 16bit → 4桁16進 表示（例: "S037C"）
static void print4(char prefix, uint16_t v) {
  char buf[6];
  buf[0] = prefix;
  for (int8_t i = 4; i >= 1; i--) {
    uint8_t n = v & 0xF; v >>= 4;
    buf[i] = (n < 10) ? ('0'+n) : ('A'+n-10);
  }
  buf[5] = '\0';
  hid.Println(buf);
}

// 8bit → 2桁16進 表示（例: "R01"）
static void print2(char prefix, uint8_t v) {
  char buf[4];
  buf[0] = prefix;
  buf[1] = ((v>>4) < 10) ? ('0'+(v>>4)) : ('A'+(v>>4)-10);
  buf[2] = ((v&0xF) < 10) ? ('0'+(v&0xF)) : ('A'+(v&0xF)-10);
  buf[3] = '\0';
  hid.Println(buf);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  WebHID.begin();
  pinMode(PIN_SS, OUTPUT);
  digitalWrite(PIN_SS, HIGH);

  delay(10000);
  hid.Clear();

  spi_init();
  print4('S', SPI1->CTLR1);          // S037C = SPI1 OK

  for (uint8_t i = 0; i < 10; i++) spi_xfer(0xFF);  // 80 clocks CS=HIGH

  digitalWrite(PIN_SS, LOW);          // CS LOW
  spi_xfer(0x40); spi_xfer(0x00); spi_xfer(0x00);
  spi_xfer(0x00); spi_xfer(0x00); spi_xfer(0x95);  // CMD0

  uint8_t r = 0xFF;
  for (uint8_t i = 0; i < 20 && (r & 0x80); i++) r = spi_xfer(0xFF);

  digitalWrite(PIN_SS, HIGH);         // CS HIGH

  print2('R', r);                     // R01 = 成功 / RFF = 失敗
  hid.Println("Done");

  if (r == 0x01) digitalWrite(LED_BUILTIN, HIGH);  // 成功時 LED 点灯
}

void loop() {}
