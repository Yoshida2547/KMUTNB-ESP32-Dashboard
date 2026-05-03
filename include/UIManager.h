#pragma once

#include <lvgl.h>
#include "Screen.h"
#include "MainScreen.h"
#include "Sensor.h"
#include "SensorManager.h"

namespace KMUTNB {

class UIManager {
private:
    inline static UIManager* instance = nullptr;
    Screen* currentScreen;
    TaskHandle_t ui_task_handle;

    UIManager(); // Private constructor
    ~UIManager(); // Destructor

    static void ui_update_task(void *pvParameter);

public:
    static UIManager* getInstance();

    bool buildUI();
    void update();
    
    void startUITask();
};

} // namespace KMUTNB
