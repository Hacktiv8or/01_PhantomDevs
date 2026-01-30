#include <Arduino.h>
#include "esp_camera.h"

#define DEBUG
#define DETECTION_THRESHOLD 0

#define CAM_WIDTH 160
#define CAM_HEIGHT 120
#define CAM_FRAME_SIZE FRAMESIZE_QQVGA
#define CAM_BUFFER_SIZE (CAM_WIDTH * CAM_HEIGHT * 3)

#define USE_CAM_BUFFER
#define CAM_PIX_FORMAT PIXFORMAT_JPEG

#define CAM_JPEG_QUALITY 12
#define CAM_FB_LOCATION CAMERA_FB_IN_DRAM

// #define EI_CLASSIFIER_ALLOCATION_STATIC 1

#define AUDIO_PIN 40 // PWM-capable GPIO
#define PWM_CHANNEL 0
#define PWN_FREQUENCY 30000 // >= 20 kHz
#define PWM_RESOLUTION 8 // 8-bit resolution

#define AUDIO_SAMPLE_RATE 16000 // must match ffmpeg output
#define VOLUME 5 // multiplier