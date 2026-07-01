# cansat2026_Sample

缶サット講習会2026 の公開サンプルデータです。

## リポジトリ構成

| フォルダ | 内容 |
|---|---|
| `Programs` | Arduino スケッチ一式（`M5StampS3A` 以下、詳細は下記参照）|
| `Schematic` | 基板の回路図・ガーバーデータ（CanSat 本体基板、UIAP_Log 基板）|
| `3D-CAD` | CanSat 筐体の3D CADデータ（STL / rsdocx）|
| `Documents` | 事前学習資料（スライド）、モデルロケットの図面（JWW/PDF）|

## 対応ハードウェア

- **メインボード**: M5Stamp S3A (ESP32-S3)
- ピン配置: `Programs/M5StampS3A/S007-V033_PinMap_01.jpg` を参照

## 必要ライブラリ

Arduino IDE のライブラリマネージャからインストールしてください。

| ライブラリ | 用途 |
|---|---|
| Adafruit NeoPixel | NeoPixel LED |
| Adafruit SSD1306 / Adafruit GFX | OLED ディスプレイ |
| Adafruit SHT31 Library | 温湿度センサ SHT3X |
| M5Unit-ENV | 気圧センサ QMP6988 |
| I2C_MPU6886 | IMU MPU6886 |
| TinyGPSPlus | GPS データ解析 |

---

## サンプルスケッチ一覧

### 基本動作確認

| スケッチ | 概要 |
|---|---|
| `LED_ChikaChika` | NeoPixel LED の点滅（赤・緑・青・白を順に切り替え）|
| `Buzzer` | パッシブブザーで音を鳴らす（GPIO1）|
| `I2C_Scan` | 接続されている I2C デバイスのアドレスを全スキャン |

### ストレージ

| スケッチ | 概要 |
|---|---|
| `SD_SPI_01` | SD カードの初期化・ファイル書き込み・読み出し・コピー |
| `SD_SPI_02` | SD カードの基本動作確認（SD_SPI_01 の簡易版）|
| `SD_Log_01` | Log クラスを使った SD カードへのテキストログ |
| `CsvLogStoreTest` | CsvLogStore ライブラリの基本動作テスト |
| `CsvLogStoreTest2` | CsvLogStore + NeoPixel ステータス LED |
| `CsvLogStoreTest3` | CsvLogStore の発展テスト |
| `OpenLog_Sample_19200` | OpenLog へ UART（19200bps）でログ出力（GPIO1/3）|

### 表示

| スケッチ | 概要 |
|---|---|
| `OLED_SSD1306` | SSD1306 OLED（128×32）にカウンタ表示（I2C 0x3C、SDA=13、SCL=15）|

### GPS

| スケッチ | 概要 |
|---|---|
| `GPSV1.1_Sample1` | GPS モジュールのデータをシリアルモニタに生 NMEA 出力 |
| `GPSV1.1_Sample2` | TinyGPSPlus で解析し、位置・時刻（JST）・速度・高度を表示 |
| `GPSV1_1_Sample3` | GPS データを SD カードに CSV ログ保存 |

GPS UART ピン: RX=GPIO44、TX=GPIO43（Serial2、115200bps）

### センサ

| スケッチ | 概要 |
|---|---|
| `LIGHT_AD5_IO7_Sample` | 光センサのアナログ電圧（GPIO5）とデジタル閾値（GPIO7）を読み取り |
| `MPU6880_1` | IMU MPU6886 の初期化と加速度・角速度・温度のシリアル出力 |
| `MPU6880_2` | IMU 50Hz で SD カードに CSV ログ（加速度・角速度・温度）|
| `ENV3_1` | ENV III ユニット 10Hz で SD カードに CSV ログ（温湿度・気圧・高度）|
| `IMU_PRO_to_OpenLog` | IMU データを OpenLog に UART 出力 |

### 統合ログ（複数センサ同時記録）

| スケッチ | レート | 記録データ |
|---|---|---|
| `ENV3MPU6880_1` | 20Hz | 温湿度・気圧・高度・加速度・角速度・IMU温度 |
| `ENV3MPU6880_LIGHT_1` | 20Hz | 上記 + 光センサ（アナログ電圧・デジタル）|

---

## ピン配置まとめ

| 機能 | ピン |
|---|---|
| NeoPixel データ | GPIO21 |
| NeoPixel 電源 EN | GPIO38 |
| ブザー | GPIO1 |
| I2C SDA | GPIO13 |
| I2C SCL | GPIO15 |
| SD MOSI | GPIO2 |
| SD MISO | GPIO4 |
| SD SCK | GPIO6 |
| SD CS | GPIO8 |
| GPS RX | GPIO44 |
| GPS TX | GPIO43 |
| 光センサ アナログ | GPIO5 |
| 光センサ デジタル | GPIO7 |

### I2C デバイスアドレス

| デバイス | アドレス |
|---|---|
| SHT31（温湿度）| 0x44 |
| QMP6988（気圧）| 0x70 |
| MPU6886（IMU）| 0x68 |
| SSD1306（OLED）| 0x3C |

---

## CSV ログ形式（ENV3MPU6880_LIGHT_1）

```
time_ms, row,
sht_temperature_c, humidity_rh,
qmp_temperature_c, pressure_pa, altitude_m,
accel_x_g, accel_y_g, accel_z_g,
gyro_x_dps, gyro_y_dps, gyro_z_dps,
imu_temperature_c,
light_v, light_d
```

- `altitude_m`: 起動時の地上気圧を基準とした相対高度（打ち上げ地点 = 0m）
- `light_v`: 光センサのアナログ電圧（0〜3.3V）
- `light_d`: 光センサの閾値デジタル出力（0 または 1）
