#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Sensor.h"
#include "ADXL345.h"

namespace KMUTNB {

/**
 * ADXL345 Accelerometer Sensor Implementation
 * Concrete implementation of Sensor interface
 */
class ADXL345Sensor : public Sensor {
private:
    static constexpr uint8_t SENSOR_ID = 0x01;
    static constexpr const char* SENSOR_NAME = "ADXL345";
    static constexpr size_t LOG_BUFFER_SIZE = 100;

    ADXL345 accel;
    bool enabled;
    float x, y, z;
    char formatted_data[64];
    
    struct LogEntry {
        float x, y, z;
        uint32_t timestamp;
    };
    LogEntry log_buffer[LOG_BUFFER_SIZE];
    size_t log_index;
    size_t log_count;

public:
    ADXL345Sensor();

    bool initialize() override;
    void enable() override;
    void disable() override;
    bool isEnabled() const override;
    bool read() override;
    uint8_t getId() const override;
    const char* getName() const override;
    SensorType getType() const override;
    void logData() override;
    void clearLog() override;
    const char* getLatestLog() const override;
    const char* getFormattedData() const override;
};

} // namespace KMUTNB
