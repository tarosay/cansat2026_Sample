
#include <Adafruit_BMP280.h>
#include <Wire.h>
#include <SparkFun_BMI270_Arduino_Library.h>
#include <Adafruit_NeoPixel.h>

#define PIN_BUTTON 0
#define LED_PIN 21
#define PIN_LED_EN 38  // S3A 固有の LED 電源イネーブルピン


Adafruit_BMP280 bmp;  // I2C
Adafruit_Sensor *bmp_temp = bmp.getTemperatureSensor();
Adafruit_Sensor *bmp_pressure = bmp.getPressureSensor();
BMI270 imu;

// I2C address selection
uint8_t i2cAddress = BMI2_I2C_PRIM_ADDR;  // 0x68
//uint8_t i2cAddress = BMI2_I2C_SEC_ADDR; // 0x69

//#define SEALEVELPRESSURE_HPA 1013.25
#define SEALEVELPRESSURE_HPA 1009.0

// create a pixel strand with 1 pixel on PIN_NEOPIXEL
Adafruit_NeoPixel pixels(1, LED_PIN);
uint32_t colorRED = pixels.Color(125, 0, 0);
uint32_t colorGREEN = pixels.Color(0, 125, 0);
uint32_t colorBLUE = pixels.Color(0, 0, 125);


void setup() {
  pinMode(PIN_LED_EN, OUTPUT);
  digitalWrite(PIN_LED_EN, HIGH);  // S3A: LED 電源イネーブル

  Serial.begin(115200);
  Serial1.begin(19200, SERIAL_8N1, 1, 3);  //G1->RXD, G3->TXD);
  delay(15000);
  Serial.println(F("IMU PRO Sample"));

  pixels.begin();  // initialize the pixel
  pinMode(PIN_BUTTON, INPUT);


  // Initialize the I2C library
  Wire.begin();

  while (imu.beginI2C(i2cAddress) != BMI2_OK) {
    // Not connected, inform user
    Serial.println("Error: BMI270 not connected, check wiring and I2C address!");

    // Wait a bit to see if connection is established
    delay(1000);
  }

  int8_t err = BMI2_OK;

  // Set accelerometer config
  bmi2_sens_config accelConfig;
  accelConfig.type = BMI2_ACCEL;
  accelConfig.cfg.acc.odr = BMI2_ACC_ODR_50HZ;
  accelConfig.cfg.acc.bwp = BMI2_ACC_OSR4_AVG1;
  accelConfig.cfg.acc.filter_perf = BMI2_PERF_OPT_MODE;
  accelConfig.cfg.acc.range = BMI2_ACC_RANGE_8G;
  err = imu.setConfig(accelConfig);

  // Set gyroscope config
  bmi2_sens_config gyroConfig;
  gyroConfig.type = BMI2_GYRO;
  gyroConfig.cfg.gyr.odr = BMI2_GYR_ODR_50HZ;
  gyroConfig.cfg.gyr.bwp = BMI2_GYR_OSR4_MODE;
  gyroConfig.cfg.gyr.filter_perf = BMI2_PERF_OPT_MODE;
  gyroConfig.cfg.gyr.ois_range = BMI2_GYR_OIS_250;
  gyroConfig.cfg.gyr.range = BMI2_GYR_RANGE_125;
  gyroConfig.cfg.gyr.noise_perf = BMI2_PERF_OPT_MODE;
  err = imu.setConfig(gyroConfig);

  // Check whether the config settings above were valid
  while (err != BMI2_OK) {
    // Not valid, determine which config was the problem
    if (err == BMI2_E_ACC_INVALID_CFG) {
      Serial.println("Accelerometer config not valid!");
    } else if (err == BMI2_E_GYRO_INVALID_CFG) {
      Serial.println("Gyroscope config not valid!");
    } else if (err == BMI2_E_ACC_GYR_INVALID_CFG) {
      Serial.println("Both configs not valid!");
    } else {
      Serial.print("Unknown error: ");
      Serial.println(err);
    }
    delay(1000);
  }

  if (!bmp.begin(BMP280_ADDRESS_ALT, BMP280_CHIPID)) {
    Serial.println(F("Failed to initialize BMP280!"));
    while (1)
      ;
  }

  /* Default settings from datasheet. */
  bmp.setSampling(
    //Adafruit_BMP280::MODE_FORCED,     /* Operating Mode. */
    Adafruit_BMP280::MODE_NORMAL,     /* Operating Mode. */
    Adafruit_BMP280::SAMPLING_X2,     /* Temp. oversampling */
    Adafruit_BMP280::SAMPLING_X16,    /* Pressure oversampling */
    Adafruit_BMP280::FILTER_X16,      /* Filtering. */
    Adafruit_BMP280::STANDBY_MS_500); /* Standby time. */

  //BMP280センサの詳細を表示
  bmp_temp->printSensorDetails();
  bmp_pressure->printSensorDetails();
}

float Ax = 0;
float Ay = 0;
float Az = 0;
float Gx = 0;
float Gy = 0;
float Gz = 0;
float Temp, Press, Alti;

void loop() {
  pixels.setPixelColor(0, colorRED);
  pixels.show();

  imu.getSensorData();

  Temp = bmp.readTemperature();
  Press = bmp.readPressure();
  //Alti = bmp.readAltitude(SEALEVELPRESSURE_HPA);
  Alti = calculateAltitude(Press, Temp);

  String log = String(millis() / 1000.0, 2) + ",";
  log += String(imu.data.accelX) + "," + String(imu.data.accelY) + "," + String(imu.data.accelZ) + ",";
  log += String(imu.data.gyroX) + "," + String(imu.data.gyroY) + "," + String(imu.data.gyroZ) + ",";
  log += String(Temp) + "," + String(Press) + "," + String(Alti) + ",";
  Serial.println(log);
  Serial1.println(log);

  pixels.clear();
  pixels.show();

  delay(250);
}

/*
* 気圧と温度から高度を計算します
*/
float calculateAltitude(float pressure, float temperature) {
  // 摂氏からケルビンへ変換
  float T = temperature + 273.15;
  // 海面気圧をパスカル単位に変換
  float P0 = SEALEVELPRESSURE_HPA * 100.0;
  // 気温の減率
  float L = 0.0065;
  // 基準温度
  float T0 = 288.15;
  // 気体定数
  float R = 8.3144598;
  // 重力加速度
  float g = 9.80665;
  // 空気のモル質量
  float M = 0.0289644;

  // 高度計算
  return (T / L) * (pow((P0 / pressure), (R * L) / (g * M)) - 1);
}
