#include "MainScreen.h"
#include "DisplayManager.h"
#include "SensorManager.h"
#include "ADXL345Sensor.h"
#include <Arduino.h>

using namespace KMUTNB;

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
    lv_obj_align(main_label, LV_ALIGN_CENTER, 0, 0);

    touch_position_label = lv_label_create(screen);
    lv_label_set_text(touch_position_label, "");
    lv_obj_align_to(touch_position_label, screen, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);

    lv_obj_set_style_text_align(touch_position_label, LV_TEXT_ALIGN_CENTER, 0);


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