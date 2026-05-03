#pragma once
#include <Arduino.h>

namespace KMUTNB {

/**
 * Abstract Sensor Interface (Strategy Pattern)
 * Defines contract for all sensor implementations
 */
class Sensor {
public:
    enum class SensorType {
        ACCELEROMETER,
        GYROSCOPE,
        TEMPERATURE,
        HUMIDITY,
        PRESSURE,
        UNKNOWN
    };

    virtual ~Sensor() = default;

    virtual bool initialize() = 0;
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual bool isEnabled() const = 0;
    virtual bool read() = 0;
    virtual uint8_t getId() const = 0;
    virtual const char* getName() const = 0;
    virtual SensorType getType() const = 0;
    virtual void logData() = 0;
    virtual void clearLog() = 0;
    virtual const char* getLatestLog() const = 0;
    virtual const char* getFormattedData() const = 0;
};

} // namespace KMUTNB
