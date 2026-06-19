// M5Stamp S3 + passive buzzer
// Buzzer: G1 / GPIO1

const int BUZZER_PIN = 1;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);

  // 起動確認音
  tone(BUZZER_PIN, 1000, 200);  // 1kHz, 200ms
  delay(300);
  tone(BUZZER_PIN, 1500, 200);  // 1.5kHz, 200ms
  delay(300);
  noTone(BUZZER_PIN);
}

void loop() {
  tone(BUZZER_PIN, 440, 300);   // A4
  delay(400);

  tone(BUZZER_PIN, 880, 300);   // A5
  delay(400);

  noTone(BUZZER_PIN);
  delay(1000);
}