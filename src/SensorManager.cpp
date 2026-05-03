#include <Arduino.h>
#include <Wire.h>
#include "SensorManager.h"

using namespace KMUTNB;

SensorManager::SensorManager() 
    : sensor_task_handle(nullptr), sensor_mutex(nullptr) {}

SensorManager* SensorManager::getInstance() {
    if (!instance) {
        instance = new SensorManager();
    }
    return instance;
}

void SensorManager::sensorTaskFunction(void* pvParameter) {
    SensorManager* manager = static_cast<SensorManager*>(pvParameter);
    manager->runSensorTask();
}

void SensorManager::runSensorTask() {
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(100));

        xSemaphoreTake(sensor_mutex, portMAX_DELAY);
        for (auto& pair : sensors_by_id) {
            Sensor* sensor = pair.second;
            if (sensor->isEnabled()) {
                if (sensor->read()) {
                    sensor->logData();
                }
            }
        }
        xSemaphoreGive(sensor_mutex);
    }
}

bool SensorManager::begin() {
    sensor_mutex = xSemaphoreCreateMutex();
    if (!sensor_mutex) {
        Serial.println("[SensorManager] Failed to create mutex");
        return false;
    }

    xTaskCreatePinnedToCore(
        sensorTaskFunction,
        "Sensor_Task",
        4096,
        this,
        2,
        &sensor_task_handle,
        1
    );

    return true;
}

void SensorManager::registerSensor(Sensor* sensor) {
    if (!sensor) return;
    // Call virtual methods outside the mutex to avoid holding the lock during polymorphic calls
    uint8_t id = sensor->getId();
    const char* name = sensor->getName();

    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    sensors_by_id[id] = sensor;
    sensors_by_name[name] = id;
    xSemaphoreGive(sensor_mutex);
    Serial.printf("[SensorManager] Registered sensor: %s (ID: %u)\n", name, id);
}

Sensor* SensorManager::getSensorById(uint8_t id) {
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    auto it = sensors_by_id.find(id);
    Sensor* result = (it != sensors_by_id.end()) ? it->second : nullptr;
    xSemaphoreGive(sensor_mutex);
    return result;
}

Sensor* SensorManager::getSensorByName(const char* name) {
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    Sensor* result = nullptr;
    auto it = sensors_by_name.find(name);
    if (it != sensors_by_name.end()) {
        auto it2 = sensors_by_id.find(it->second);
        if (it2 != sensors_by_id.end()) {
            result = it2->second;
        }
    }
    xSemaphoreGive(sensor_mutex);
    return result;
}

void SensorManager::enableSensor(uint8_t id) {
    Sensor* sensor = getSensorById(id);
    if (sensor) sensor->enable();
}

void SensorManager::disableSensor(uint8_t id) {
    Sensor* sensor = getSensorById(id);
    if (sensor) sensor->disable();
}

void SensorManager::logAllData() {
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    for (auto& pair : sensors_by_id) {
        Sensor* sensor = pair.second;
        Serial.println(sensor->getLatestLog());
    }
    xSemaphoreGive(sensor_mutex);
}

SensorManager::~SensorManager() {
    for (auto& pair : sensors_by_id) {
        delete pair.second;
    }
    if (sensor_mutex) vSemaphoreDelete(sensor_mutex);
}
