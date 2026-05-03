#include "StorageManager.h"
#include <Arduino.h>
#include <LittleFS.h>

using namespace KMUTNB;

void StorageManager::initLittleFS() {
    if (!LittleFS.begin()) {
        Serial.println("Error: Failed to mount LittleFS");
        return;
    }

    File file = LittleFS.open("/text.txt", "r");
    if (!file) {
        Serial.println("Warning: Failed to open /text.txt for reading");
        return;
    }

    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}