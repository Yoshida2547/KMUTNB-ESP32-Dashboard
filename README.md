# KMUTNB-ESP32-V2.0-TEST

An embedded ESP32-S3 platform featuring LVGL GUI, FreeRTOS multi-core task management, and a pluggable sensor system. This project demonstrates advanced embedded systems patterns including the Strategy pattern for pluggable components, singleton resource managers, and thread-safe concurrent programming.

**Platform**: ESP32-S3 | **GUI**: LVGL (Light and Versatile Graphics Library) | **RTOS**: FreeRTOS | **Build Tool**: PlatformIO

---

## ✨ Features

- **Multi-Core GUI Framework** – LVGL rendering on Core 0 with concurrent sensor polling and UI updates on Core 1
- **Pluggable Sensor Architecture** – Abstract `Sensor` interface with concrete implementations (ADXL345 accelerometer, extensible for LTR303, HDC1080, LPS22HB)
- **Thread-Safe Resource Management** – Singleton managers (`AppManager`, `SensorManager`, `UIManager`, `DisplayManager`) with mutex protection for LVGL and sensor data
- **ILI9488 Display Support** – 3.5" 320x480 RGB TFT with touch input, fully calibrated
- **Real-Time Sensor Monitoring** – 100ms polling interval with live UI updates
- **Modular Screen System** – Abstract `Screen` interface for building pluggable UI screens (currently: `MainScreen`)

---

## 🚀 Quick Start

### Prerequisites

- **PlatformIO Core** (installed and on PATH)
- **Python 3.6+** (for build scripts)
- **ESP32-S3 Development Board** with ILI9488 3.5" TFT display and ADXL345 accelerometer
- **USB-to-UART adapter** for serial communication (auto-detected as `/dev/ttyUSB0` or `/dev/ttyACM0`)

### Installation

1. **Clone and open workspace**:
   ```bash
   cd /home/surasak/Documents/PlatformIO/Projects/KMUTNB-ESP32-V2.0-TEST
   code .
   ```

2. **Run post-install setup** (configures TFT_eSPI):
   ```bash
   python3 scripts/post_install.py
   ```

3. **Build the main firmware**:
   ```bash
   platformio run -e KMUTNB-ESP32-V1
   ```

4. **Upload to device**:
   ```bash
   platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-speed 115200
   ```

---

## 📦 Build Commands

### Main Application
```bash
# Default build (uses KMUTNB-ESP32-V1 from platformio.ini)
platformio run

# Build with upload and serial monitor (auto port detection)
platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-speed 115200

# Specify explicit serial port
platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-port /dev/ttyUSB0 --monitor-speed 115200
```

### Utility Environments
```bash
# I2C Device Scanner – Detect connected sensors on I2C bus
platformio run -e i2c_scanner

# Touch Calibration Utility – Calibrate display touch input
platformio run -e Touch_calibrate

# LCD Tester – Test display rendering without sensors
platformio run -e KMUTNB-ESP32-V1-LCD-TESTER

# LVGL Benchmark – Performance testing
platformio run -e lvgl_benchmark
```

### Monitor Serial Output
```bash
# Auto-detect port (recommended)
platformio device monitor --baud 115200

# Specify port explicitly
platformio device monitor --port /dev/ttyUSB0 --baud 115200

# Combined upload + monitor
platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-speed 115200
```

---

## 📂 Project Structure

