# Sensor Implementation Template

Use this template to quickly implement a new sensor. Replace `XyzSensor` with your sensor name throughout.

---

## Step 1: Create Header File

**File**: `include/XyzSensor.h`

```cpp
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include "Sensor.h"

namespace KMUTNB {

/**
 * XyzSensor Implementation
 * Concrete implementation of Sensor interface for [sensor model/purpose]
 * 
 * I2C Address: 0xXX
 * Datasheet: [link to datasheet]
 */
class XyzSensor : public Sensor {
private:
    static constexpr uint8_t SENSOR_ID = 0xXX;           // Unique ID for this sensor
    static constexpr const char* SENSOR_NAME = "Xyz";    // Human-readable name
    static constexpr size_t LOG_BUFFER_SIZE = 100;

    // Hardware interface
    uint8_t i2c_address;
    bool enabled;

    // Sensor data
    float value1, value2, value3;  // Replace with actual sensor readings
    char formatted_data[64];

    // Logging
    struct LogEntry {
        float value1, value2, value3;
        uint32_t timestamp;
    };
    LogEntry log_buffer[LOG_BUFFER_SIZE];
    size_t log_index;
    size_t log_count;

public:
    XyzSensor(uint8_t address = 0xXX);  // Constructor with I2C address
    virtual ~XyzSensor() = default;

    // Implement all Sensor pure virtual methods
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

private:
    // Helper methods for your sensor
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    bool readMultipleRegisters(uint8_t start_reg, uint8_t* buffer, size_t length);
};

} // namespace KMUTNB
```

---

## Step 2: Create Implementation File

**File**: `src/XyzSensor.cpp`

```cpp
#include <Arduino.h>
#include <Wire.h>
#include "XyzSensor.h"
#include <cstdio>

using namespace KMUTNB;

// Constructor
XyzSensor::XyzSensor(uint8_t address) 
    : i2c_address(address), enabled(false), 
      value1(0.0f), value2(0.0f), value3(0.0f),
      log_index(0), log_count(0) {
    snprintf(formatted_data, sizeof(formatted_data), "Value1: 0.00 Value2: 0.00 Value3: 0.00");
}

// Initialize sensor (called once at startup)
bool XyzSensor::initialize() {
    // Ensure Wire is initialized
    Wire.begin();
    
    // Try to detect sensor on I2C bus
    Wire.beginTransmission(i2c_address);
    if (Wire.endTransmission() != 0) {
        Serial.printf("[XyzSensor] Device not found at 0x%02X\n", i2c_address);
        return false;
    }

    // Read WHO_AM_I or ID register to verify correct device
    uint8_t device_id = readRegister(0x0F);  // Adjust register based on datasheet
    if (device_id != 0xAB) {  // Replace 0xAB with expected device ID
        Serial.printf("[XyzSensor] Invalid device ID: 0x%02X (expected 0xAB)\n", device_id);
        return false;
    }

    // Perform soft reset (if supported)
    // writeRegister(0x1F, 0x80);  // Uncomment if sensor has reset bit
    // vTaskDelay(pdMS_TO_TICKS(100));  // Wait for reset to complete

    // Configure sensor registers
    if (!writeRegister(0x20, 0x47)) {  // Example: control register
        Serial.println("[XyzSensor] Failed to write control register");
        return false;
    }

    if (!writeRegister(0x21, 0x00)) {  // Example: config register
        Serial.println("[XyzSensor] Failed to write config register");
        return false;
    }

    Serial.println("[XyzSensor] Initialized successfully");
    return true;
}

void XyzSensor::enable() {
    enabled = true;
    Serial.printf("[XyzSensor] Enabled\n");
}

void XyzSensor::disable() {
    enabled = false;
    Serial.printf("[XyzSensor] Disabled\n");
}

bool XyzSensor::isEnabled() const {
    return enabled;
}

// Read sensor data (called periodically by SensorManager)
bool XyzSensor::read() {
    if (!enabled) return false;

    // Read raw data from sensor (adjust based on datasheet)
    uint8_t raw_data[6];  // Adjust size based on sensor output
    
    if (!readMultipleRegisters(0x28, raw_data, 6)) {  // Adjust starting register
        Serial.println("[XyzSensor] Failed to read sensor data");
        return false;
    }

    // Convert raw data to physical units
    // Example: 16-bit little-endian to float conversion
    int16_t raw1 = (int16_t)(raw_data[1] << 8 | raw_data[0]);
    int16_t raw2 = (int16_t)(raw_data[3] << 8 | raw_data[2]);
    int16_t raw3 = (int16_t)(raw_data[5] << 8 | raw_data[4]);

    // Scale to physical units (adjust sensitivity factor based on datasheet)
    const float SENSITIVITY = 0.002f;  // Replace with actual sensitivity (LSB/unit)
    value1 = raw1 * SENSITIVITY;
    value2 = raw2 * SENSITIVITY;
    value3 = raw3 * SENSITIVITY;

    // Format data for display
    snprintf(formatted_data, sizeof(formatted_data), 
             "V1: %.2f V2: %.2f V3: %.2f", value1, value2, value3);

    return true;
}

uint8_t XyzSensor::getId() const {
    return SENSOR_ID;
}

const char* XyzSensor::getName() const {
    return SENSOR_NAME;
}

Sensor::SensorType XyzSensor::getType() const {
    // Return appropriate type from enum
    return SensorType::UNKNOWN;  // Replace with actual type (ACCELEROMETER, TEMPERATURE, etc.)
}

void XyzSensor::logData() {
    if (log_count < LOG_BUFFER_SIZE) {
        log_count++;
    }
    
    log_buffer[log_index].value1 = value1;
    log_buffer[log_index].value2 = value2;
    log_buffer[log_index].value3 = value3;
    log_buffer[log_index].timestamp = millis();
    
    log_index = (log_index + 1) % LOG_BUFFER_SIZE;
}

void XyzSensor::clearLog() {
    log_index = 0;
    log_count = 0;
    memset(log_buffer, 0, sizeof(log_buffer));
}

const char* XyzSensor::getLatestLog() const {
    if (log_count == 0) return "No data logged";
    
    static char log_str[128];
    size_t latest = (log_index == 0) ? (LOG_BUFFER_SIZE - 1) : (log_index - 1);
    
    snprintf(log_str, sizeof(log_str), 
             "[%u ms] V1: %.2f V2: %.2f V3: %.2f",
             log_buffer[latest].timestamp,
             log_buffer[latest].value1,
             log_buffer[latest].value2,
             log_buffer[latest].value3);
    
    return log_str;
}

const char* XyzSensor::getFormattedData() const {
    return formatted_data;
}

// Private helper: Write to I2C register
bool XyzSensor::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(i2c_address);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

// Private helper: Read from I2C register
uint8_t XyzSensor::readRegister(uint8_t reg) {
    Wire.beginTransmission(i2c_address);
    Wire.write(reg);
    Wire.endTransmission(false);  // Send repeated start
    
    Wire.requestFrom((int)i2c_address, 1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

// Private helper: Read multiple bytes from I2C
bool XyzSensor::readMultipleRegisters(uint8_t start_reg, uint8_t* buffer, size_t length) {
    Wire.beginTransmission(i2c_address);
    Wire.write(start_reg);
    Wire.endTransmission(false);  // Send repeated start
    
    size_t bytes_read = Wire.requestFrom((int)i2c_address, (int)length);
    if (bytes_read != length) {
        return false;
    }
    
    for (size_t i = 0; i < length; i++) {
        buffer[i] = Wire.read();
    }
    return true;
}
```

