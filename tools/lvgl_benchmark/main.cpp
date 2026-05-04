#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>

#include <demos/lv_demos.h>

namespace {

// Display SPI pins (HSPI)
constexpr int8_t PIN_MISO = 47;
constexpr int8_t PIN_MOSI = 11;
constexpr int8_t PIN_SCLK = 10;
constexpr int8_t PIN_CS = 45;
constexpr int8_t PIN_DC = 7;
constexpr int8_t PIN_RST = 46;
constexpr int8_t PIN_BL = 16;

// Touch SPI pins (separate from display)
constexpr int8_t TOUCH_CS = 17;
constexpr int8_t TOUCH_IRQ = 21;
constexpr int8_t TOUCH_MISO = 5;
constexpr int8_t TOUCH_MOSI = 6;
constexpr int8_t TOUCH_CLK = 4;

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCLK, PIN_MOSI, PIN_MISO);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST, 1, false);

SPIClass vspi = SPIClass(FSPI);
XPT2046_Touchscreen *touch = nullptr;

static lv_color_t *draw_buf = nullptr;
static lv_display_t *display = nullptr;
static lv_indev_t *indev_touchpad = nullptr;
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

void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    LV_UNUSED(indev);

    if (!touch) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    if (touch->touched()) {
        TS_Point p = touch->getPoint();

        int16_t x = map(p.x, 200, 3700, 1, display_width);
        int16_t y = map(p.y, 240, 3800, display_height, 1);

        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;

        Serial.printf("[lvgl_benchmark][touch] raw=(%d,%d) mapped=(%d,%d)\n", p.x, p.y, x, y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
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

    Serial.println("[lvgl_benchmark] initializing XPT2046 touch");
    vspi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    touch = new XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);
    if (touch->begin(vspi)) {
        Serial.println("[lvgl_benchmark] XPT2046 touch initialized");
        touch->setRotation(1);
    } else {
        Serial.println("[lvgl_benchmark] XPT2046 touch init failed");
    }

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

    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read_cb);
    lv_indev_set_display(indev_touchpad, display);

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