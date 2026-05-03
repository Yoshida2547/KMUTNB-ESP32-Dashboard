#pragma once

#include <Arduino.h>
#include <FreeRTOS.h>
#include <lvgl.h>
#include "UIManager.h"

namespace KMUTNB {

class AppManager {
private:
    inline static AppManager* instance = nullptr;
    SemaphoreHandle_t gui_mutex;
    TaskHandle_t lvgl_task_handle;
    bool lvgl_initialized;
    
    AppManager();
    ~AppManager();
    static void task_lvgl(void *pvParameter);
    void startLVGLTask();

public:
    // RAII guard for GUI mutex (public so other managers can use it)
    class GuiLock {
    public:
        explicit GuiLock(AppManager* app) : app_(app) { if (app_) app_->lockGUI(); }
        ~GuiLock() { if (app_) app_->unlockGUI(); }
    private:
        AppManager* app_;
    };
    static AppManager* getInstance();
    void begin();

    // Accessor for the UI
    void lockGUI();
    void unlockGUI();
};

} // namespace KMUTNB
