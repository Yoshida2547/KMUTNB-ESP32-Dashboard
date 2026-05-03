#include "AppManager.h"
#include "StorageManager.h"
#include "DisplayManager.h"
#include "UIManager.h"
#include "SensorManager.h"
#include "ADXL345Sensor.h"
#include <Wire.h>

using namespace KMUTNB;

AppManager::AppManager() : gui_mutex(nullptr), lvgl_task_handle(nullptr), lvgl_initialized(false) {
}

AppManager* AppManager::getInstance() {
    if (!instance) {
        instance = new AppManager();
    }
    return instance;
}

AppManager::~AppManager() {
    if (lvgl_task_handle) {
        vTaskDelete(lvgl_task_handle);
        lvgl_task_handle = nullptr;
    }
    if (gui_mutex) {
        vSemaphoreDelete(gui_mutex);
        gui_mutex = nullptr;
    }
}

void AppManager::begin() {
    Serial.begin(115200);
    
    // Initialize file system
    StorageManager::initLittleFS();

    // Initialize display
    DisplayManager* displayManager = DisplayManager::getInstance();
    displayManager->init();

    // Initialize sensors
    // Ensure I2C is initialized once centrally
    Wire.begin();
    SensorManager* sensorManager = SensorManager::getInstance();
    sensorManager->begin();
    
    // Create and register ADXL345 accelerometer
    ADXL345Sensor* accel = new ADXL345Sensor();
    if (!accel->initialize()) {
        Serial.println("[AppManager] Failed to initialize ADXL345");
        delete accel;
    } else {
        accel->enable();
        sensorManager->registerSensor(accel);
    }

    // Initialize UI
    UIManager* uiManager = UIManager::getInstance();
    if (!uiManager->buildUI()) {
        Serial.println("[AppManager] UI build failed");
    }

    lvgl_initialized = true;

    gui_mutex = xSemaphoreCreateMutex();

    this->startLVGLTask();
    uiManager->startUITask();        
}

void AppManager::startLVGLTask() {
    if (gui_mutex != NULL) {
        xTaskCreatePinnedToCore(task_lvgl, "LVGL_Task", 12288, this, 10, &lvgl_task_handle, 0);
    } else {
        Serial.println("Error: Failed to create GUI mutex");
    }
}

void AppManager::task_lvgl(void *pvParameter) {

    AppManager *app = static_cast<AppManager*>(pvParameter);

    if (!app) {
        Serial.println("[AppManager] task_lvgl: invalid parameter");
        return;
    }

    // 3. FreeRTOS Task Loop for LVGL Updates
    while (true) {
        lv_tick_inc(5);

        if (xSemaphoreTake(app->gui_mutex, portMAX_DELAY) == pdTRUE) {
            lv_task_handler();
            DisplayManager::getInstance()->processDisplayTransfers();
            xSemaphoreGive(app->gui_mutex);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void AppManager::lockGUI() { xSemaphoreTake(gui_mutex, portMAX_DELAY); }
void AppManager::unlockGUI() { xSemaphoreGive(gui_mutex); }
