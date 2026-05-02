/**
 * D8Output5sec.ino
 *
 * Arduino pin D8(MOSI) を pinMode/digitalWrite で
 * 5秒 HIGH → 5秒 LOW を繰り返す。
 *
 * DMMで D8(MOSI) - GND 間を測定する。
 *
 * PC6-PC7は接続しない。
 */

#include <Arduino.h>
#include <WebHID.h>
#include "Hid.h"

#define LED_BUILTIN 2
#define TEST_PIN 8   // D8 = MOSI

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TEST_PIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  digitalWrite(TEST_PIN, HIGH);   // HIGHスタート

  WebHID.begin();

  // Webサイト側の接続・ペア選択の手動操作待ち
  delay(8000);

  hid.Clear();
  hid.Println("D8 OUTPUT 5SEC");
  hid.Println("MEASURE D8/MOSI TO GND");
  hid.Println("START = HIGH");
  hid.Println("DO NOT CONNECT D8 TO PC7");

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(TEST_PIN, HIGH);
  hid.Println("D8 HIGH 5SEC");
}

void loop() {
  digitalWrite(TEST_PIN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  hid.Println("D8 HIGH 5SEC");
  delay(5000);

  digitalWrite(TEST_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  hid.Println("D8 LOW 5SEC");
  delay(5000);
}