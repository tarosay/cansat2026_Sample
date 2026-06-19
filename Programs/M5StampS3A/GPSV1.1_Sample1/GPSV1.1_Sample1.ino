#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 44, 43);

  delay(1500);
  Serial.println("GPS V1.1 Sample");
}

void loop() {
  if (Serial.available() > 0) {
    int receivedByte = Serial.read();
    Serial2.write(receivedByte);
  }

  if (Serial2.available() > 0) {
    int receivedByte = Serial2.read();
    Serial.write(receivedByte);
  }
  delay(10);
}