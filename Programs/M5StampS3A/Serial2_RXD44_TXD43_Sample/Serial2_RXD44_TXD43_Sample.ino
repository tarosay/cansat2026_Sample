#include <Arduino.h>
//#include <SoftwareSerial.h>
//#define SOFT_RXD 5
//#define SOFT_TXD 7
//SoftwareSerial SoftSerial(SOFT_RXD, SOFT_TXD);

void setup() {
  Serial.begin(115200);
  //Serial1.begin(115200, SERIAL_8N1, 1, 3);  //G1->RXD, G3->TXD
  ////Serial1.setRxBufferSize(512); //default 128

  Serial2.begin(115200, SERIAL_8N1, 44, 43);  //G44->RXD, G43->TXD
  //Serial2.setRxBufferSize(512); //default 128

  ////SoftSerial.begin(38400, SWSERIAL_8N1, SOFT_RXD, SOFT_TXD, false, 256);
  //SoftSerial.begin(38400); //ソフトシリアルは38400以下でないと通信できない。
  delay(500);
  Serial.println("Stamp S3 Loopback Test");
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

  // if (SoftSerial.available() > 0) {
  //   int receivedByte = SoftSerial.read();
  //   Serial.write(receivedByte);
  // }

  delay(10);
}