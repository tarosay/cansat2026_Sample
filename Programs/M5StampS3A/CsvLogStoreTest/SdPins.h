#pragma once

#include <Arduino.h>

static constexpr uint8_t SD_SPI_MOSI_PIN = 2;
static constexpr uint8_t SD_SPI_MISO_PIN = 4;
static constexpr uint8_t SD_SPI_SCK_PIN  = 6;
static constexpr uint8_t SD_SPI_CS_PIN   = 8;

static constexpr uint32_t SD_SPI_FREQ = 25000000;