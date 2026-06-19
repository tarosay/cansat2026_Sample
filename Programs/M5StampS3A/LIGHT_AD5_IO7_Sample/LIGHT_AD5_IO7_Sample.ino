#include <Arduino.h>

#define A_OUT 5
#define D_OUT 7

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(A_OUT, INPUT);
  pinMode(D_OUT, INPUT);

  Serial.println("Stamp-S3 LIGHT Demo Sample");
}

void loop() {
  float vInput = 3.3 * analogRead(A_OUT) / 4095;  //電圧Vに変換
  int dInput = digitalRead(D_OUT);

  String log = "";
  log += String(vInput) + "," + String(dInput);
  log += ",3.5,0";
  Serial.println(log);

  delay(100);
}
