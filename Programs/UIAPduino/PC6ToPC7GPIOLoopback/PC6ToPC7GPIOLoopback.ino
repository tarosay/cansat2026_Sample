/**
 * PC6ToPC7GPIOLoopback.ino
 *
 * PC6(MOSI/D8) を GPIO 出力、
 * PC7(MISO/D9) を GPIO 入力にして、
 * PC6 -> PC7 の物理ループバックを確認する。
 *
 * 配線:
 *   PC6(MOSI/D8) と PC7(MISO/D9) を直結
 *
 * 期待出力:
 *   L=0
 *   H=1
 *   PASS
 *
 * 異常例:
 *   L=1
 *   H=1
 *     → PC6 LOW が PC7 に届いていない / PC7がプルアップのまま
 *
 *   L=0
 *   H=0
 *     → PC6 HIGH が PC7 に届いていない / GND側に短絡
 *
 *   L=1
 *   H=0
 *     → 配線/読み取り/出力が異常
 */

#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN 2

static void gpio_loopback_init(void) {
  // GPIOC clock enable
  RCC->APB2PCENR |= (1u << 4);

  // PC6: GPIO push-pull output 50MHz
  // PC7: input pull-up/down
  GPIOC->CFGLR =
    (GPIOC->CFGLR & ~((0xFu << 24) | (0xFu << 28)))
    | (0x3u << 24)
    | (0x8u << 28);

  // PC7 pull-up
  GPIOC->BSHR = (1u << 7);
}

static void pc6_low(void) {
  GPIOC->BCR = (1u << 6);
}

static void pc6_high(void) {
  GPIOC->BSHR = (1u << 6);
}

static uint8_t pc7_read(void) {
  return (GPIOC->INDR & (1u << 7)) ? 1 : 0;
}

static void print_result(const char *label, uint8_t value) {
  char buf[8];

  buf[0] = label[0];
  buf[1] = '=';
  buf[2] = value ? '1' : '0';
  buf[3] = '\0';

  hid.Println(buf);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  WebHID.begin();
  delay(10000);
  hid.Clear();

  hid.Println("PC6->PC7 GPIO LOOP");
  hid.Println("WIRE PC6 TO PC7");
  hid.Println("DO NOT APPLY 5V TO PC7");

  gpio_loopback_init();

  pc6_low();
  delay(100);
  uint8_t l = pc7_read();

  pc6_high();
  delay(100);
  uint8_t h = pc7_read();

  print_result("L", l);
  print_result("H", h);

  if (l == 0 && h == 1) {
    hid.Println("PASS");
    digitalWrite(LED_BUILTIN, HIGH);
  } else {
    hid.Println("FAIL");
  }
}

void loop() {
}