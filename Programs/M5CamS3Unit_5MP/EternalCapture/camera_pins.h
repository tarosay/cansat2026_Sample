#ifndef _CAMERA_PINS_H_
#define _CAMERA_PINS_H_ 1

//#elif defined(CAMERA_MODEL_M5STACK_CAMS3_UNIT)
// カメラのピン設定 (M5Stack UnitCam S3)
#define PWDN_GPIO_NUM     -1   // PWDN未使用なら -1
#define RESET_GPIO_NUM    21   // RESET未使用なら -1
#define XCLK_GPIO_NUM     11
#define SIOD_GPIO_NUM     17  // SDA
#define SIOC_GPIO_NUM     41  // SCL

#define Y2_GPIO_NUM       6
#define Y3_GPIO_NUM       15
#define Y4_GPIO_NUM       16
#define Y5_GPIO_NUM       7
#define Y6_GPIO_NUM       5
#define Y7_GPIO_NUM       10
#define Y8_GPIO_NUM       4
#define Y9_GPIO_NUM       13

#define VSYNC_GPIO_NUM    42
#define HREF_GPIO_NUM     18
#define PCLK_GPIO_NUM     12


#define LED_GPIO_NUM 14
#endif  // _CAMERA_PINS_H_