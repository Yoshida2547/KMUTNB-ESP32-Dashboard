#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
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

static lv_color_t *draw_buf = nullptr;
static lv_display_t *display = nullptr;
static lv_indev_t *indev = nullptr;
static lv_obj_t *status_label = nullptr;
static lv_obj_t *spinner_obj = nullptr;
static lv_obj_t *benchmark_label = nullptr;
static uint32_t last_stats_ms = 0;
static uint32_t display_width = 0;
static uint32_t display_height = 0;
static uint32_t last_tick_ms = 0;
static uint32_t flush_count = 0;
static uint64_t flush_time_sum_us = 0;
static uint32_t last_flush_area_w = 0;
static uint32_t last_flush_area_h = 0;

void benchmark_timer_cb(lv_timer_t *timer) {
    LV_UNUSED(timer);

    const uint32_t elapsed_ms = millis() - last_stats_ms;
    const uint32_t flush_fps_x10 = (elapsed_ms > 0U) ? (flush_count * 10000UL) / elapsed_ms : 0U;
    const uint32_t avg_flush_us = (flush_count > 0U) ? static_cast<uint32_t>(flush_time_sum_us / flush_count) : 0U;

    lv_label_set_text_fmt(
        benchmark_label,
        "flush %lu.%lu fps\navg %lu us\narea %lux%lu",
        static_cast<unsigned long>(flush_fps_x10 / 10U),
        static_cast<unsigned long>(flush_fps_x10 % 10U),
        static_cast<unsigned long>(avg_flush_us),
        static_cast<unsigned long>(last_flush_area_w),
        static_cast<unsigned long>(last_flush_area_h));

    Serial.printf(
        "[benchmark] flush=%lu.%lu fps avg_flush=%lu us last_area=%lux%lu\n",
        static_cast<unsigned long>(flush_fps_x10 / 10U),
        static_cast<unsigned long>(flush_fps_x10 % 10U),
        static_cast<unsigned long>(avg_flush_us),
        static_cast<unsigned long>(last_flush_area_w),
        static_cast<unsigned long>(last_flush_area_h));

    flush_count = 0;
    flush_time_sum_us = 0;
    last_stats_ms = millis();
}

void log_cb(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.print(buf);
}

void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = (area->x2 - area->x1 + 1);
    const uint32_t h = (area->y2 - area->y1 + 1);
    const uint32_t flush_start_us = micros();

    static int flush_log_count = 0;
    if (flush_log_count < 8 || (flush_log_count % 10) == 0) {
        Serial.printf("[flush_cb] area x1=%d y1=%d x2=%d y2=%d w=%u h=%u\n", area->x1, area->y1, area->x2, area->y2, w, h);
    }
    flush_log_count++;
    last_flush_area_w = w;
    last_flush_area_h = h;

    gfx->draw24bitRGBBitmap(area->x1, area->y1, px_map, w, h);
    flush_time_sum_us += static_cast<uint64_t>(micros() - flush_start_us);
    ::flush_count++;

    lv_display_flush_ready(disp);
}

void touch_read_cb(lv_indev_t *indev_handle, lv_indev_data_t *data) {
    LV_UNUSED(indev_handle);
    data->state = LV_INDEV_STATE_RELEASED;
}

void build_ui() {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *panel = lv_obj_create(screen);
    lv_obj_set_size(panel, LV_PCT(100), LV_PCT(100));
    lv_obj_center(panel);
    lv_obj_set_style_radius(panel, 0, 0);
    lv_obj_set_style_border_width(panel, 8, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x00AEEF), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(panel);
    lv_label_set_text(title, "LVGL 18BIT OK");
    lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 18);

    status_label = lv_label_create(panel);
    lv_label_set_text(status_label, "Arduino_GFX 18-bit");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x000000), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 68);

    spinner_obj = lv_spinner_create(panel);
    lv_obj_set_size(spinner_obj, 72, 72);
    lv_obj_align(spinner_obj, LV_ALIGN_CENTER, 0, -10);
    lv_spinner_set_anim_params(spinner_obj, 1200, 60);

    benchmark_label = lv_label_create(panel);
    lv_label_set_text(benchmark_label, "loop 0.0 fps\nflush 0 avg 0 us\narea 0x0");
    lv_obj_set_style_text_color(benchmark_label, lv_color_hex(0x000000), 0);
    lv_obj_align(benchmark_label, LV_ALIGN_CENTER, 0, 44);

    lv_obj_t *hint = lv_label_create(panel);
    lv_label_set_text(hint, "If this appears, the UI works.");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x000000), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);

    lv_obj_t *bar = lv_bar_create(panel);
    lv_obj_set_size(bar, 260, 20);
    lv_obj_align(bar, LV_ALIGN_BOTTOM_MID, 0, -48);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, 65, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x000000), LV_PART_INDICATOR);

    lv_screen_load(screen);
    lv_obj_invalidate(screen);
    lv_refr_now(display);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);

    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    Serial.println("[new_library_tester_lvgl] calling gfx->begin()");
    if (!gfx->begin()) {
        Serial.println("[new_library_tester_lvgl] Arduino_GFX init failed");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("[new_library_tester_lvgl] gfx->begin() succeeded");
    Serial.println("[new_library_tester_lvgl] testing screen colors without extra bus init...");

    // Keep each color visible long enough to confirm the panel is actually responding.
    Serial.println("[new_library_tester_lvgl] fill RED");
    gfx->fillScreen(RED);
    delay(1000);

    Serial.println("[new_library_tester_lvgl] fill GREEN");
    gfx->fillScreen(GREEN);
    delay(1000);

    Serial.println("[new_library_tester_lvgl] fill BLUE");
    gfx->fillScreen(BLUE);
    delay(1000);

    Serial.println("[new_library_tester_lvgl] fill BLACK");
    gfx->fillScreen(BLACK);

    display_width = gfx->width();
    display_height = gfx->height();
    Serial.printf("[new_library_tester_lvgl] display size: %u x %u\n", display_width, display_height);

    lv_init();
    lv_log_register_print_cb(log_cb);

    const size_t draw_buf_height = 20;
    const size_t draw_buf_size_bytes = static_cast<size_t>(display_width) * draw_buf_height * sizeof(lv_color_t);
    Serial.printf("[new_library_tester_lvgl] allocating draw buffer: %u bytes (%u lines)\n", static_cast<unsigned>(draw_buf_size_bytes), static_cast<unsigned>(draw_buf_height));
    draw_buf = static_cast<lv_color_t *>(heap_caps_malloc(draw_buf_size_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!draw_buf) {
        draw_buf = static_cast<lv_color_t *>(malloc(draw_buf_size_bytes));
    }
    if (!draw_buf) {
        Serial.println("[new_library_tester_lvgl] draw buffer allocation failed");
        while (true) {
            delay(1000);
        }
    }

    display = lv_display_create(display_width, display_height);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB888);
    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_buffers(display, draw_buf, nullptr, draw_buf_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(display);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    build_ui();
    lv_timer_create(benchmark_timer_cb, 250, nullptr);

    last_stats_ms = millis();
    last_tick_ms = last_stats_ms;
    Serial.println("[new_library_tester_lvgl] initialized");
}

void loop() {
    const uint32_t now_ms = millis();
    const uint32_t elapsed_ms = now_ms - last_tick_ms;

    if (elapsed_ms > 0U) {
        lv_tick_inc(elapsed_ms);
        last_tick_ms = now_ms;
    }
    lv_timer_handler();

    if (now_ms - last_stats_ms >= 1000U) {
        last_stats_ms = now_ms;
    }

    delay(1);
}
