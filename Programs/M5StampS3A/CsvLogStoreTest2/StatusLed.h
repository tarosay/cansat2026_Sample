#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "BoardConfig.h"

// Status NeoPixel colors
static constexpr uint32_t STATUS_LED_COLOR_WRITE_GREEN = 0x007D00;
static constexpr uint32_t STATUS_LED_COLOR_ERROR_RED = 0x7D0000;

static Adafruit_NeoPixel statusLedPixel(1, STATUS_LED_PIN);

static void ledOn(uint32_t color);
static void ledOff();

static void ledBegin() {
  pinMode(STATUS_LED_EN_PIN, OUTPUT);
  digitalWrite(STATUS_LED_EN_PIN, HIGH);

  statusLedPixel.begin();
  ledOff();
}

static void ledOn(uint32_t color) {
  statusLedPixel.setPixelColor(0, color);
  statusLedPixel.show();
}

static void ledOff() {
  statusLedPixel.clear();
  statusLedPixel.show();
}