```
├── include/              # Public APIs and abstractions
│   ├── Sensor.h         # Abstract sensor interface
│   ├── Screen.h         # Abstract screen interface
│   ├── AppManager.h     # Application lifecycle & LVGL orchestration
│   ├── SensorManager.h  # Sensor registry & polling task
│   ├── UIManager.h      # Screen lifecycle & UI update task
│   ├── DisplayManager.h # LCD hardware & touch driver
│   ├── MainScreen.h     # Current UI implementation
│   ├── ADXL345Sensor.h  # Accelerometer implementation
│   └── StorageManager.h # NVS (flash) storage utilities
│
├── src/                 # Implementations
│   ├── main.cpp         # Entry point (setup/loop)
│   ├── AppManager.cpp   # Manager implementations
│   ├── SensorManager.cpp
│   ├── UIManager.cpp
│   ├── DisplayManager.cpp
│   ├── MainScreen.cpp   # UI rendering & touch handling
│   ├── ADXL345Sensor.cpp
│   └── StorageManager.cpp
│
├── scripts/             # Build and configuration utilities
│   ├── lv_conf.h        # LVGL configuration (auto-installed)
│   ├── Setup21_ILI9488.h # TFT_eSPI display config
│   ├── Touch.h/.cpp     # Touch input driver
│   ├── post_install.py  # Setup script for TFT_eSPI
│   └── apply_touch_vspi.py
│
├── lib/                 # External libraries
│   └── lvgl_touch_calibration/ # Touch calibration component
│
├── tools/               # Standalone utilities
│   ├── i2c_scanner/     # I2C device detection
│   ├── lcd_tester/      # Display-only testing
│   ├── lvgl_benchmark/  # Performance benchmarking
│   └── Touch_calibrate/ # Interactive touch calibration
│
├── test/                # Reference sensor implementations (not in main build)
│   ├── test_buzzer/
│   ├── test_rgb_led/
│   └── test_i2c_scan/
│
├── platformio.ini       # Build configuration & environments
└── README.md            # This file
```

---

## 🏗️ Architecture & Design Patterns

### Strategy Pattern: Pluggable Sensors & Screens

**Sensor Abstraction** ([`include/Sensor.h`](include/Sensor.h)):
```cpp
class Sensor {
public:
    virtual bool initialize() = 0;
    virtual void read() = 0;
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual void logData() = 0;
};
```

**Concrete Implementations**:
- [`AcceleroMeter`](include/ADXL345Sensor.h) – ADXL345 I2C accelerometer (currently active)
- Future: `TemperatureSensor` (HDC1080), `LightSensor` (LTR303), `PressureSensor` (LPS22HB)

**Screen Abstraction** ([`include/Screen.h`](include/Screen.h)):
```cpp
class Screen {
public:
    virtual void build(lv_obj_t* parent) = 0;
    virtual void update() = 0;
    virtual lv_obj_t* getScreen() = 0;
};
```

**Concrete Implementations**:
- [`MainScreen`](include/MainScreen.h) – Sensor display + touch cursor (current)
- Future: `SettingsScreen`, `MenuScreen`

### Singleton Managers: Centralized Resource Control

| Manager | Purpose | Thread | Core |
|---------|---------|--------|------|
| [`AppManager`](include/AppManager.h) | App lifecycle, LVGL task orchestration | LVGL Task | 0 |
| [`SensorManager`](include/SensorManager.h) | Sensor registry, polling loop | Sensor Task | 1 |
| [`UIManager`](include/UIManager.h) | Screen lifecycle, UI updates | UI Task | 1 |
| [`DisplayManager`](include/DisplayManager.h) | LCD hardware, touch driver | Both | 0/1 |

**Usage Pattern**:
```cpp
AppManager* app = AppManager::getInstance();
SensorManager* sensors = SensorManager::getInstance();
UIManager* ui = UIManager::getInstance();
```

---

## ⚡ Task Layout & Concurrency

### FreeRTOS Task Hierarchy
```
Core 0 (app_cpu):
  └─ LVGL Task (priority 10, 8KB stack)
     ├─ Render GUI to display every 5ms
     ├─ Process touch input
     └─ Protected by gui_mutex for thread safety

Core 1 (pro_cpu):
  ├─ Sensor Task (priority 2, 4KB stack)
  │  └─ Poll sensors every 100ms
  │     └─ Protected by sensor_mutex
  │
  └─ UI Task (priority 1, 4KB stack)
     └─ Update screen every 100ms
        └─ Protected by gui_mutex when calling LVGL

Main (loop):
  └─ Periodic logging (minimal work)
```

### ⚠️ Thread Safety Rules

**LVGL is NOT thread-safe.** All LVGL calls require `gui_mutex` lock:

```cpp
// ❌ DANGEROUS – Race condition!
lv_label_set_text(label, "Hello");

// ✅ CORRECT – Thread-safe
AppManager::getInstance()->lockGUI();
lv_label_set_text(label, "Hello");
AppManager::getInstance()->unlockGUI();
```

**Note**: `UIManager::ui_update_task` already handles locking internally. For custom LVGL calls outside tasks, always lock.

---

## 🔌 Hardware Configuration

### I2C Sensors (Pins SDA=21, SCL=22, 100kHz)

