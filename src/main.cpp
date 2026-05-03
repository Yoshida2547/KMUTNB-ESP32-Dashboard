#include <Arduino.h>
#include "AppManager.h"
#include "SensorManager.h"
#include "UIManager.h"

using namespace KMUTNB;

// --- Main Setup & Loop ---
void setup()
{
    // Initialize SensorManager with all sensors
    if (!SensorManager::getInstance()->begin()) {
        Serial.println("SensorManager initialization failed");
        while (1) { delay(100); }
    }

    // Start hardware, LVGL and independent UI update tasks centrally
    AppManager::getInstance()->begin();

    Serial.println("System initialized successfully");
}

void loop()
{
    // SensorManager runs in background task (Core 1)
    // UIManager runs in its own task
    // MainScreen pulls data directly from SensorManager
    
    // Optional: Log sensor data periodically
    static uint32_t last_log = 0;
    if (millis() - last_log > 5000) {
        SensorManager::getInstance()->logAllData();
        last_log = millis();
    }

    delay(100);
}