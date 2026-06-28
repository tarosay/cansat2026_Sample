#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  Serial1.begin(19200, SERIAL_8N1, 1, 3);  //G1->RXD, G3->TXD);
  //Serial1.begin(9600, SERIAL_8N1, 1, 3);  //G1->RXD, G3->TXD);

  delay(1500);
  Serial.println(F("Stamp S3 OpenLog Sample1_19200"));
  Serial1.println(F("Stamp S3 OpenLog Sample1_19200"));
}
int i = 0;
void loop() {
  delay(250);

  Serial.print(i);
  Serial.println(F(" Stamp S3 OpenLog Sample1_19200"));

  Serial1.print(i);
  Serial1.println(F(" Stamp S3 OpenLog Sample1_19200"));
  i++;
}
