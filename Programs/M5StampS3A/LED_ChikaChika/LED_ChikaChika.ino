#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define PIN_BUTTON 0
#define LED_PIN 21
#define PIN_LED_EN 38  // S3A 固有の LED 電源イネーブルピン

#define COLOR_REPEAT 2

// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, LED_PIN);

uint8_t color = 0, count = 0;
uint32_t colors[] = { pixels.Color(125, 0, 0), pixels.Color(0, 125, 0), pixels.Color(0, 0, 125), pixels.Color(125, 125, 125) };
const uint8_t COLORS_LEN = (uint8_t)(sizeof(colors) / sizeof(colors[0]));

void setup() {
  pinMode(PIN_LED_EN, OUTPUT);
  digitalWrite(PIN_LED_EN, HIGH);  // S3A: LED 電源イネーブル

  Serial.begin(115200);
  pixels.begin();  // initialize the pixel
  Serial.println("StampS3 demo!");
  pinMode(PIN_BUTTON, INPUT);
}

void loop() {
  pixels.setPixelColor(0, colors[color]);
  pixels.show();

  delay(250);

  pixels.clear();
  pixels.show();

  delay(250);

  Serial.println(count);
  count++;

  if (count >= COLOR_REPEAT) {
    count = 0;
    color++;
    if (color >= COLORS_LEN) {
      color = 0;
    }
  }

  Serial.print("button: ");
  Serial.println(digitalRead(PIN_BUTTON));
}