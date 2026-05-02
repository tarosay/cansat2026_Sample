/**
 * PC6Output10sec.ino
 *
 * PC6(MOSI/D8) をGPIO出力として
 * 10秒 LOW → 10秒 HIGH を繰り返す。
 *
 * DMMで PC6/D8 - GND 間を測定する。
 */

#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN 2

static void pc6_output_init(void) {
  RCC->APB2PCENR |= (1u << 4);

  GPIOC->CFGLR =
    (GPIOC->CFGLR & ~(0xFu << 24))
    | (0x3u << 24);   // PC6 GPIO push-pull output 50MHz
}

static void pc6_low(void) {
  GPIOC->BCR = (1u << 6);
}

static void pc6_high(void) {
  GPIOC->BSHR = (1u << 6);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  WebHID.begin();
  delay(3000);
  hid.Clear();

  hid.Println("PC6 OUTPUT 10SEC");
  hid.Println("MEASURE PC6/D8 TO GND");
  hid.Println("DO NOT CONNECT PC6 TO PC7");

  pc6_output_init();
}

void loop() {
  pc6_low();
  digitalWrite(LED_BUILTIN, LOW);
  hid.Println("PC6 LOW 10SEC");
  delay(10000);

  pc6_high();
  digitalWrite(LED_BUILTIN, HIGH);
  hid.Println("PC6 HIGH 10SEC");
  delay(10000);
}