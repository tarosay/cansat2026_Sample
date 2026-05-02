/**
 * SD_SPI_03 — OpenLog 代用ファームウェア (WebHID ステータス付き)
 *
 * ボード: HID ProMicro CH32V003 SD+WebHID
 *         (Board Version: V1.4)
 *
 * 機能:
 *   - UART で受け取ったデータを SD カードに記録 (OpenLog 相当)
 *   - ブラウザの WebHID コンソールでファイル名・状態を確認できる
 *
 * ログファイル: LOG00000.TXT, LOG00001.TXT, ...
 *   起動のたびに番号が 1 つ増える (SD_SPI_02 と同じ命名規則)
 *
 * UART:
 *   RX = PD6  9600 bps (HardwareSerial を使わず直接レジスタ制御)
 *
 * WebHID:
 *   hid-console.html を開いて接続すると起動ログ・エラーが見える。
 *   USB未接続でも UART→SD 記録は正常に動作する。
 *
 * フラッシュ節約のため SDClass/File ラッパーを使わず SdFat を直接使用。
 *   - SD.exists() / File クラス を使わない → walkPath, malloc 等が除去される
 *   - O_CREAT|O_EXCL で「ファイルが存在すれば失敗」を利用して空き番号を探す
 */

#include <Arduino.h>
#include <SD.h>      // Sd2Card / SdVolume / SdFile / O_* 定数のため
#include <SPI.h>
#include <WebHID.h>
#include "Hid.h"

static const uint8_t PIN_SS = 6;

// SDClass/File ラッパーを経由せず SdFat を直接使う
static Sd2Card  card;
static SdVolume volume;
static SdFile   root;
static SdFile   logFile;

// ── 最小 UART1 受信 (HardwareSerial を引き込まない) ────────────────────────
// CH32V003: USART1, PD6=RX (alternate function)
// RCC->APB2PCENR は ch32fun.h で定義済み
// ※ コアの uart_init と名前衝突を避けるため sd03_ プレフィックスを付ける

static void sd03_uart_init(uint32_t baud)
{
    // GPIOD + USART1 クロック有効
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;
    // PD6: 入力フロート (USART1 RX)
    GPIOD->CFGLR = (GPIOD->CFGLR & ~(0xFu << 24))
                 | (0x4u << 24);   // CNF=01(floating input), MODE=00
    // ボーレート (HCLK=48MHz)
    USART1->BRR = (uint16_t)(48000000UL / baud);
    // 受信のみ有効
    USART1->CTLR1 = USART_Mode_Rx | USART_CTLR1_UE;
}
static uint8_t sd03_uart_available(void)
{
    return (USART1->STATR & USART_FLAG_RXNE) ? 1 : 0;
}
static uint8_t sd03_uart_read(void)
{
    return (uint8_t)(USART1->DATAR & 0xFFu);
}

// ── ファイル名生成 "LOG00000.TXT" ─────────────────────────────────────────────
static void makeFilename(char* buf, uint16_t n)
{
    buf[0]='L'; buf[1]='O'; buf[2]='G';
    buf[3]='0'+(n/10000)%10;
    buf[4]='0'+(n/1000)%10;
    buf[5]='0'+(n/100)%10;
    buf[6]='0'+(n/10)%10;
    buf[7]='0'+(n)%10;
    buf[8]='.'; buf[9]='T'; buf[10]='X'; buf[11]='T'; buf[12]=0;
}

// ── フラッシュカウンタ (millis() を使わない) ──────────────────────────────────
static uint16_t flushCounter = 0;
static const uint16_t FLUSH_EVERY = 512;  // 512 バイトごとにフラッシュ

void setup()
{
    WebHID.begin();
    SPI.begin();

    delay(10000);   // USB 列挙待ち
    hid.Clear();

    // ── SD 初期化 (SdFat 直接) ──────────────────────────────────────────────
    if (!card.init(SPI_HALF_SPEED, PIN_SS) ||
        !volume.init(card) ||
        !root.openRoot(volume)) {
        hid.Println("SD FAIL");
        while (1);
    }

    // ── 空きファイル名を探してオープン ─────────────────────────────────────
    // O_CREAT|O_EXCL: ファイルが既に存在すると open が失敗する性質を利用。
    // SD.exists() + walkPath 系コードを引き込まない。
    char filename[13];
    uint16_t i;
    for (i = 0; i < 65535; i++) {
        makeFilename(filename, i);
        if (logFile.open(root, filename, O_WRITE | O_CREAT | O_EXCL)) break;
    }

    if (!logFile.isOpen()) {
        hid.Println("OPEN FAIL");
        while (1);
    }

    hid.Println(filename);
    hid.Println("Ready.");

    // ── UART 開始 ───────────────────────────────────────────────────────────
    sd03_uart_init(9600);
}

void loop()
{
    // UART → SD 書き込み
    while (sd03_uart_available()) {
        logFile.write(sd03_uart_read());
        if (++flushCounter >= FLUSH_EVERY) {
            logFile.sync();   // SdFile::sync() = File::flush() の実体
            flushCounter = 0;
        }
    }
}
