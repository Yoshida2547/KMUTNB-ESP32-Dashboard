//...
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <FreeRTOS.h>
#include <lvgl.h>

#include "lv_tc.h"
#include "lv_tc_screen.h"
#include "esp_nvs_tc.h"

#define TFT_LED_PIN 16

typedef struct {
    TFT_eSPI * tft;
} lv_tft_espi_t;

// LVGL draw into this buffer, 1/10 screen size usually works well. The size is
// in bytes.
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
static uint8_t *draw_buf;
static TFT_eSPI *tft;

SemaphoreHandle_t gui_mutex = xSemaphoreCreateMutex();

void your_start_application() {
    // Your application code here - setup only, no loops!
    
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "Hello, LVGL!");
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

void your_tc_finish_cb(
        lv_event_t *event
    ) {
    Serial.println("Calibration finished! Starting application...");
    your_start_application();
}

void your_indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
  uint16_t x, y;
  bool pressed = tft->getTouch(&x, &y);

  if(pressed) {
    data->point.x = x;  // Pass raw coordinates - calibration system handles transformation
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
    Serial.printf("Touch: x=%d, y=%d (raw coords)\n", x, y);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void init() {

    lv_init();

    draw_buf = new uint8_t[DRAW_BUF_SIZE];

    lv_display_t *display = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, DRAW_BUF_SIZE);
    lv_display_set_rotation(display, LV_DISPLAY_ROTATION_90);

    lv_tft_espi_t *dsc = (lv_tft_espi_t *)lv_display_get_driver_data(display);
    tft = dsc->tft;
    tft->setRotation(1); // Set rotation to landscape mode

    pinMode(TFT_LED_PIN, OUTPUT);
    digitalWrite(TFT_LED_PIN, HIGH);

    //...

    /*
        Create and set up the input device
    */
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, your_indev_read_cb);

    /*
        Initialize the calibrated touch device.
        Uses its user_data field. DO NOT OVERRIDE
    */
    lv_tc_indev_init(indev);

    /*
        If using NVS:
        Register a calibration coefficients save callback.
    */
    lv_tc_register_coeff_save_cb(esp_nvs_tc_coeff_save_cb);

    /*
        Create the calibration screen.
    */
    lv_obj_t *tCScreen = lv_tc_screen_create();

    /*
        Register a callback for when the calibration finishes.
        An LV_EVENT_READY event is triggered.
    */
    lv_obj_add_event_cb(tCScreen, your_tc_finish_cb, LV_EVENT_READY, NULL);
    Serial.println("Event callback registered for LV_EVENT_READY");

    /*
        If using NVS:
        Init NVS and check for existing calibration data.
    */
    if(esp_nvs_tc_coeff_init()) {
        /*
            Data exists: proceed with the normal application without
            showing the calibration screen
        */
        Serial.println("Calibration data found in NVS. Starting application...");
        your_start_application();
    } else {
        /*
            There is no data: load the calibration screen, perform the calibration
        */
        Serial.println("No calibration data found in NVS. Starting calibration...");
        lv_disp_load_scr(tCScreen);
        lv_tc_screen_start(tCScreen);
    }
}

void setup() {
    Serial.begin(115200);
    init();
}

void loop() {
     /* Nothing to do here - everything is handled in the calibration screen's event callbacks and the main application function */
    // Handle LVGL ticks and task handler
    lv_tick_inc(5);
    if (xSemaphoreTake(gui_mutex, portMAX_DELAY) == pdTRUE) {
        lv_task_handler();
        xSemaphoreGive(gui_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(5));
}

//...