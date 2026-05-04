# KMUTNB-ESP32-V2.0-TEST: AI Agent Guide

**Project**: Embedded ESP32-S3 platform with LVGL GUI, FreeRTOS multi-core tasks, and pluggable sensor system.

---

## Quick Start for Agents

### Build Commands
```bash
# Default build (uses `default_envs` from `platformio.ini`)
platformio run

# If `platformio` is not on PATH in this shell
/home/surasak/.platformio/penv/bin/platformio run -d /home/surasak/Documents/PlatformIO/Projects/KMUTNB-ESP32-V2.0-TEST

# Main application (ESP32-S3)
platformio run -e KMUTNB-ESP32-V1

# With upload + serial monitor
platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-port /dev/ttyUSB0 --monitor-speed 115200

# Testing utilities
platformio run -e i2c_scanner          # Scan I2C bus for connected devices
platformio run -e Touch_calibrate      # Calibrate touch input
platformio run -e KMUTNB-ESP32-V1-LCD-TESTER  # Test display only
```

### Runtime Monitoring & Verification
To monitor and verify your program at runtime, use the serial monitor to view logs and output. You can use `/dev/ttyACM0`, `/dev/ttyUSB0`, or let PlatformIO auto-detect the port:

```bash
# Use auto-detect (recommended)
platformio device monitor --baud 115200

# Or specify the port explicitly (e.g. /dev/ttyACM0 or /dev/ttyUSB0)
platformio device monitor --port /dev/ttyACM0 --baud 115200
```

Or combine upload and monitoring:

```bash
# Auto-detect port
platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-speed 115200

# Or specify port
platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-port /dev/ttyACM0 --monitor-speed 115200
```

Use this to verify sensor data, debug output, and runtime status.

### Build Notes
- The repository default build target is [`env:KMUTNB-ESP32-V1`](platformio.ini), so `platformio run` builds the main firmware by default.
- The main firmware lives in [`env:KMUTNB-ESP32-V1`](platformio.ini); utility builds are isolated through `build_src_filter` in the tool environments.
- Build commands require PlatformIO Core to be installed and available as `platformio` or `pio` on `PATH`.
- In this workspace shell, `platformio run` may fail if the PlatformIO binary is not on `PATH`; use `/home/surasak/.platformio/penv/bin/platformio` as a direct fallback.
- For touched files only, prefer the smallest relevant environment over a full project sweep.

### Git Workflow (All Agents)
- Any agent that changes files in this project must create a commit for those changes before finishing.
- Use clear commit messages with a concise scope prefix, e.g. `fix(display): correct touch coordinate mapping`.
- Do not amend previous commits unless explicitly requested.
- Do not rewrite history; avoid force-push style workflows.

### Project Structure
- **`include/`** – Public APIs: `Sensor`, `Screen` (abstract), managers, `AcceleroMeter`, `MainScreen`
- **`src/`** – Implementations: managers, `main.cpp`, UI logic
- **`scripts/`** – Build utilities: LCD/touch config, post-install setup
- **`tools/`** – Standalone utilities: LCD tester, I2C scanner, calibration tools
- **`test/`** – Reference sensor test code (not in main build)
- **`platformio.ini`** – Build config: `-std=c++17` (required), environment definitions

---

## Architecture & Patterns

### Design: Strategy Pattern + Singletons + Namespace

**Abstract Interfaces** (enable pluggable implementations):
- [`Sensor`](include/Sensor.h) – Base for accelerometer, temperature, pressure sensors
  - Concrete: [`AcceleroMeter`](include/ADXL345Sensor.h) (ADXL345 via I2C)
  - Future: `TemperatureSensor`, `HumiditySensor` (test code exists in `/test/`)
- [`Screen`](include/Screen.h) – Base for UI screens
  - Concrete: [`MainScreen`](include/MainScreen.h) (sensor data display)
  - Future: `SettingsScreen`, `MenuScreen` (swap via `UIManager::currentScreen`)

**Singleton Managers** (centralized resource control):
- [`AppManager`](include/AppManager.h) – Application lifecycle, LVGL task orchestration
- [`SensorManager`](include/SensorManager.h) – Sensor registry, polling task (Core 1, 100ms interval)
- [`UIManager`](include/UIManager.h) – Screen lifecycle, UI task (Core 1, 100ms updates)
- [`DisplayManager`](include/DisplayManager.h) – LCD hardware, touch input, LVGL display driver

