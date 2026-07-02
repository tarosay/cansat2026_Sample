#ifndef _CAMERA_H_
#define _CAMERA_H_ 1

#define FRAME_COUNT 50
#define FPS 5

bool cameraInit();
uint32_t captureFrames(uint32_t frameCount, uint32_t fps, uint32_t* width, uint32_t* height, String& folder);
uint32_t captureFrames(uint32_t* width, uint32_t* height, String& folder);
uint32_t getMaxFolderNumber();

#endif  // _CAMERA_H_