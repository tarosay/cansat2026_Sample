#ifndef _CAMERA_H_
#define _CAMERA_H_ 1

#define FRAME_COUNT 1000
#define FPS 5
#define MY_REBOOT 600 //600枚撮影したら、再起動する 

bool cameraInit();
uint32_t captureFrames(uint32_t frameCount, uint32_t fps, uint32_t* width, uint32_t* height, String& folder);
uint32_t captureFrames(uint32_t* width, uint32_t* height, String& folder);
uint32_t getMaxFolderNumber();

#endif  // _CAMERA_H_