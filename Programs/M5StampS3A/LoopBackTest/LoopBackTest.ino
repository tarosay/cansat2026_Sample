#include <Arduino.h>

#define PIN_BUTTON 0

void setup() {
  pinMode(PIN_BUTTON, INPUT);
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, 1, 3);
  Serial2.begin(19200, SERIAL_8N1, 44, 43);

  delay(2500);
  Serial.println("LoopBack Test");
}

void loop() {
  if (Serial.available() > 0) {
    int receivedByte = Serial.read();
    Serial1.write(receivedByte);
  }

  if (Serial1.available() > 0) {
    int receivedByte = Serial1.read();
    Serial2.write(receivedByte);
  }

  if (Serial2.available() > 0) {
    int receivedByte = Serial2.read();
    Serial.write(receivedByte);
  }
  delay(10);
}