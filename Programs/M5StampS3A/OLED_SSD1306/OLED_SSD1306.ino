#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_WIDTH 128
#define OLED_HEIGHT 32

#define OLED_ADDR 0x3C

#define PIN_SDA 13
#define PIN_SCL 15

Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void setup() {
  Serial.begin(115200);
  delay(1500);

  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("OLED init failed. Try address 0x3D.");
    while (true) {
      delay(1000);
    }
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("M5Stamp S3");

  display.setCursor(0, 12);
  display.println("SSD1306 128x32");

  display.setCursor(0, 24);
  display.println("I2C 0x3C SDA13 SCL15");

  display.display();
}

void loop() {
  static uint32_t counter = 0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println("M5Stamp S3");

  display.setCursor(0, 12);
  display.print("Count: ");
  display.println(counter++);

  display.setCursor(0, 24);
  display.println("OLED OK");

  display.display();

  delay(500);
}