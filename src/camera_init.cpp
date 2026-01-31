#ifndef CAMERA_INIT_CPP
#define CAMERA_INIT_CPP

#include "config.h"

#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     15
#define SIOD_GPIO_NUM     4
#define SIOC_GPIO_NUM     5
#define Y9_GPIO_NUM       16
#define Y8_GPIO_NUM       17
#define Y7_GPIO_NUM       18
#define Y6_GPIO_NUM       12
#define Y5_GPIO_NUM       10
#define Y4_GPIO_NUM       8
#define Y3_GPIO_NUM       9
#define Y2_GPIO_NUM       11
#define VSYNC_GPIO_NUM    6
#define HREF_GPIO_NUM     7
#define PCLK_GPIO_NUM     13

static camera_config_t camera_config = {
    .pin_pwdn  = PWDN_GPIO_NUM,
    .pin_reset = RESET_GPIO_NUM,
    .pin_xclk  = XCLK_GPIO_NUM,
    .pin_sscb_sda = SIOD_GPIO_NUM,
    .pin_sscb_scl = SIOC_GPIO_NUM,
    .pin_d7    = Y9_GPIO_NUM,
    .pin_d6    = Y8_GPIO_NUM,
    .pin_d5    = Y7_GPIO_NUM,
    .pin_d4    = Y6_GPIO_NUM,
    .pin_d3    = Y5_GPIO_NUM,
    .pin_d2    = Y4_GPIO_NUM,
    .pin_d1    = Y3_GPIO_NUM,
    .pin_d0    = Y2_GPIO_NUM,
    .pin_vsync = VSYNC_GPIO_NUM,
    .pin_href  = HREF_GPIO_NUM,
    .pin_pclk  = PCLK_GPIO_NUM,

    .xclk_freq_hz = 20000000,
    .ledc_timer   = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = CAM_PIX_FORMAT,     // JPEG is fastest/smallest
    .frame_size   = CAM_FRAME_SIZE,   // we'll resize to 96x96 anyway
    .jpeg_quality = CAM_JPEG_QUALITY,
    .fb_count     = 1,
    .fb_location  = CAM_FB_LOCATION,
    .grab_mode    = CAMERA_GRAB_WHEN_EMPTY
};

#endif