**Namespace**: `namespace KMUTNB` (all classes). Use `using namespace KMUTNB;` or `KMUTNB::ClassName`.

---

## Critical: Thread Safety & Concurrency

### Task Layout
```
Core 0: LVGL Task (prio 10, 8KB stack) – GUI rendering, touch handling
Core 1: Sensor Task (prio 2, 4KB) + UI Task (prio 1, 4KB) – Data polling, screen updates
Main: loop() – Periodic logging only (minimal work)
```

### ⚠️ LVGL Thread Safety Rules
**LVGL is NOT thread-safe.** All LVGL calls require `gui_mutex` lock:
```cpp
AppManager::getInstance()->lockGUI();
// LVGL operations here
AppManager::getInstance()->unlockGUI();
```

**`UIManager::ui_update_task` already handles this.** For custom UI updates outside tasks:
- Wrap with `lockGUI()/unlockGUI()` 
- Never call LVGL from sensor task without locking

### Mutexes
- **`AppManager::gui_mutex`** – Protects all LVGL calls
- **`SensorManager::sensor_mutex`** – Protects `sensors_by_id` map access during task and registration

---

## Common Development Tasks

### Adding a New Sensor

1. **Create header** [`include/XyzSensor.h`]:
   ```cpp
   namespace KMUTNB {
   class XyzSensor : public Sensor {
   public:
       bool initialize() override;
       void read() override;  // Implement all pure virtual methods
       // ... other overrides
   };
   } // namespace KMUTNB
   ```

2. **Implement** [`src/XyzSensor.cpp`]:
   ```cpp
   using namespace KMUTNB;
   XyzSensor::XyzSensor() : ... {}
   bool XyzSensor::initialize() { /* I2C setup */ }
   // ...
   ```

