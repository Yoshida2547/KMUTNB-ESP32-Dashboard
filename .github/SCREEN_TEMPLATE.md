# Screen Implementation Template

Use this template to quickly implement a new screen. Replace `MyScreen` with your screen name throughout.

---

## Step 1: Create Header File

**File**: `include/MyScreen.h`

```cpp
#pragma once

#include <lvgl.h>
#include "Screen.h"
#include "Sensor.h"
#include "SensorManager.h"

namespace KMUTNB {

/**
 * MyScreen Implementation
 * Custom screen for [purpose: settings, sensor display, menu, etc.]
 * 
 * Layout:
 * - [Describe your screen layout here]
 * - [e.g., Title bar, sensor values, buttons]
 */
class MyScreen : public Screen {
private:
    // LVGL objects
    lv_obj_t* screen;
    lv_obj_t* title_label;
    lv_obj_t* value_label;
    lv_obj_t* button;
    // Add more widgets as needed

    // Screen state
    bool data_loaded;
    
    // Helper methods
    void create_widgets();
    void setup_styles();
    static void button_click_cb(lv_event_t* event);

public:
    MyScreen();
    virtual ~MyScreen() = default;

    // Override Screen interface methods
    void build() override;
    void update() override;
    lv_obj_t* getScreen() const override { return screen; }
};

} // namespace KMUTNB
```

---

## Step 2: Create Implementation File

**File**: `src/MyScreen.cpp`

```cpp
#include "MyScreen.h"
#include "DisplayManager.h"
#include <Arduino.h>

using namespace KMUTNB;

// Constructor
MyScreen::MyScreen() 
    : screen(nullptr), title_label(nullptr), value_label(nullptr), 
      button(nullptr), data_loaded(false) {
}

// Build UI (called once when screen is created)
void MyScreen::build() {
    // Create main screen object
    screen = lv_obj_create(NULL);

    // Setup color/style for entire screen
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x1a1a2e), 0);

    // Create title label
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "My Screen");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);

    // Create value display label
    value_label = lv_label_create(screen);
    lv_label_set_text(value_label, "Value: --");
    lv_obj_align(value_label, LV_ALIGN_CENTER, 0, -40);
    lv_obj_set_style_text_color(value_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_text_font(value_label, &lv_font_montserrat_24, 0);

    // Create button (if needed)
    button = lv_button_create(screen);
    lv_obj_set_size(button, 120, 40);
    lv_obj_align(button, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(button, button_click_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* button_label = lv_label_create(button);
    lv_label_set_text(button_label, "Action");
    lv_obj_center(button_label);

    // Apply any custom styles
    setup_styles();

    Serial.println("[MyScreen] Screen built successfully");
}

// Update data (called periodically by UIManager at 100ms interval)
void MyScreen::update() {
    if (!screen) return;

    SensorManager* sensorManager = SensorManager::getInstance();

    // Pull data from sensors
    Sensor* sensor = sensorManager->getSensorByName("ADXL345");
    
    if (value_label && sensor) {
        lv_label_set_text(value_label, sensor->getFormattedData());
    }

    // Update any other UI elements based on state
    // Example: animate, show/hide, change colors, etc.
}

// Button callback example
void MyScreen::button_click_cb(lv_event_t* event) {
    MyScreen* screen = (MyScreen*)lv_event_get_user_data(event);
    
    Serial.println("[MyScreen] Button clicked!");
    
    // Handle button action here
    // Example: switch to another screen, trigger action, etc.
}

// Helper: Setup custom styles
void MyScreen::setup_styles() {
    // Create custom style for button
    static lv_style_t button_style;
    lv_style_init(&button_style);
    lv_style_set_bg_color(&button_style, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_border_color(&button_style, lv_color_white());
    lv_style_set_border_width(&button_style, 2);
    lv_style_set_radius(&button_style, 5);

    lv_obj_add_style(button, &button_style, 0);
}
```

---

## Step 3: Update UIManager

**File**: `include/UIManager.h` (optional: if you want to swap screens dynamically)

You can add a method to UIManager to switch screens:

```cpp
public:
    // Add this method to UIManager.h
    void setCurrentScreen(Screen* new_screen) {
        if (new_screen) {
            // Delete old screen if it exists
            if (currentScreen) {
                delete currentScreen;
            }
            currentScreen = new_screen;
            buildUI();
        }
    }
```

