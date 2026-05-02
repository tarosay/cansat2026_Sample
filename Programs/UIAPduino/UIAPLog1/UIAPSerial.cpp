#include <Arduino.h>
#include "UIAPSerial.h"

UIAPSerial uart;

// ── USART1 受信初期化 ─────────────────────────────────────────────────────────
// PD6 を入力プルアップ、USART1 を RX-only で起動する。
void UIAPSerial::begin(uint32_t baud)
{
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;

  // PD6: 入力プルアップ (CNF=10, MODE=00 → 0x8)
  GPIOD->CFGLR = (GPIOD->CFGLR & ~(0xFu << 24))
               | (0x8u << 24);
  GPIOD->OUTDR |= (1u << 6);

  // ボーレート設定 (クロック 48MHz)
  USART1->BRR   = (uint16_t)(48000000UL / baud);
  USART1->CTLR1 = USART_Mode_Rx | USART_CTLR1_UE;

  // 受信バッファクリア
  volatile uint16_t dummy = USART1->DATAR;
  (void)dummy;
}

// ── 受信データあり？ ──────────────────────────────────────────────────────────
uint8_t UIAPSerial::available()
{
  return (USART1->STATR & USART_FLAG_RXNE) ? 1 : 0;
}

// ── 1 バイト読み出し ─────────────────────────────────────────────────────────
uint8_t UIAPSerial::read()
{
  return (uint8_t)(USART1->DATAR & 0xFFu);
}
