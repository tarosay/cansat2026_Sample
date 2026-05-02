/**
 * SPINetsAllToggle_D6_D7_D8_D9.ino
 *
 * UIAPduino Pro Micro CH32V003 V1.4
 *
 * SPI使用ピン D6,D7,D8,D9 を同時に
 * 5秒 HIGH → 5秒 LOW で繰り返す。
 *
 * 対象:
 *   D6 = SS / CS
 *   D7 = SCK
 *   D8 = MOSI
 *   D9 = MISO
 *
 * DMMで各Uパッド - GND間を測る。
 *
 * 注意:
 *   D9/MISOもGPIO出力として一時的に使う。
 *   SDカードや外部出力デバイスは外すこと。
 *   D9/PC7には5Vを入れないこと。
 */

#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN 2

#define PIN_CS 6
#define PIN_SCK 7
#define PIN_MOSI 8
#define PIN_MISO 9

static void spiPinsModeOutput(void) {
  pinMode(PIN_CS, OUTPUT);
  pinMode(PIN_SCK, OUTPUT);
  pinMode(PIN_MOSI, OUTPUT);
  pinMode(PIN_MISO, OUTPUT);
}

static void spiPinsHigh(void) {
  digitalWrite(PIN_CS, HIGH);
  digitalWrite(PIN_SCK, HIGH);
  digitalWrite(PIN_MOSI, HIGH);
  digitalWrite(PIN_MISO, HIGH);
}

static void spiPinsLow(void) {
  digitalWrite(PIN_CS, LOW);
  digitalWrite(PIN_SCK, LOW);
  digitalWrite(PIN_MOSI, LOW);
  digitalWrite(PIN_MISO, LOW);
}

void setup() {
  if (FLASH->STATR & (1 << 14)) NVIC_SystemReset();
  SystemReset_StartMode(Start_Mode_BOOT);
  //pinMode(PD4, OUTPUT);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  spiPinsModeOutput();

  // HIGHスタート
  spiPinsHigh();
  digitalWrite(LED_BUILTIN, HIGH);

  WebHID.begin();

  // WebHID接続・ペア選択待ち
  delay(15000);

  hid.Clear();
  hid.Println("SPI NETS ALL TOGGLE");
  hid.Println("D6,D7,D8,D9");
  hid.Println("ALL HIGH 5SEC -> ALL LOW 5SEC");
  hid.Println("Disconnect SD/external devices");
  hid.Println("D9/PC7: DO NOT APPLY 5V");
}

void loop() {
  spiPinsHigh();
  digitalWrite(LED_BUILTIN, HIGH);
  hid.Println("ALL HIGH 5SEC");
  delay(5000);

  spiPinsLow();
  digitalWrite(LED_BUILTIN, LOW);
  hid.Println("ALL LOW 5SEC");
  delay(5000);
}