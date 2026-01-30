#include "esp_task_wdt.h"

#include "SPIFFS.h"
#include "camera_init.cpp"
#include "ObjDetection1_inferencing.h"


#define BYTES_PER_PIXEL 3
#define FRAMEBUFFER_SIZE (EI_CLASSIFIER_INPUT_WIDTH * EI_CLASSIFIER_INPUT_HEIGHT * 3)


void listSPIFFSFiles();
static int get_image_data(size_t offset, size_t length, float* out);
void bilinear_resize_rgb(const uint8_t* src, int src_w, int src_h, uint8_t *dst, int dst_w, int dst_h);
void inference_task(void *pvParameters);

static uint8_t* framebuffer = nullptr;
static ei_impulse_result_t result = { 0 };
static signal_t phantom_signal = { .get_data = nullptr, .total_length = FRAMEBUFFER_SIZE };
static uint64_t last_beeped = 0;

File audio_file; uint64_t last_sample_micros = 0;
const uint64_t SAMPLE_INTERVAL = (1000000UL / AUDIO_SAMPLE_RATE);

static inline void debug_print(const char* string) {
    #ifdef DEBUG
    Serial.println(string);
    #endif
}


void setup() {
    // Serial chaalu kiya jaa rha hai
    Serial.begin(115200); delay(500);
    debug_print("✅ Serial safaltapoorvak prarambh ho gaya.");

    debug_print("👉 SPIFFS Filesystem mount kiya jaa rha hai ...");
    SPIFFS.begin(); listSPIFFSFiles();
    debug_print("✅ SPIFFS Filesystem safaltapoorvak mount ho gaya.");

    debug_print("👉 PWM pin par LEDC (Audio) setup kiya jaa rha hai ...");
    ledcSetup(PWM_CHANNEL, PWN_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(AUDIO_PIN, PWM_CHANNEL);
    debug_print("✅ PWM pin par LEDC (Audio) setup safaltapoorvak ho gaya.");

    // framebuffer allocation
    debug_print("👉 framebuffer ko sthaan prapt karvaya jaa rha hai ...");
    #ifdef USE_CAM_BUFFER
    framebuffer = (uint8_t*)malloc(CAM_BUFFER_SIZE);
    #else
    framebuffer = (uint8_t*)malloc(FRAMEBUFFER_SIZE);
    #endif
    if (framebuffer == NULL) {
        Serial.println("⁉️ framebuffer ko sthaan prapt nahi ho paaya ‼️");
        while (true) delay(1000);
    }
    debug_print("✅ framebuffer ko safaltapoorvak sthaan prapt ho gaya.");

    // camera initialization
    debug_print("👉 Camera prarambh kiya jaa rha hai ...");
    esp_err_t cam_init_status = esp_camera_init(&camera_config);
    if (cam_init_status != ESP_OK) {
        Serial.println("⁉️ Camera prarambh nahi ho paaya ‼️");
        while (true) delay (1000);
    }
    debug_print("✅ Camera safaltapoorvak prarambh ho gaya.");

    // OV3660 special setup
    sensor_t* cam_sensor = esp_camera_sensor_get();
    if (cam_sensor->id.PID == OV3660_PID) {
        debug_print("👉 Camera OV3600 ke liye khaas settings ki jaa rahi hai ...");
        cam_sensor->set_vflip(cam_sensor, 1);
        cam_sensor->set_hmirror(cam_sensor, 1);
        cam_sensor->set_brightness(cam_sensor, 1);
        cam_sensor->set_saturation(cam_sensor, 1);
    }
    debug_print("✅ Camera poori tarah taiyaar ho gaya.");

    debug_print("✅ Setup function safaltapoorvak sampann hua.");
}

void loop() {
    // capture frame
    debug_print("👉 Camera se frame liya jaa rha hai ...");
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("⁉️ Camera se frame nahi le sake ‼️");
        delay(1000); return;
    }
    debug_print("✅ Camera se safaltapoorvak frame le liya.");

    // convert to rgb888 (if needed)
    #ifdef USE_CAM_BUFFER
    debug_print("👉 frame ko rgb888 me parivartit kiya jaa rha hai ...");
    bool converted = fmt2rgb888(fb->buf, fb->len, CAM_PIX_FORMAT, framebuffer);
    if (!converted) {
        Serial.println("⁉️ frame rgb888 me parivartit nahi ho paaya ‼️");
        delay(1000); return;
    }
    debug_print("✅ frame safaltapoorvak rgb888 me parivartit ho gaya.");

    // save dimensions
    uint16_t source_width = fb->width;
    uint16_t source_height = fb->height;

    // return the frame buffer
    debug_print("👉 Camera ko frame buffer vapas diya jaa raha hai ...");
    esp_camera_fb_return(fb);
    debug_print("✅ Camera ko frame buffer vapas de diya.");

    // resize frame buffer
    debug_print("👉 frame ko resize kara jaa rha hai ...");
    bilinear_resize_rgb(
        framebuffer, source_width,
        source_height, framebuffer,
        EI_CLASSIFIER_INPUT_WIDTH,
        EI_CLASSIFIER_INPUT_HEIGHT
    );
    debug_print("✅ frame safaltapoorvak resize ho gaya.");
    #else
    bilinear_resize_rgb(
        fb->buf, fb->width,
        fb->height, framebuffer,
        EI_CLASSIFIER_INPUT_WIDTH,
        EI_CLASSIFIER_INPUT_HEIGHT
    );

    // return the frame buffer
    debug_print("👉 Camera ko frame buffer vapas diya jaa raha hai ...");
    esp_camera_fb_return(fb);
    debug_print("✅ Camera ko frame buffer vapas de diya.");
    #endif

    debug_print("👉 Object Detection Model run kiya jaa rha hai ...");
    phantom_signal.get_data = &get_image_data;
    esp_task_wdt_reset();
    EI_IMPULSE_ERROR err = run_classifier(&phantom_signal, &result, false);
    esp_task_wdt_reset();
    if (err != EI_IMPULSE_OK) {
        Serial.printf("⁉️  Object Detection Model run nahi ho paaya (Err: %d) ‼️\n", err);
        return;
    }
    debug_print("✅ Object Detection safaltapoorvak sampann hua");

    Serial.printf(
        "ℹ️  Predictions [ DSP: %dms | Classification: ❕%dms❕ | Anomaly: %dms ] ~\n",
        result.timing.dsp, result.timing.classification, result.timing.anomaly
    );

    for (uint8_t i = 0; i < result.bounding_boxes_count; i++) {
        ei_impulse_result_bounding_box_t bb = result.bounding_boxes[i];
        if (bb.value < DETECTION_THRESHOLD) continue;
        Serial.printf(
            "\t👉 %s - %.0f%% - [ pos: (%u, %u) | size: (%u, %u) ]\n",
            bb.label, round(bb.value*100), bb.x, bb.y, bb.width, bb.height
        ); if (i+1 == result.bounding_boxes_count) Serial.println("");
        // audio_file = SPIFFS.open("/does_not_exists", "r"); // assigning null file
        if (strcmp(bb.label, "50_INR") == 0) {
            audio_file = SPIFFS.open("/50_INR.raw", "r");
        } else if (strcmp(bb.label, "100_INR") == 0) {
            audio_file = SPIFFS.open("/100_INR.raw", "r");
        } else if (strcmp(bb.label, "person") == 0) {
            audio_file = SPIFFS.open("/PERSON.raw", "r");
        } else { Serial.printf("\t\t⁉️ Unexpected label: \"%s\"\n", bb.label); }
        if (audio_file) {
            ledcWrite(0, 255);
            uint32_t file_size = audio_file.size();
            uint8_t* sample = (uint8_t*)malloc(file_size);
            uint64_t now;
            audio_file.read(sample, audio_file.size());
            for (size_t idx = 0; idx < file_size; ) {
                now = micros();
                if (now - last_sample_micros >= SAMPLE_INTERVAL) {
                    last_sample_micros = now;
                    // ledcWrite(ANNEL, constrain((int)((sample - 128) * VOLUME + 128), 0, 255));
                    ledcWrite(PWM_CHANNEL, (sample[idx] - 128) * VOLUME + 128);
                    idx++;
                }
            }; audio_file.close();
        }
    }

    if (millis() - last_beeped > BEEP_DELAY) {
        Serial.println("Beep");
        File beep_file = SPIFFS.open("/beep.raw", "r");
        size_t beep_size = beep_file.size(); uint32_t now;
        uint8_t* beep_buffer = (uint8_t*)malloc(beep_size);
        beep_file.read(beep_buffer, beep_size);
        for (size_t idx = 0; idx < beep_size; ) {
            now = micros();
            if (now - last_sample_micros >= SAMPLE_INTERVAL) {
                last_sample_micros = now;
                ledcWrite(PWM_CHANNEL, beep_buffer[idx]);
                idx++;
            }
        }; last_beeped = millis();
    }

    debug_print("\n---------------------------------\n");
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

void listSPIFFSFiles() {
    Serial.println("_---SPIFFS Files-----");
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
        Serial.println(file.name());
        file = root.openNextFile();
    }; Serial.println("----------------------");
}