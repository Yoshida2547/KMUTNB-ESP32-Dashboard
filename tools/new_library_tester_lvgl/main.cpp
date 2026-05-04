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

Arduino_DataBus *bus = new Arduino_ESP32SPI(PIN_DC, PIN_CS, PIN_SCLK, PIN_MOSI, PIN_MISO);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST, 1, false);

XPT2046_Touchscreen *touch = nullptr;

static lv_color_t *draw_buf = nullptr;
static lv_display_t *display = nullptr;
static lv_indev_t *indev_touchpad = nullptr;
static lv_obj_t *status_label = nullptr;
static uint32_t display_width = 0;
static uint32_t display_height = 0;
static uint32_t last_tick_ms = 0;

void log_cb(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.print(buf);
}

void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const uint32_t w = (area->x2 - area->x1 + 1);
    const uint32_t h = (area->y2 - area->y1 + 1);

    gfx->draw24bitRGBBitmap(area->x1, area->y1, px_map, w, h);

    lv_display_flush_ready(disp);
}

void touchpad_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    if (!touch) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    if (touch->touched()) {
        TS_Point p = touch->getPoint();
        data->point.x = p.x;
        data->point.y = p.y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
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
    lv_label_set_text(status_label, "Arduino_GFX 18-bit ready");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x000000), 0);
    lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 68);

    lv_obj_t *hint = lv_label_create(panel);
    lv_label_set_text(hint, "If this appears, LVGL and the display are working.");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x000000), 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -18);

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
    gfx->fillScreen(BLACK);

    display_width = gfx->width();
    display_height = gfx->height();
    Serial.printf("[new_library_tester_lvgl] display size: %u x %u\n", display_width, display_height);

    // Initialize touch screen
    Serial.println("[new_library_tester_lvgl] initializing XPT2046 touch");
    touch = new XPT2046_Touchscreen(TOUCH_CS, TOUCH_IRQ);
    if (touch->begin()) {
        Serial.println("[new_library_tester_lvgl] XPT2046 touch initialized");
        touch->setRotation(1);
    } else {
        Serial.println("[new_library_tester_lvgl] XPT2046 touch init failed");
    }

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

    // Register touch input device
    indev_touchpad = lv_indev_create();
    lv_indev_set_type(indev_touchpad, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_touchpad, touchpad_read_cb);
    lv_indev_set_display(indev_touchpad, display);

    build_ui();

    last_tick_ms = millis();
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

    delay(1);
}