3. **Register in** [`src/AppManager.cpp`](src/AppManager.cpp#L44):
   ```cpp
   XyzSensor* sensor = new XyzSensor();
   if (sensor->initialize()) {
       sensor->enable();
       SensorManager::getInstance()->registerSensor(sensor);
   }
   ```

4. **Display in UI** via `SensorManager::getSensorByName()` or `getSensorById()` in `MainScreen::update()`.

### Adding a New Screen
1. Create [`include/MyScreen.h`] inheriting from [`Screen`](include/Screen.h)
2. Implement `build()`, `update()`, `getScreen()` 
3. In [`UIManager`](include/UIManager.h), swap: `currentScreen = new MyScreen()`
4. All lifecycle managed automatically by `UIManager` destructor.

### Debugging
- **I2C issues**: Run `platformio run -e i2c_scanner` to verify device addresses
- **Touch not working**: Run `platformio run -e Touch_calibrate`, collect output, update [`DisplayManager.h`](include/DisplayManager.h#L12) `DisplayConfig::touch_cal_data[5]`
- **Sensor not reading**: Check [`SensorManager::logAllData()`](src/SensorManager.cpp#L95) output in `loop()` (called every 5s)
- **Display corruption**: Increase `draw_buf_size` in [`DisplayManager.cpp`](src/DisplayManager.cpp#L40)
- **Stack overflow**: Monitor with `uxTaskGetStackHighWaterMark()` if modifying tasks; default: LVGL 8KB, sensor 4KB, UI 4KB

---

## Hardware Configuration

### I2C Sensors (default pins SDA=21, SCL=22)
| Address | Sensor | Test Location |
|---------|--------|---|
| 0x29 | LTR303ALS (ambient light) | `/test/test_ltr303/` |
| 0x40 | HDC1080 (temp/humidity) | `/test/test_hdc1080/` |
| 0x53 | ADXL345 (accelerometer) | **Currently active** |
| 0x5C | LPS22HB (pressure) | `/test/test_lps22hb/` |

### SPI Display (HSPI: MOSI=13, MISO=12, SCLK=14, CS=15, DC=2, RST=4)
- **Driver**: ILI9488 (3.5" 320x480 RGB)
- **Config**: [scripts/Setup21_ILI9488.h](scripts/Setup21_ILI9488.h) (auto-installed by post_install.py)
- **Touch calibration**: Hardcoded in [DisplayManager.h:12](include/DisplayManager.h#L12); run Touch_calibrate environment to update

### LED
- **RGB LED**: GPIO 16, controlled via Adafruit NeoPixel library (available but not currently used)

---

## Key Implementation Details

### Sensor Read Loop
[`SensorManager::runSensorTask()`](src/SensorManager.cpp#L19):
- Polls all registered sensors every 100ms on Core 1
- Acquires `sensor_mutex` before iterating `sensors_by_id`
- Calls `sensor->read()` → `sensor->logData()` for enabled sensors
- **Note**: Blocking I2C calls okay here (task-based), but slow sensors may reduce UI framerate

### UI Update Loop
[`UIManager::ui_update_task()`](src/UIManager.cpp#L42):
- Updates screen every 100ms on Core 1
- Acquires `gui_mutex` (thread-safe LVGL access)
- Pulls sensor data via `SensorManager::getSensorByName()` in [`MainScreen::update()`](src/MainScreen.cpp#L32)

### LVGL Rendering Loop
[`AppManager::task_lvgl()`](src/AppManager.cpp#L72):
- Core 0, priority 10 (highest), 5ms tick
- Calls `lv_task_handler()` under `gui_mutex` lock
- Handles touch input automatically

---

## Important Notes

### ⚠️ Gotchas
- **Post-install script required**: Run `python3 scripts/post_install.py` if TFT_eSPI config missing
- **C++17 mandatory**: `inline static` members require `-std=c++17` flag (already set in platformio.ini)
- **Touch coordinates swapped**: `DisplayManager::touch_read_cb` swaps X/Y due to display rotation (see line 28 [DisplayManager.cpp](src/DisplayManager.cpp#L28))
- **Sensor data not yet integrated**: Test utilities exist for LTR303, HDC1080, LPS22HB; only ADXL345 currently registered

### Stack/Memory
- **LVGL task**: 8KB (watch for custom widgets)
- **Sensor task**: 4KB (sufficient for I2C polling)
- **UI task**: 4KB (sufficient for current screen updates)
- **Display buffer**: ~60KB (10% of 320x480x16-bit); increase if graphical corruption occurs

### Testing
- Individual sensor tests in `/test/` directories (reference implementations)
- Run each test environment in `platformio.ini` to validate hardware
- Main binary excludes test code via `build_src_filter` in `platformio.ini`

---

## Links to Key Files

| File | Purpose |
|------|---------|
| [platformio.ini](platformio.ini) | Build environments, dependencies, flags |
| [src/main.cpp](src/main.cpp) | Entry point: `setup()` / `loop()` |
| [include/Sensor.h](include/Sensor.h) | Abstract sensor interface |
| [include/Screen.h](include/Screen.h) | Abstract screen interface |
| [include/AppManager.h](include/AppManager.h) | Application lifecycle, LVGL task |
| [include/SensorManager.h](include/SensorManager.h) | Sensor registry, polling task |
| [include/UIManager.h](include/UIManager.h) | Screen lifecycle, UI task |
| [include/DisplayManager.h](include/DisplayManager.h) | LCD/touch hardware driver |
| [include/MainScreen.h](include/MainScreen.h) | Current UI (sensor display + touch cursor) |
| [include/ADXL345Sensor.h](include/ADXL345Sensor.h) | Accelerometer implementation |

---

## Next Steps for AI Agents

When adding features or fixing bugs:

1. **Check thread context**: Are you in task or main loop? Protect LVGL calls.
2. **Check namespace**: Use `using namespace KMUTNB;` in implementations; fully qualify in headers.
3. **Check singletons**: Don't instantiate managers directly; use `::getInstance()`.
4. **Check error paths**: All `init()`/`begin()`/`initialize()` return `bool`; validate before proceeding.
5. **Check FreeRTOS**: Don't `delay()` in tasks; use `vTaskDelay(pdMS_TO_TICKS(ms))`.

---

**Last Updated**: May 4, 2026 | **Build Status**: ✅ Compiles with `-std=c++17`
