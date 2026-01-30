#pragma once

#include <Arduino.h>
#include "esp_camera.h"

#define CAM_PIX_FORMAT PIXFORMAT_JPEG
#define CAM_FRAME_SIZE FRAMESIZE_QQVGA
#define CAM_JPEG_QUALITY 12
#define CAM_FB_LOCATION CAMERA_FB_IN_DRAM