/**
 * UART TX test
 * Board : HID ProMicro CH32V003 / CH32V00x系
 * UART  : USART1 TX = PD5
 * Baud  : 9600
 *
 * 接続:
 *   送信側 PD5(TX) -> UIAPLog1 側 PD6(RX)
 *   GND           -> GND
 */

#include <Arduino.h>

#define LED_BUILTIN 2

static const uint32_t UART_BAUD = 19200UL;
static const uint32_t SEND_INTERVAL_MS = 5000UL;

static void uiap_uart_tx_init(uint32_t baud) {
  RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;

  // PD5 = USART1_TX
  // MODE=11: output 50MHz
  // CNF =10: alternate function push-pull
  GPIOD->CFGLR = (GPIOD->CFGLR & ~(0xFu << 20))
                 | (0xBu << 20);

  USART1->BRR = (uint16_t)(48000000UL / baud);
  USART1->CTLR1 = USART_Mode_Tx | USART_CTLR1_UE;
}

static void uiap_uart_write(uint8_t b) {
  while ((USART1->STATR & USART_FLAG_TXE) == 0) {
  }
  USART1->DATAR = b;
}

static void uiap_uart_print(const char *s) {
  while (*s) {
    uiap_uart_write((uint8_t)*s++);
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  uiap_uart_tx_init(UART_BAUD);
}

void loop() {
  static uint32_t sendCount = 0;

  digitalWrite(LED_BUILTIN, HIGH);

  char buf[64];
  snprintf(
    buf,
    sizeof(buf),
    "Hello World. count=%lu millis=%lu\r\n",
    (unsigned long)sendCount,
    (unsigned long)millis()
  );

  uiap_uart_print(buf);

  sendCount++;

  digitalWrite(LED_BUILTIN, LOW);
  delay(SEND_INTERVAL_MS);
}