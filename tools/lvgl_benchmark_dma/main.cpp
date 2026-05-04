#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <esp_heap_caps.h>
#include <lvgl.h>
#include <XPT2046_Touchscreen.h>

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

// Use DMA-capable databus for faster bulk pixel transfers
Arduino_DataBus *bus = new Arduino_ESP32SPIDMA(PIN_DC, PIN_CS, PIN_SCLK, PIN_MOSI, PIN_MISO);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST, 1, false);

SPIClass vspi = SPIClass(FSPI);
XPT2046_Touchscreen *touch = nullptr;

static lv_color_t *draw_buf = nullptr;
static lv_color_t *draw_buf2 = nullptr;
static lv_display_t *display = nullptr;
static lv_indev_t *indev_touchpad = nullptr;
static uint32_t display_width = 0;
static uint32_t display_height = 0;
static uint32_t last_tick_ms = 0;
static uint32_t frame_count = 0;
static uint32_t fps_report_ms = 0;
static uint16_t hue = 0;

void log_cb(lv_log_level_t level, const char *buf) {
    // Intentionally left blank to avoid costly Serial prints during benchmark.
    (void)level;
    (void)buf;
}

void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = static_cast<uint32_t>(area->x2 - area->x1 + 1);
    const uint32_t h = static_cast<uint32_t>(area->y2 - area->y1 + 1);

    gfx->draw24bitRGBBitmap(area->x1, area->y1, px_map, w, h);
    
    frame_count++;
    const uint32_t now_ms = millis();
    if (now_ms - fps_report_ms >= 1000) {
        Serial.printf("[benchmark_dma] FPS: %lu frames in 1s\n", frame_count);
        frame_count = 0;
        fps_report_ms = now_ms;
    }
    
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
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void build_benchmark_ui() {
    lv_obj_t *screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
    lv_screen_load(screen);
}

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);

    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    Serial.println("[lvgl_benchmark_dma] calling gfx->begin()");
    if (!gfx->begin()) {
        Serial.println("[lvgl_benchmark_dma] Arduino_GFX init failed");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("[lvgl_benchmark_dma] gfx->begin() succeeded");
    gfx->fillScreen(BLACK);

    display_width = gfx->width();
    display_height = gfx->height();
    Serial.printf("[lvgl_benchmark_dma] display size: %u x %u\n", display_width, display_height);

    Serial.println("[lvgl_benchmark_dma] initializing XPT2046 touch");
    vspi.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    touch = new XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);
    if (touch->begin(vspi)) {
        Serial.println("[lvgl_benchmark_dma] XPT2046 touch initialized");
        touch->setRotation(1);
    } else {
        Serial.println("[lvgl_benchmark_dma] XPT2046 touch init failed");
    }

    lv_init();
    lv_log_register_print_cb(log_cb);

    // Use RGB888 with DMA for best performance; allocate 60 lines
    const size_t draw_buf_height = 60;
    const size_t draw_buf_size_bytes = static_cast<size_t>(display_width) * draw_buf_height * sizeof(lv_color_t);
    Serial.printf("[lvgl_benchmark_dma] allocating draw buffer: %u bytes (%u lines)\n",
                  static_cast<unsigned>(draw_buf_size_bytes),
                  static_cast<unsigned>(draw_buf_height));

    draw_buf = static_cast<lv_color_t *>(heap_caps_malloc(draw_buf_size_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!draw_buf) {
        draw_buf = static_cast<lv_color_t *>(malloc(draw_buf_size_bytes));
    }
    if (!draw_buf) {
        Serial.println("[lvgl_benchmark_dma] draw buffer allocation failed");
        while (true) {
            delay(1000);
        }
    }

    draw_buf2 = static_cast<lv_color_t *>(heap_caps_malloc(draw_buf_size_bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
    if (!draw_buf2) {
        draw_buf2 = static_cast<lv_color_t *>(malloc(draw_buf_size_bytes));
    }
    if (!draw_buf2) {
        Serial.println("[lvgl_benchmark_dma] second draw buffer allocation failed");
        while (true) {
            delay(1000);
        }
    }

    display = lv_display_create(display_width, display_height);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB888);
    lv_display_set_flush_cb(display, flush_cb);
    lv_display_set_buffers(display, draw_buf, draw_buf2, draw_buf_size_bytes, LV_DISPLAY_RENDER_MODE_FULL);
    lv_display_set_default(display);

    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read_cb);
    lv_indev_set_display(indev_touchpad, display);

    build_benchmark_ui();

    last_tick_ms = millis();
    fps_report_ms = millis();
    Serial.println("[lvgl_benchmark_dma] initialized - color cycle benchmark started");
}

void loop() {
    const uint32_t now_ms = millis();
    const uint32_t elapsed_ms = now_ms - last_tick_ms;

    if (elapsed_ms > 0U) {
        lv_tick_inc(elapsed_ms);
        last_tick_ms = now_ms;
    }

    // Generate a simple color-cycling pattern to stress the display system
    lv_obj_t *screen = lv_scr_act();
    hue += 2;
    
    for (uint16_t y = 0; y < display_height; y += 40) {
        for (uint16_t x = 0; x < display_width; x += 40) {
            uint16_t color = lv_color_to_u32(lv_color_hsv_to_rgb((hue + x + y) % 360, 100, 100));
            lv_obj_t *rect = lv_obj_create(screen);
            lv_obj_set_size(rect, 40, 40);
            lv_obj_set_pos(rect, x, y);
            lv_obj_set_style_bg_color(rect, lv_color_hex(color & 0xFFFFFF), 0);
            lv_obj_set_style_border_width(rect, 0, 0);
        }
    }

    lv_timer_handler();
    delay(1);
}
