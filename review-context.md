# Code Review Summary: KMUTNB-ESP32-V2.0-TEST

**Date:** May 4, 2026  
**Reviewer:** Code Review Agent  
**Focus Areas:** Thread safety, FreeRTOS patterns, LVGL GUI safety, design architecture

---

## Executive Summary

Overall project quality is **good** with solid embedded C++ architecture. Singleton pattern, Strategy pattern, and task separation are well-implemented. However, **two critical race conditions** require immediate fixes before production use.

---

## 🔴 Critical Issues (Must Fix)

### Issue 1: Race Condition in `SensorManager::getSensorByName()`
**File:** `src/SensorManager.cpp` (lines 59-66)  
**Severity:** P0 - Data race  
**Problem:**
- Mutex is released between finding sensor ID and accessing the sensor pointer
- Another thread could delete the sensor between the lookup and actual use
- Affects sensor thread iteration in `runSensorTask()`

**Impact:** Potential null pointer dereference or accessing freed memory

**Fix Strategy:** Keep mutex locked throughout entire operation:
```cpp
Sensor* SensorManager::getSensorByName(const char* name) {
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    Sensor* result = nullptr;
    auto it = sensors_by_name.find(name);
    if (it != sensors_by_name.end()) {
        auto it2 = sensors_by_id.find(it->second);
        result = (it2 != sensors_by_id.end()) ? it2->second : nullptr;
    }
    xSemaphoreGive(sensor_mutex);
    return result;
}
```

---

### Issue 2: Logic Error in `AppManager::task_lvgl()`
**File:** `src/AppManager.cpp` (line 77-79)  
**Severity:** P0 - Logic error  
**Problem:**
```cpp
if (!instance) {
    return; // Task already running
}
```
- Checks global `instance` instead of the passed `pvParameter`
- Comment says "Task already running" but code does the opposite
- Should check if `app` parameter is valid

**Impact:** Task silently exits if global instance is null, even with valid parameter

**Fix:**
```cpp
if (!app || !app->gui_mutex) {
    return;
}
```

---

## 🟡 Moderate Issues (Should Fix)

### Issue 3: Missing RAII Guard for Mutex Locking
**File:** `src/UIManager.cpp` (lines 46-54)  
**Severity:** P1 - Exception safety  
**Problem:**
```cpp
AppManager::getInstance()->lockGUI();
uiManager->update();
AppManager::getInstance()->unlockGUI();
```
- No exception safety guarantee
- If `update()` throws, mutex remains locked forever

**Recommendation:** Create RAII guard class:
```cpp
class GuiLock {
public:
    GuiLock(AppManager* app) : app(app) { app->lockGUI(); }
    ~GuiLock() { app->unlockGUI(); }
private:
    AppManager* app;
};

// Usage
{
    GuiLock lock(AppManager::getInstance());
    uiManager->update();
}
```

---

### Issue 4: Task Stack Sizes May Be Insufficient
**File:** `src/AppManager.cpp` (line 68), `src/UIManager.cpp` (line 41)  
**Severity:** P1 - Runtime stability  
**Current:**
- LVGL task: 8192 bytes (tight)
- Sensor task: 4096 bytes (OK)
- UI task: 4096 bytes (OK)

**Recommendation:** Increase LVGL task to 12288 bytes for complex screen rendering:
```cpp
xTaskCreatePinnedToCore(task_lvgl, "LVGL_Task", 12288, this, 10, &lvgl_task_handle, 0);
```

---

### Issue 5: Inefficient Mutex Coverage
**File:** `src/SensorManager.cpp` (lines 55-63)  
**Severity:** P2 - Performance  
**Problem:**
```cpp
void SensorManager::registerSensor(Sensor* sensor) {
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    uint8_t id = sensor->getId();
    sensors_by_id[id] = sensor;
    sensors_by_name[sensor->getName()] = id;  // Calling virtual method inside lock
    xSemaphoreGive(sensor_mutex);
}
```

**Recommendation:** Call virtual methods outside lock:
```cpp
void SensorManager::registerSensor(Sensor* sensor) {
    if (!sensor) return;
    uint8_t id = sensor->getId();
    const char* name = sensor->getName();
    
    xSemaphoreTake(sensor_mutex, portMAX_DELAY);
    sensors_by_id[id] = sensor;
    sensors_by_name[name] = id;
    xSemaphoreGive(sensor_mutex);
}
```

