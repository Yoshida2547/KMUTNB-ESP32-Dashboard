#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <lvgl.h>

namespace {

constexpr int8_t PIN_MISO = 47;
constexpr int8_t PIN_MOSI = 11;
constexpr int8_t PIN_SCLK = 10;
constexpr int8_t PIN_CS = 45;
constexpr int8_t PIN_DC = 7;
constexpr int8_t PIN_RST = 46;
constexpr int8_t PIN_BL = 16;

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCLK, PIN_MOSI, PIN_MISO);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST, 1, false);

static uint8_t *draw_buf = nullptr;
static lv_display_t *display = nullptr;
static lv_indev_t *indev = nullptr;
static lv_obj_t *tick_label = nullptr;
static lv_obj_t *fps_label = nullptr;
static lv_obj_t *status_label = nullptr;
static uint32_t tick_count = 0;
static uint32_t last_stats_ms = 0;
static uint32_t frames = 0;
static uint64_t frame_time_sum_us = 0;
static uint32_t display_width = 0;
static uint32_t display_height = 0;

void log_cb(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.print(buf);
}

void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = (area->x2 - area->x1 + 1);
    const uint32_t h = (area->y2 - area->y1 + 1);

    gfx->startWrite();
    gfx->draw16bitRGBBitmap(area->x1, area->y1, reinterpret_cast<uint16_t *>(px_map), w, h);
    gfx->endWrite();

    lv_display_flush_ready(disp);
}

void touch_read_cb(lv_indev_t *indev_handle, lv_indev_data_t *data) {
    LV_UNUSED(indev_handle);
    data->state = LV_INDEV_STATE_RELEASED;
}

void build_ui() {
    lv_obj_t *screen = lv_screen_active();
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x0F172A), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);

    lv_obj_t *title = lv_label_create(screen);
    lv_label_set_text(title, "new_library_tester LVGL");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFDE68A), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 14);

    status_label = lv_label_create(screen);
    lv_label_set_text(status_label, "Arduino_GFX ready");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x93C5FD), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 56);

    tick_label = lv_label_create(screen);
    lv_label_set_text(tick_label, "tick: 0");
    lv_obj_set_style_text_color(tick_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(tick_label, LV_ALIGN_CENTER, 0, -8);

    fps_label = lv_label_create(screen);
    lv_label_set_text(fps_label, "fps: --");
    lv_obj_set_style_text_color(fps_label, lv_color_hex(0x86EFAC), 0);
    lv_obj_align(fps_label, LV_ALIGN_CENTER, 0, 28);

    lv_obj_t *hint = lv_label_create(screen);
    lv_label_set_text(hint, "Display-only LVGL demo");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x7DD3FC), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);

    lv_obj_t *bar = lv_bar_create(screen);
    lv_obj_set_size(bar, 220, 18);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -48);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 65, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x22C55E), LV_PART_INDICATOR);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);

    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    if (!gfx->begin()) {
        Serial.println("[new_library_tester_lvgl] Arduino_GFX init failed");
        while (true) {
            delay(1000);
        }
    }

    bus->begin(60000000);
    gfx->fillScreen(BLACK);

    display_width = gfx->width();
    display_height = gfx->height();

    lv_init();
    lv_log_register_print_cb(log_cb);

    draw_buf = static_cast<uint8_t *>(malloc(display_width * display_height / 10 * (LV_COLOR_DEPTH / 8)));
    if (!draw_buf) {
        Serial.println("[new_library_tester_lvgl] draw buffer allocation failed");
        while (true) {
            delay(1000);
        }
    }

    display = lv_display_create(display_width, display_height);
    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_buffers(display, draw_buf, nullptr, display_width * display_height / 10 * (LV_COLOR_DEPTH / 8), LV_DISPLAY_RENDER_MODE_PARTIAL);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    build_ui();

    last_stats_ms = millis();
    Serial.println("[new_library_tester_lvgl] initialized");
}

void loop() {
    const uint32_t frame_start_us = micros();

    lv_timer_handler();

    const uint32_t frame_time_us = micros() - frame_start_us;
    frame_time_sum_us += frame_time_us;
    frames++;

    if (tick_label) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "tick: %lu", static_cast<unsigned long>(tick_count++));
        lv_label_set_text(tick_label, buffer);
    }

    const uint32_t now_ms = millis();
    if (now_ms - last_stats_ms >= 1000U) {
        const uint32_t fps_x10 = (frames * 10000UL) / (now_ms - last_stats_ms);
        const uint32_t avg_frame_us = (frames > 0) ? (frame_time_sum_us / frames) : 0;

        if (fps_label) {
            char fps_buf[32];
            snprintf(fps_buf, sizeof(fps_buf), "fps: %lu.%lu", fps_x10 / 10U, fps_x10 % 10U);
            lv_label_set_text(fps_label, fps_buf);
        }

        Serial.printf("[lvgl_demo] fps=%lu.%lu avg_frame=%lu us\n", fps_x10 / 10U, fps_x10 % 10U, avg_frame_us);
        frames = 0;
        frame_time_sum_us = 0;
        last_stats_ms = now_ms;
    }

    delay(5);
}
