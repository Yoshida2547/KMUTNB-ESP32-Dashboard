#pragma once
#include "Sensor.h"
#include <map>
#include <FreeRTOS.h>

namespace KMUTNB {

class SensorManager {
private:
    inline static SensorManager* instance = nullptr;
    std::map<uint8_t, Sensor*> sensors_by_id;
    std::map<const char*, uint8_t> sensors_by_name;
    TaskHandle_t sensor_task_handle;
    SemaphoreHandle_t sensor_mutex;

    SensorManager();
    static void sensorTaskFunction(void* pvParameter);
    void runSensorTask();

public:
    static SensorManager* getInstance();
    bool begin();
    void registerSensor(Sensor* sensor);
    Sensor* getSensorById(uint8_t id);
    Sensor* getSensorByName(const char* name);
    void enableSensor(uint8_t id);
    void disableSensor(uint8_t id);
    void logAllData();
    ~SensorManager();
};

} // namespace KMUTNB