---

## Step 3: Register in AppManager

**File**: `src/AppManager.cpp` (in `begin()` method)

```cpp
void AppManager::begin() {
    // ... existing initialization ...

    // Initialize sensors
    SensorManager* sensorManager = SensorManager::getInstance();
    sensorManager->begin();
    
    // Create and register ADXL345
    AcceleroMeter* accel = new AcceleroMeter();
    if (!accel->initialize()) {
        Serial.println("[AppManager] Failed to initialize ADXL345");
        delete accel;
    } else {
        accel->enable();
        sensorManager->registerSensor(accel);
    }

    // ========== ADD YOUR SENSOR HERE ==========
    XyzSensor* xyz = new XyzSensor(0xXX);  // Replace 0xXX with I2C address
    if (!xyz->initialize()) {
        Serial.println("[AppManager] Failed to initialize XyzSensor");
        delete xyz;
    } else {
        xyz->enable();
        sensorManager->registerSensor(xyz);
    }
    // ==========================================

    // ... rest of initialization ...
}
```

---

## Step 4: Display in MainScreen (Optional)

**File**: `src/MainScreen.cpp` (in `update()` method)

```cpp
void MainScreen::update() {
    SensorManager* sensorManager = SensorManager::getInstance();
    
    // Display existing sensors
    Sensor* accel = sensorManager->getSensorByName("ADXL345");
    if (main_label && accel) {
        lv_label_set_text(main_label, accel->getFormattedData());
    }

    // ========== DISPLAY YOUR SENSOR HERE ==========
    Sensor* xyz = sensorManager->getSensorByName("Xyz");
    if (another_label && xyz) {
        lv_label_set_text(another_label, xyz->getFormattedData());
    }
    // =============================================

    // ... rest of update ...
}
```

---

## Checklist

- [ ] Replace all `XyzSensor` with your actual sensor name (PascalCase)
- [ ] Replace `0xXX` with actual I2C address (hex format)
- [ ] Update `SENSOR_ID` (unique uint8_t)
- [ ] Update `SENSOR_NAME` (human-readable string)
- [ ] Adjust register addresses from datasheet
- [ ] Implement `read()` conversion formula based on datasheet
- [ ] Set correct `SensorType` enum value
- [ ] Update sensitivity/scale factor
- [ ] Test with `i2c_scanner` environment first
- [ ] Register in `AppManager::begin()`
- [ ] Add unit test in `/test/` if complex

---

## Testing

```bash
# Verify I2C communication
platformio run -e i2c_scanner

# Build main application
platformio run -e KMUTNB-ESP32-V1

# Monitor sensor output (check serial logs for "[XyzSensor]" messages)
```

See [AGENTS.md](AGENTS.md) for debugging strategies.
