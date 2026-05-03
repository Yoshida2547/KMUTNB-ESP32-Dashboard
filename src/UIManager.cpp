#include "UIManager.h"
#include "AppManager.h"
#include <Arduino.h>

using namespace KMUTNB;

UIManager::UIManager() : currentScreen(new MainScreen()), ui_task_handle(nullptr) {}

UIManager::~UIManager() {
    if (ui_task_handle) {
        vTaskDelete(ui_task_handle);
        ui_task_handle = nullptr;
    }
    if (currentScreen) {
        delete currentScreen;
        currentScreen = nullptr;
    }
}

bool UIManager::buildUI() {
    // Let the screen shape itself
    if (!currentScreen) return false;
    currentScreen->build();

    // Load the screen into display
    if (currentScreen->getScreen()) {
        lv_screen_load(currentScreen->getScreen());
        return true;
    } else {
        Serial.println("[UIManager] buildUI failed: screen not created");
        return false;
    }
}

void UIManager::update() {
    // Delegate the data-updates back to the screen
    if (currentScreen) {
        currentScreen->update();
    }
}

void UIManager::startUITask() {
    // Pass 'this' as parameter to access non-static methods
    xTaskCreatePinnedToCore(ui_update_task, "UI_Data_Task", 4096, this, 1, &ui_task_handle, 1);
}

void UIManager::ui_update_task(void *pvParameter) {
    UIManager* uiManager = static_cast<UIManager*>(pvParameter);
    
    while (true) {
        // Wait for 100ms for UI refresh
        vTaskDelay(pdMS_TO_TICKS(100)); 

        // Lock the GUI using RAII guard and execute update
        {
            AppManager::GuiLock lock(AppManager::getInstance());
            uiManager->update();
        }
    }
}

UIManager* UIManager::getInstance() {
    if (!instance) {
        instance = new UIManager();
    }
    return instance;
}
