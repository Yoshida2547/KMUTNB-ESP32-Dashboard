#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <lvgl.h>

#include <demos/lv_demos.h>

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
static uint32_t display_width = 0;
static uint32_t display_height = 0;
static uint32_t last_tick_ms = 0;

void log_cb(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.print(buf);
}

void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = static_cast<uint32_t>(area->x2 - area->x1 + 1);
    const uint32_t h = static_cast<uint32_t>(area->y2 - area->y1 + 1);

    gfx->draw24bitRGBBitmap(area->x1, area->y1, px_map, w, h);
    lv_display_flush_ready(disp);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);

    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    Serial.println("[lvgl_benchmark] calling gfx->begin()");
    if (!gfx->begin()) {
        Serial.println("[lvgl_benchmark] Arduino_GFX init failed");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("[lvgl_benchmark] gfx->begin() succeeded");
    gfx->fillScreen(BLACK);

    display_width = gfx->width();
    display_height = gfx->height();
    Serial.printf("[lvgl_benchmark] display size: %u x %u\n", display_width, display_height);

    lv_init();
    lv_log_register_print_cb(log_cb);

    const size_t draw_buf_height = 80;
    const size_t draw_buf_size_bytes = static_cast<size_t>(display_width) * draw_buf_height * sizeof(lv_color_t);
    Serial.printf("[lvgl_benchmark] allocating draw buffer: %u bytes (%u lines)\n",
                  static_cast<unsigned>(draw_buf_size_bytes),
                  static_cast<unsigned>(draw_buf_height));

    draw_buf = static_cast<lv_color_t *>(heap_caps_malloc(draw_buf_size_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!draw_buf) {
        draw_buf = static_cast<lv_color_t *>(malloc(draw_buf_size_bytes));
    }
    if (!draw_buf) {
        Serial.println("[lvgl_benchmark] draw buffer allocation failed");
        while (true) {
            delay(1000);
        }
    }

    display = lv_display_create(display_width, display_height);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB888);
    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_buffers(display, draw_buf, nullptr, draw_buf_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_default(display);

    lv_demo_benchmark();

    last_tick_ms = millis();
    Serial.println("[lvgl_benchmark] initialized");
}

void loop() {
    const uint32_t now_ms = millis();
    const uint32_t elapsed_ms = now_ms - last_tick_ms;

    if (elapsed_ms > 0U) {
        lv_tick_inc(elapsed_ms);
        last_tick_ms = now_ms;
    }

    lv_timer_handler();
    delay(1);
}