#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

namespace KMUTNB {

typedef struct {
    TFT_eSPI *tft;
} lv_tft_espi_t;

struct DisplayConfig {
    uint8_t tft_rotation = 1;
    lv_display_rotation_t lv_rotation = LV_DISPLAY_ROTATION_90;
    uint16_t touch_cal_data[5] = { 259, 3601, 190, 3630, 7 };
    int8_t led_pin = 16;
    uint8_t led_on_state = HIGH;
    uint32_t draw_buf_size = (TFT_WIDTH * TFT_HEIGHT / 5 * (LV_COLOR_DEPTH / 8));
};

class DisplayManager {
private:
    inline static DisplayManager* instance = nullptr;

    TFT_eSPI *tft;
    lv_display_t *display;
    uint8_t *draw_buf;
    uint16_t calData[5];
    lv_indev_t *indev;
    bool dma_enabled;
    bool flush_pending;
    lv_display_t *pending_disp;

    DisplayManager();

    static void log_print(lv_log_level_t level, const char *buf);
    static void touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data);
    static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

public:
    static DisplayManager* getInstance();
    
    void init(const DisplayConfig& config = DisplayConfig());
    void processDisplayTransfers();
    lv_indev_t* getIndev() const { return indev; }
    TFT_eSPI* getTFT() const { return tft; }
};

} // namespace KMUTNB