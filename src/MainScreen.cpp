#include "MainScreen.h"
#include "DisplayManager.h"
#include "SensorManager.h"
#include "ADXL345Sensor.h"
#include <Arduino.h>

using namespace KMUTNB;

static void event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * obj = lv_event_get_target_obj(e);
    if(code == LV_EVENT_VALUE_CHANGED) {
        char buf[32];
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        LV_LOG_USER("Option: %s", buf);
    }
}

MainScreen::MainScreen() : screen(nullptr), main_label(nullptr), touch_cursor(nullptr), touch_position_label(nullptr), cachedSensor(nullptr) {}

void MainScreen::build() {
    // Create a new screen object
    screen = lv_obj_create(NULL);

    lv_indev_t *indev = DisplayManager::getInstance()->getIndev();
    if (indev) {
        // Touch cursor
        touch_cursor = lv_obj_create(screen);
        lv_obj_set_size(touch_cursor, 10, 10);
        lv_obj_set_style_bg_color(touch_cursor, lv_palette_main(LV_PALETTE_RED), 0);
        lv_obj_set_style_radius(touch_cursor, LV_RADIUS_CIRCLE, 0);
        lv_indev_set_cursor(indev, touch_cursor);
    }

    // Hello Label
    main_label = lv_label_create(screen);
    lv_label_set_text(main_label, "Initializing...");
    lv_obj_align(main_label, LV_ALIGN_BOTTOM_RIGHT, 0, -30);

    touch_position_label = lv_label_create(screen);
    lv_label_set_text(touch_position_label, "");
    lv_obj_align(touch_position_label, LV_ALIGN_BOTTOM_RIGHT, 0, -10);

    lv_obj_t * spinner = lv_spinner_create(screen);
    lv_obj_set_size(spinner, 100, 100);
    lv_obj_center(spinner);
    lv_spinner_set_anim_params(spinner, 10000, 200);

    /*Create a normal drop down list*/
    lv_obj_t * dd = lv_dropdown_create(screen);
    lv_dropdown_set_options(dd, "Apple\n"
                            "Banana\n"
                            "Orange\n"
                            "Cherry\n"
                            "Grape\n"
                            "Raspberry\n"
                            "Melon\n"
                            "Orange\n"
                            "Lemon\n"
                            "Nuts");
    
    lv_obj_align(dd, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_add_event_cb(dd, event_handler, LV_EVENT_ALL, NULL);

    // Cache the ADXL345 sensor reference if available
    SensorManager* sensorManager = SensorManager::getInstance();
    if (sensorManager) {
        cachedSensor = sensorManager->getSensorById(0x53);
        if (!cachedSensor) cachedSensor = sensorManager->getSensorByName("ADXL345");
    }
}

void MainScreen::update() {
    // Use cached sensor reference to avoid repeated lookups
    Sensor* sensor = cachedSensor;

    // Fallback: try lookup if cache is empty
    if (!sensor) {
        SensorManager* sensorManager = SensorManager::getInstance();
        if (sensorManager) {
            sensor = sensorManager->getSensorById(0x53);
            if (!sensor) sensor = sensorManager->getSensorByName("ADXL345");
            cachedSensor = sensor; // cache for future updates
        }
    }

    lv_indev_t *indev = DisplayManager::getInstance()->getIndev();

    if (main_label && sensor) {
        lv_label_set_text(main_label, sensor->getFormattedData());
    }

    if (touch_position_label && indev) {
        lv_point_t point;
        lv_indev_get_point(indev, &point);

        lv_label_set_text_fmt(touch_position_label, "Touch: (%d, %d)", point.x, point.y);
    }
}