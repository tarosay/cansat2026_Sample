#pragma once
#include <stdint.h>

/**
 * UIAPSerial.h  —  USART1 受信クラス (UIAPLog1 用)
 *
 * CH32V003 USART1: RX = PD6 (ピン16), TX は未使用
 * レジスタ直接アクセス（HardwareSerial 未使用）
 *
 * 使い方:
 *   uart.begin(9600);          // RX 初期化
 *   if (uart.available()) {    // 受信データあり
 *     uint8_t b = uart.read(); // 1 バイト読み出し
 *   }
 */
class UIAPSerial {
public:
  void    begin(uint32_t baud);
  uint8_t available();
  uint8_t read();
};

extern UIAPSerial uart;
