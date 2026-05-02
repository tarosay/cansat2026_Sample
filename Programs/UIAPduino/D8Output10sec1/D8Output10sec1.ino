#include <Arduino.h>

#define LED_BUILTIN 2
#define TEST_PIN 8   // D8 = MOSI

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(TEST_PIN, OUTPUT);
}

void loop() {
  digitalWrite(TEST_PIN, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(5000);

  digitalWrite(TEST_PIN, LOW);
  digitalWrite(LED_BUILTIN, LOW);
  delay(5000);
}