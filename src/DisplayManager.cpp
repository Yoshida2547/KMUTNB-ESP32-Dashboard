#include "DisplayManager.h"

using namespace KMUTNB;

DisplayManager::DisplayManager()
    : tft(nullptr),
      display(nullptr),
      draw_buf(nullptr),
      indev(nullptr),
      dma_enabled(false),
      flush_pending(false),
      pending_disp(nullptr) {
    // Wait for init() to set calibration data
}

DisplayManager* DisplayManager::getInstance() {
    if (!instance) {
        instance = new DisplayManager();
    }
    return instance;
}

void DisplayManager::log_print(lv_log_level_t level, const char *buf) {
    LV_UNUSED(level);
    Serial.print(buf);
}

void DisplayManager::touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    if (!instance || !instance->tft) return;

    uint16_t x, y;
    bool pressed = instance->tft->getTouch(&x, &y);

    if (pressed) {
        data->point.x = y;
        data->point.y = x;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void DisplayManager::flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    if (!instance || !instance->tft) {
        lv_display_flush_ready(disp);
        return;
    }

    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    if (instance->dma_enabled) {
        // LVGL issues one flush at a time; keep it non-blocking and complete in processDisplayTransfers().
        instance->tft->startWrite();
#if !defined(SPI_18BIT_DRIVER)
        instance->tft->pushImageDMA(area->x1, area->y1, w, h, reinterpret_cast<uint16_t *>(px_map));
        instance->flush_pending = true;
        instance->pending_disp = disp;
        return;
#endif
    }

    instance->tft->startWrite();
    instance->tft->setAddrWindow(area->x1, area->y1, w, h);
    instance->tft->pushColors(reinterpret_cast<uint16_t *>(px_map), w * h, true);
    instance->tft->endWrite();
    lv_display_flush_ready(disp);
}

void DisplayManager::init(const DisplayConfig& config) {
    lv_init();
    lv_log_register_print_cb(DisplayManager::log_print);

    // Copy calibration data from config
    for (int i = 0; i < 5; i++) {
        calData[i] = config.touch_cal_data[i];
    }

    draw_buf = new uint8_t[config.draw_buf_size];
    if (!draw_buf) {
        Serial.println("[DisplayManager] Failed to allocate draw buffer");
        return;
    }
    display = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, config.draw_buf_size);
    lv_display_set_rotation(display, config.lv_rotation);
    lv_display_set_flush_cb(display, flush_cb);
    
    lv_tft_espi_t *dsc = (lv_tft_espi_t *)lv_display_get_driver_data(display);
    tft = dsc->tft;
    tft->setRotation(config.tft_rotation);
    tft->setTouch(calData);

#if !defined(SPI_18BIT_DRIVER)
    dma_enabled = tft->initDMA(false);
    if (dma_enabled) {
        Serial.println("[DisplayManager] TFT DMA enabled");
    } else {
        Serial.println("[DisplayManager] TFT DMA unavailable, fallback to blocking flush");
    }
#else
    dma_enabled = false;
    Serial.println("[DisplayManager] TFT driver is in SPI_18BIT mode; DMA flush is not available");
#endif

    LV_LOG_INFO("Display initialized with resolution %dx%d", TFT_WIDTH, TFT_HEIGHT);

    indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_read_cb);

    if (config.led_pin >= 0) {
        pinMode(config.led_pin, OUTPUT);
        digitalWrite(config.led_pin, config.led_on_state);
    }
}

void DisplayManager::processDisplayTransfers() {
    if (!dma_enabled || !flush_pending || !tft) return;

#if !defined(SPI_18BIT_DRIVER)
    if (!tft->dmaBusy()) {
        tft->endWrite();
        if (pending_disp) {
            lv_display_flush_ready(pending_disp);
        }
        pending_disp = nullptr;
        flush_pending = false;
    }
#endif
}
