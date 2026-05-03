#pragma once

#include <lvgl.h>
#include "Screen.h"
#include "Sensor.h"
#include "SensorManager.h"

namespace KMUTNB {

/**
 * Main Screen Implementation
 * Inherits from Screen interface to support polymorphic screen types
 */
class MainScreen : public Screen {
private:
    lv_obj_t* screen;
    lv_obj_t* main_label;
    lv_obj_t* touch_cursor;
    lv_obj_t* touch_position_label;
    Sensor* cachedSensor;

public:
    MainScreen();
    virtual ~MainScreen() = default;

    // Override Screen interface methods
    void build() override;
    void update() override;
    lv_obj_t* getScreen() const override { return screen; }
};

} // namespace KMUTNB