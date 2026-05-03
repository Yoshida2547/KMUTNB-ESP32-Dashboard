#include <Arduino.h>
#include <Wire.h>
#include "ADXL345Sensor.h"
#include <cstdio>

using namespace KMUTNB;

ADXL345Sensor::ADXL345Sensor() 
    : accel(ADXL345_ALT), enabled(false), x(0), y(0), z(0), log_index(0), log_count(0) {
    snprintf(formatted_data, sizeof(formatted_data), "X: 0.00 Y: 0.00 Z: 0.00");
}

bool ADXL345Sensor::initialize() {
    byte deviceID = accel.readDeviceID();
    if (deviceID == 0) {
        Serial.println("[ADXL345] Device ID read failed");
        return false;
    }

    if (!accel.writeRate(ADXL345_RATE_200HZ)) {
        Serial.println("[ADXL345] Write rate failed");
        return false;
    }

    if (!accel.writeRange(ADXL345_RANGE_16G)) {
        Serial.println("[ADXL345] Write range failed");
        return false;
    }

    if (!accel.start()) {
        Serial.println("[ADXL345] Start failed");
        return false;
    }

    Serial.println("[ADXL345] Initialized successfully");
    return true;
}

void ADXL345Sensor::enable() { 
    enabled = true; 
}

void ADXL345Sensor::disable() { 
    enabled = false; 
}

bool ADXL345Sensor::isEnabled() const { 
    return enabled; 
}

bool ADXL345Sensor::read() {
    if (!enabled) return false;
    if (accel.update()) {
        x = accel.getX();
        y = accel.getY();
        z = accel.getZ();
        snprintf(formatted_data, sizeof(formatted_data), "X: %.2f Y: %.2f Z: %.2f", x, y, z);
        return true;
    }
    return false;
}

uint8_t ADXL345Sensor::getId() const { 
    return SENSOR_ID; 
}

const char* ADXL345Sensor::getName() const { 
    return SENSOR_NAME; 
}

Sensor::SensorType ADXL345Sensor::getType() const { 
    return SensorType::ACCELEROMETER; 
}

void ADXL345Sensor::logData() {
    if (log_count < LOG_BUFFER_SIZE) {
        log_count++;
    }
    log_buffer[log_index].x = x;
    log_buffer[log_index].y = y;
    log_buffer[log_index].z = z;
    log_buffer[log_index].timestamp = millis();
    log_index = (log_index + 1) % LOG_BUFFER_SIZE;
}

void ADXL345Sensor::clearLog() {
    log_index = 0;
    log_count = 0;
}

const char* ADXL345Sensor::getLatestLog() const {
    static char log_str[256];
    if (log_count == 0) {
        snprintf(log_str, sizeof(log_str), "No data logged");
        return log_str;
    }
    size_t last_idx = (log_index == 0) ? LOG_BUFFER_SIZE - 1 : log_index - 1;
    snprintf(log_str, sizeof(log_str), 
        "ID:%u Name:%s Entries:%zu Last[X:%.2f Y:%.2f Z:%.2f T:%lu]",
        SENSOR_ID, SENSOR_NAME, log_count,
        log_buffer[last_idx].x, log_buffer[last_idx].y, log_buffer[last_idx].z,
        log_buffer[last_idx].timestamp);
    return log_str;
}

const char* ADXL345Sensor::getFormattedData() const { 
    return formatted_data; 
}