| Address | Sensor | Status | Test Location |
|---------|--------|--------|---|
| 0x53 | **ADXL345** (Accelerometer) | ✅ Active | Currently integrated |
| 0x29 | LTR303ALS (Ambient Light) | ⏳ Available | [`/test/test_ltr303/`](test/) |
| 0x40 | HDC1080 (Temp/Humidity) | ⏳ Available | [`/test/test_hdc1080/`](test/) |
| 0x5C | LPS22HB (Pressure) | ⏳ Available | [`/test/test_lps22hb/`](test/) |

**Verify connected devices**:
```bash
platformio run -e i2c_scanner
platformio device monitor --baud 115200
```

### SPI Display

- **Model**: ILI9488 (3.5" 320x480 RGB TFT)
- **Interface**: HSPI (SPI2 on ESP32-S3)
- **Pins**: MOSI=13, MISO=12, SCLK=14, CS=15, DC=2, RST=4
- **Config**: Installed from [`scripts/Setup21_ILI9488.h`](scripts/Setup21_ILI9488.h) via `post_install.py`
- **Driver**: TFT_eSPI library with LVGL integration

### Touch Input (VSPI)

- **Type**: Capacitive touch overlay on ILI9488
- **Calibration**: Hardcoded in [`include/DisplayManager.h:L12`](include/DisplayManager.h#L12)
- **Update calibration**:
  ```bash
  platformio run -e Touch_calibrate --monitor-speed 115200
  ```
  Collect the 5-element calibration array and update `DisplayConfig::touch_cal_data[5]`

### LED (Optional)

- **RGB LED**: GPIO 16 (Adafruit NeoPixel compatible, currently unused)

---

## 🛠️ Common Development Tasks

### Adding a New Sensor

1. **Create header** `include/XyzSensor.h`:
   ```cpp
   namespace KMUTNB {
   class XyzSensor : public Sensor {
   public:
       bool initialize() override;
       void read() override;
       // ... implement all pure virtual methods
   };
   }
   ```

2. **Implement** `src/XyzSensor.cpp`:
   ```cpp
   using namespace KMUTNB;
   bool XyzSensor::initialize() { /* I2C setup */ }
   void XyzSensor::read() { /* Poll sensor */ }
   ```

3. **Register in** [`src/AppManager.cpp:L44`](src/AppManager.cpp#L44):
   ```cpp
   XyzSensor* sensor = new XyzSensor();
   if (sensor->initialize()) {
       sensor->enable();
       SensorManager::getInstance()->registerSensor(sensor);
   }
   ```

4. **Display in UI** via `SensorManager::getSensorByName()` in [`MainScreen::update()`](src/MainScreen.cpp#L32)

### Adding a New Screen

1. Create `include/MyScreen.h` inheriting from [`Screen`](include/Screen.h)
2. Implement `build()`, `update()`, `getScreen()`
3. In [`UIManager`](include/UIManager.h), set: `currentScreen = new MyScreen()`
4. Lifecycle automatically managed by `UIManager`'s destructor

### Debugging Common Issues

| Issue | Solution |
|-------|----------|
| I2C device not found | Run `platformio run -e i2c_scanner` to verify address & connection |
| Touch input not working | Run calibration tool, update `DisplayManager.h:L12` with new cal data |
| Sensor not reading | Check `SensorManager::logAllData()` output in `loop()` (every 5s) |
| Display corruption | Increase `draw_buf_size` in [`DisplayManager.cpp:L40`](src/DisplayManager.cpp#L40) |
| Stack overflow crash | Monitor with `uxTaskGetStackHighWaterMark()`; default limits: LVGL 8KB, Sensor 4KB, UI 4KB |

---

## 📝 Build Configuration

### Key Flags (platformio.ini)

- **C++ Standard**: `-std=c++17` (required for `inline static` members in singletons)
- **Optimization**: `-O2` (Release build for performance)
- **LVGL Config**: Auto-installed from [`scripts/lv_conf.h`](scripts/lv_conf.h)
- **Display Driver**: TFT_eSPI with HSPI configuration

### Environment Definitions

- `KMUTNB-ESP32-V1` – Main firmware (default)
- `i2c_scanner` – I2C device detection
- `Touch_calibrate` – Touch calibration utility
- `KMUTNB-ESP32-V1-LCD-TESTER` – Display-only testing
- `lvgl_benchmark` – Performance analysis

See [platformio.ini](platformio.ini) for full configuration.

---

## 📊 Performance & Memory

| Component | Memory | Task Priority | Core |
|-----------|--------|---|------|
| LVGL Task | 8 KB stack | 10 (highest) | 0 |
| Sensor Polling | 4 KB stack | 2 | 1 |
| UI Update | 4 KB stack | 1 | 1 |
| Display Buffer | ~60 KB heap | — | — |

**Tips**:
- LVGL task priority is highest to ensure responsive GUI
- Sensor and UI tasks have lower priority; blocking I2C calls are acceptable
- Display buffer is ~10% of framebuffer (320×480×16-bit); increase if graphical artifacts occur
- Monitor with `uxTaskGetStackHighWaterMark()` if modifying task stack sizes

---

## 🔍 Verification & Testing

### Verify Build
```bash
platformio run -e KMUTNB-ESP32-V1
```

### Test Display Only
```bash
platformio run -e KMUTNB-ESP32-V1-LCD-TESTER -t upload --monitor-speed 115200
```

### Benchmark Performance
```bash
platformio run -e lvgl_benchmark -t upload --monitor-speed 115200
```

### Scan I2C Bus
```bash
platformio run -e i2c_scanner -t upload --monitor-speed 115200
```

---

## 🎯 Important Notes

### ⚠️ Gotchas
- **Post-install required**: Run `python3 scripts/post_install.py` if TFT_eSPI config is missing
- **C++17 mandatory**: Singletons use `inline static` members; requires `-std=c++17` (already set)
- **Touch coordinates swapped**: `DisplayManager::touch_read_cb` swaps X/Y due to display rotation
- **Sensor data not yet integrated**: Test utilities exist for LTR303, HDC1080, LPS22HB; only ADXL345 currently registered

### Build Failures & Solutions
| Error | Cause | Solution |
|-------|-------|----------|
| `undefined reference to '__atomic_load_8'` | Missing Arduino core libs | Verify `platformio.ini` includes Arduino framework |
| `lv_conf.h not found` | TFT_eSPI not initialized | Run `python3 scripts/post_install.py` |
| Undefined I2C sensor methods | Sensor not registered | Check `AppManager::initialize()` for registration |
| Touch not responding | Calibration data stale | Re-run `platformio run -e Touch_calibrate` |

---

## 🚦 Git Workflow

All commits must follow a scope-based message format:

```bash
# Format: scope(component): description
git commit -m "fix(sensor): correct ADXL345 initialization timing"
git commit -m "feat(ui): add settings screen"
git commit -m "refactor(manager): improve thread safety in SensorManager"
```

Common scopes: `display`, `sensor`, `ui`, `manager`, `build`, `doc`

---

## 📚 Key Files Reference

| File | Purpose |
|------|---------|
| [platformio.ini](platformio.ini) | Build environments & dependencies |
| [src/main.cpp](src/main.cpp) | Entry point: `setup()` / `loop()` |
| [include/Sensor.h](include/Sensor.h) | Abstract sensor interface |
| [include/Screen.h](include/Screen.h) | Abstract screen interface |
| [include/AppManager.h](include/AppManager.h) | Application lifecycle & LVGL task |
| [include/SensorManager.h](include/SensorManager.h) | Sensor registry & polling |
| [include/UIManager.h](include/UIManager.h) | Screen lifecycle & UI updates |
| [include/DisplayManager.h](include/DisplayManager.h) | LCD/touch hardware driver |
| [include/MainScreen.h](include/MainScreen.h) | Current UI (sensor display) |
| [include/ADXL345Sensor.h](include/ADXL345Sensor.h) | Accelerometer implementation |
| [.github/copilot-instructions.md](.github/copilot-instructions.md) | AI agent customization & conventions |

---

## 🔗 Related Documentation

- **AI Development Guide**: See [.github/copilot-instructions.md](.github/copilot-instructions.md) for conventions, thread safety rules, and AI agent best practices
- **LVGL Documentation**: https://docs.lvgl.io/master/
- **FreeRTOS Documentation**: https://www.freertos.org/
- **TFT_eSPI Repository**: https://github.com/Bodmer/TFT_eSPI
- **PlatformIO Documentation**: https://docs.platformio.org/

---

## 📋 License

See repository for license information.

---

**Last Updated**: May 9, 2026 | **Build Status**: ✅ Compiles with `-std=c++17` | **Platform**: ESP32-S3