---

## Step 4: Create/Switch Screen

### Option A: Replace MainScreen at startup

**File**: `src/AppManager.cpp` (in `begin()` method)

```cpp
void AppManager::begin() {
    // ... existing initialization ...

    // Initialize UI with MyScreen instead of MainScreen
    UIManager* uiManager = UIManager::getInstance();
    // Swap: uiManager->setCurrentScreen(new MyScreen());
    // (Note: UIManager starts with MainScreen by default in constructor)
    
    uiManager->buildUI();

    // ... rest of initialization ...
}
```

### Option B: Switch screens dynamically at runtime

```cpp
// From anywhere in your code:
UIManager* uiManager = UIManager::getInstance();
uiManager->setCurrentScreen(new MyScreen());
```

---

## LVGL Widget Reference

### Common Widgets
```cpp
// Labels
lv_obj_t* label = lv_label_create(parent);
lv_label_set_text(label, "Text");

// Buttons
lv_obj_t* button = lv_button_create(parent);
lv_obj_add_event_cb(button, click_callback, LV_EVENT_CLICKED, user_data);

// Sliders
lv_obj_t* slider = lv_slider_create(parent);
lv_slider_set_range(slider, 0, 100);

// Arcs (gauge-like)
lv_obj_t* arc = lv_arc_create(parent);
lv_arc_set_range(arc, 0, 100);

// Spinbox
lv_obj_t* spinbox = lv_spinbox_create(parent);
lv_spinbox_set_range(spinbox, 0, 100);

// Gauge
lv_obj_t* gauge = lv_gauge_create(parent);

// Chart (for data graphs)
lv_obj_t* chart = lv_chart_create(parent);
lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
```

### Positioning
```cpp
lv_obj_align(widget, LV_ALIGN_CENTER, 0, 0);
lv_obj_align(widget, LV_ALIGN_TOP_LEFT, 10, 10);
lv_obj_align(widget, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
```

### Styling
```cpp
lv_obj_set_style_bg_color(widget, lv_color_hex(0xFF0000), 0);
lv_obj_set_style_text_color(widget, lv_color_white(), 0);
lv_obj_set_style_text_font(widget, &lv_font_montserrat_20, 0);
lv_obj_set_style_border_width(widget, 2, 0);
lv_obj_set_style_radius(widget, 5, 0);
```

---

## Thread Safety Reminder

⚠️ **If updating UI from a task or callback:**

```cpp
// ALWAYS lock the GUI mutex when calling LVGL functions outside ui_update_task
AppManager::getInstance()->lockGUI();
lv_label_set_text(label, "New text");
AppManager::getInstance()->unlockGUI();
```

The UI task already handles this for `update()` method.

---

## Checklist

- [ ] Replace all `MyScreen` with your screen class name (PascalCase)
- [ ] Update title label text to match screen purpose
- [ ] Add all necessary widgets (labels, buttons, sliders, etc.)
- [ ] Implement `update()` to pull data from sensors/managers
- [ ] Handle button callbacks (if using buttons)
- [ ] Test LVGL alignment and positioning
- [ ] Apply custom styles/colors if needed
- [ ] Add to UIManager or switch via `setCurrentScreen()`
- [ ] Test with touch input (if using buttons)

---

## Testing

```bash
# Build main application
platformio run -e KMUTNB-ESP32-V1

# Upload to device
platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-port /dev/ttyUSB0

# Monitor serial output for "[MyScreen]" messages
```

---

## Common Patterns

### Multi-page Navigation (Simple Example)

```cpp
// In main app, swap screens on button click:
static void next_screen_callback(lv_event_t* event) {
    UIManager::getInstance()->setCurrentScreen(new NextScreen());
}

// In MyScreen::build():
lv_obj_add_event_cb(button, next_screen_callback, LV_EVENT_CLICKED, nullptr);
```

### Real-time Data Display

```cpp
// In update() method, continuously pull fresh data:
void MyScreen::update() {
    Sensor* sensor = SensorManager::getInstance()->getSensorByName("SensorName");
    if (sensor) {
        lv_label_set_text_fmt(data_label, "Latest: %s", sensor->getFormattedData());
    }
}
```

---

See [AGENTS.md](AGENTS.md) for full thread safety rules and debugging strategies.
