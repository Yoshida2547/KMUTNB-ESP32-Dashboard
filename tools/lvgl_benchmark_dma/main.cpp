#include <Arduino.h>
#include <Arduino_GFX_Library.h>

namespace {

// Display SPI pins (HSPI)
constexpr int8_t PIN_MISO = 47;
constexpr int8_t PIN_MOSI = 11;
constexpr int8_t PIN_SCLK = 10;
constexpr int8_t PIN_CS = 45;
constexpr int8_t PIN_DC = 7;
constexpr int8_t PIN_RST = 46;
constexpr int8_t PIN_BL = 16;

// Use DMA-capable databus for faster bulk pixel transfers
Arduino_DataBus *bus = new Arduino_ESP32SPIDMA(PIN_DC, PIN_CS, PIN_SCLK, PIN_MOSI, PIN_MISO);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST, 1, false);

uint32_t display_width = 0;
uint32_t display_height = 0;
uint32_t frame_count = 0;
uint32_t fps_report_ms = 0;
uint16_t hue = 0;

}  // namespace

void setup() {
    Serial.begin(115200);
    delay(150);

    pinMode(PIN_BL, OUTPUT);
    digitalWrite(PIN_BL, HIGH);

    Serial.println("[gfx_benchmark_dma] calling gfx->begin()");
    if (!gfx->begin()) {
        Serial.println("[gfx_benchmark_dma] Arduino_GFX init failed");
        while (true) {
            delay(1000);
        }
    }

    Serial.println("[gfx_benchmark_dma] gfx->begin() succeeded");
    gfx->fillScreen(BLACK);

    display_width = gfx->width();
    display_height = gfx->height();
    Serial.printf("[gfx_benchmark_dma] display size: %u x %u\n", display_width, display_height);

    fps_report_ms = millis();
    Serial.println("[gfx_benchmark_dma] starting color-cycle benchmark with DMA");
}

void loop() {
    // Draw color-cycling bars to stress DMA throughput
    hue += 4;
    for (uint16_t y = 0; y < display_height; y += 32) {
        for (uint16_t x = 0; x < display_width; x += 32) {
            uint16_t h = (hue + (x >> 5) + (y >> 5)) % 360;
            // Approximate HSV to RGB for color variety
            uint8_t r = (h < 60) ? 255 : (h < 120) ? 255 - ((h - 60) * 255 / 60) : 0;
            uint8_t g = (h < 60) ? ((h) * 255 / 60) : (h < 120) ? 255 : (h < 180) ? 255 - ((h - 120) * 255 / 60) : 0;
            uint8_t b = (h < 120) ? 0 : (h < 180) ? ((h - 120) * 255 / 60) : 255;
            gfx->fillRect(x, y, 32, 32, gfx->color565(r, g, b));
        }
    }
    
    frame_count++;
    const uint32_t now_ms = millis();
    if (now_ms - fps_report_ms >= 1000) {
        Serial.printf("[gfx_benchmark_dma] FPS: %lu\n", frame_count);
        frame_count = 0;
        fps_report_ms = now_ms;
    }
}
