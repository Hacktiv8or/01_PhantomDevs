#include "SPIFFS.h"
#include "camera_config.cpp"
#include "ObjDetection1_inferencing.h"


#define BYTES_PER_PIXEL 3
#define FRAMEBUFFER_SIZE (EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * 3)


static int get_image_data(size_t offset, size_t length, float* out);
void bilinear_resize_rgb(const uint8_t* src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h);
void inference_task(void *pvParameters);

static uint8_t* framebuffer = nullptr;
static ei_impulse_result_t result = { 0 };
static signal_t phantom_signal = { .get_data = nullptr, .total_length = FRAMEBUFFER_SIZE };

void setup() {
    // Serial chaalu kiya jaa rha hai
    Serial.begin(115200); delay(500);

    // framebuffer allocation
    framebuffer = (uint8_t*)malloc(CAM_BUFFER_SIZE);
    if (framebuffer == NULL) {
        Serial.println("⁉️ framebuffer ko sthaan prapt nahi ho paaya ‼️");
        while (true) delay(1000);
    }

    // camera initialization
    esp_err_t cam_init_status = esp_camera_init(&camera_config);
    if (cam_init_status != ESP_OK) {
        Serial.println("⁉️ Camera prarambh nahi ho paaya ‼️");
        while (true) delay (1000);
    }

    // OV3660 special setup
    sensor_t* cam_sensor = esp_camera_sensor_get();
    if (cam_sensor->id.PID == OV3660_PID) {
        cam_sensor->set_vflip(cam_sensor, 1);
        cam_sensor->set_hmirror(cam_sensor, 1);
        cam_sensor->set_brightness(cam_sensor, 1);
        cam_sensor->set_saturation(cam_sensor, 1);
    }
}

void loop() {
    // capture frame
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("⁉️ Camera se frame nahi le sake ‼️");
        delay(1000); return;
    }

    // convert to rgb888 (if needed)
    bool converted = fmt2rgb888(fb->buf, fb->len, CAM_PIX_FORMAT, framebuffer);
    if (!converted) {
        Serial.println("⁉️ frame rgb888 me parivartit nahi ho paaya ‼️");
        delay(1000); return;
    }

    // save dimensions
    uint16_t source_width = fb->width;
    uint16_t source_height = fb->height;

    // return the frame buffer
    esp_camera_fb_return(fb);

    // resize frame buffer
    bilinear_resize_rgb(
        framebuffer, source_width,
        source_height, framebuffer,
        EI_CLASSIFIER_INPUT_WIDTH,
        EI_CLASSIFIER_INPUT_HEIGHT
    );

    // return the frame buffer
    esp_camera_fb_return(fb);

    phantom_signal.get_data = &get_image_data;
    EI_IMPULSE_ERROR err = run_classifier(&phantom_signal, &result, false);
    if (err != EI_IMPULSE_OK) {
        Serial.println("⁉️  Object Detection Model run nahi ho paaya ‼️");
        return;
    }

    Serial.printf(
        "ℹ️ Predictions [ DSP: %dms | Classification: ❕%dms❕ | Anomaly: %dms ] ~\n",
        result.timing.dsp, result.timing.classification, result.timing.anomaly
    );

    for (uint8_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value < DETECTION_THRESHOLD) continue;
        Serial.printf(
            "\t👉 %s - %.0f%% - [ pos: (%u, %u) | size: (%u, %u) ]\n",
            bb.label, round(bb.value*100), bb.x, bb.y, bb.width, bb.height
        ); if (i+1 == result.bounding_boxes_count) Serial.println("");
    }
}

static int get_image_data(size_t offset, size_t length, float *out_ptr) {
    size_t pixel_ix = offset * 3;
    size_t pixels_left = length;
    size_t out_ptr_ix = 0;

    while (pixels_left != 0) {
        out_ptr[out_ptr_ix] = (
            (framebuffer[pixel_ix + 2] << 16)
            + (framebuffer[pixel_ix + 1] << 8)
            + framebuffer[pixel_ix]
        );

        out_ptr_ix++;
        pixel_ix += 3;
        pixels_left--;
    }; return 0;
}

void bilinear_resize_rgb(const uint8_t *src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h) {
    float x_ratio = static_cast<float>(src_w - 1) / dst_w;
    float y_ratio = static_cast<float>(src_h - 1) / dst_h;

    for (int y = 0; y < dst_h; ++y) {
        float fy = y * y_ratio;
        int sy = static_cast<int>(fy);
        float fy_frac = fy - sy;

        for (int x = 0; x < dst_w; ++x) {
            float fx = x * x_ratio;
            int sx = static_cast<int>(fx);
            float fx_frac = fx - sx;

            for (int c = 0; c < 3; ++c) {
                uint8_t p00 = src[(sy * src_w + sx) * 3 + c];
                uint8_t p10 = src[(sy * src_w + sx + 1) * 3 + c];
                uint8_t p01 = src[((sy + 1) * src_w + sx) * 3 + c];
                uint8_t p11 = src[((sy + 1) * src_w + sx + 1) * 3 + c];

                float top = p00 * (1 - fx_frac) + p10 * fx_frac;
                float bottom = p01 * (1 - fx_frac) + p11 * fx_frac;
                dst[(y * dst_w + x) * 3 + c] = static_cast<uint8_t>(top * (1 - fy_frac) + bottom * fy_frac);
            }
        }
    }
}