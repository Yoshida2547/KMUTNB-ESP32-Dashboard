#pragma once

#include <lvgl.h>

namespace KMUTNB {

/**
 * Abstract Screen Interface (Strategy Pattern)
 * Defines contract for all screen implementations
 */
class Screen {
public:
    virtual ~Screen() = default;

    /**
     * Build/initialize the screen UI elements
     */
    virtual void build() = 0;

    /**
     * Update screen data and UI elements
     */
    virtual void update() = 0;

    /**
     * Get the base LVGL screen object
     */
    virtual lv_obj_t* getScreen() const = 0;
};

} // namespace KMUTNB