---

### Issue 6: Wire.begin() Called Multiple Times
**File:** `src/ADXL345Sensor.cpp` (line 13)  
**Severity:** P2 - I2C initialization  
**Problem:**
- Called in sensor's `initialize()` method
- Executes every time sensor is re-initialized
- Will break if multiple I2C sensors are added

**Recommendation:** Call centrally in `AppManager::begin()` before any sensor initialization:
```cpp
void AppManager::begin() {
    Serial.begin(115200);
    Wire.begin();  // ← Once here
    StorageManager::initLittleFS();
    DisplayManager::getInstance()->init();
    // ...
}
```

---

## 🟢 Minor Issues (Nice to Have)

### Issue 7: Class Naming Inconsistency
**File:** `include/ADXL345Sensor.h`, `src/ADXL345Sensor.cpp`  
**Problem:** Class is named `AcceleroMeter` but file is `ADXL345Sensor.*`

**Recommendation:** Rename class to `ADXL345Sensor` for consistency

---

### Issue 8: Missing Error Handling in `buildUI()`
**File:** `src/UIManager.cpp` (lines 21-27)  
**Problem:** No error return checking
**Recommendation:** Add return type for validation:
```cpp
bool UIManager::buildUI() {
    if (!currentScreen) return false;
    currentScreen->build();
    lv_screen_load(currentScreen->getScreen());
    return true;
}
```

---

### Issue 9: No Allocation Failure Check
**File:** `src/DisplayManager.cpp` (line 44)  
**Problem:**
```cpp
draw_buf = new uint8_t[config.draw_buf_size];
// No null check
lv_display_t *display = lv_tft_espi_create(..., draw_buf, ...);
```

**Recommendation:** Add validation:
```cpp
draw_buf = new uint8_t[config.draw_buf_size];
if (!draw_buf) {
    Serial.println("[DisplayManager] Failed to allocate draw buffer");
    return;
}
```

---

### Issue 10: Inefficient Fallback Lookup
**File:** `src/MainScreen.cpp` (lines 35-40)  
**Problem:**
```cpp
Sensor* sensor = sensorManager->getSensorById(0x53);
if (!sensor) {
    sensor = sensorManager->getSensorByName("ADXL345");
}
```

**Recommendation:** Cache sensor reference during MainScreen construction or at AppManager setup

---

## ✅ Architecture Strengths

1. **Singleton Pattern:** Correctly implemented for all managers
2. **Strategy Pattern:** Abstract `Sensor` and `Screen` base classes with proper contracts
3. **Thread Safety:** LVGL mutex protection in place; FreeRTOS patterns correct
4. **Task Separation:** LVGL on Core 0, sensors/UI on Core 1 (proper pinning)
5. **Namespace:** Consistent use of `KMUTNB` namespace
6. **Error Handling:** Reasonable initialization validation
7. **Design:** Clear separation of concerns (Display, UI, Sensors, AppManager)

---

## Testing Recommendations

1. **Stress Test:** Run sensor sampling at high frequency (10Hz+) while updating UI to expose race conditions
2. **Memory Check:** Monitor heap fragmentation with dynamic sensor allocation/deallocation
3. **Stack Monitoring:** Use FreeRTOS high-water mark checking on all tasks
4. **LVGL Mutex:** Verify no deadlocks with concurrent screen updates and sensor reads

---

## Priority Fix Order

1. ✅ **Fix race condition in `getSensorByName()`** (P0)
2. ✅ **Fix logic error in `task_lvgl()`** (P0)
3. ✅ **Move `Wire.begin()` to centralized location** (P1)
4. ✅ **Increase LVGL stack to 12KB** (P1)
5. ✅ **Add RAII GuiLock class** (P1)
6. ✅ **Rename `AcceleroMeter` to `ADXL345Sensor`** (P2)
7. Refactor mutex coverage optimization (P2)
8. Add error handling to build functions (P3)

---

## Conclusion

The project demonstrates solid embedded systems design. Fix the two critical race conditions and implement the RAII guard pattern for production readiness. All other issues are refinements for robustness and maintainability.
