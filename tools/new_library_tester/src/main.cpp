#include <Arduino.h>
#include <Arduino_GFX_Library.h>

namespace {

constexpr int8_t PIN_MISO = 47;
constexpr int8_t PIN_MOSI = 11;
constexpr int8_t PIN_SCLK = 10;
constexpr int8_t PIN_CS = 45;
constexpr int8_t PIN_DC = 7;
constexpr int8_t PIN_RST = 46;
constexpr int8_t PIN_BL = 16;

Arduino_DataBus *bus = new Arduino_ESP32SPIDMA(PIN_DC, PIN_CS, PIN_SCLK, PIN_MOSI, PIN_MISO);
Arduino_GFX *gfx = new Arduino_ILI9488_18bit(bus, PIN_RST, 1, false);

uint16_t hue = 0;
uint32_t frame_count = 0;
uint32_t report_start_ms = 0;
uint64_t frame_time_sum_us = 0;

void drawStatus() {
  gfx->setTextColor(WHITE);
  gfx->setCursor(14, 16);
  gfx->setTextSize(2);
  gfx->println("Arduino_GFX OK");

  gfx->setCursor(14, 44);
  gfx->setTextSize(1);
  gfx->println("ILI9488 on HSPI (DMA)");
  gfx->println("SCK=10 MOSI=11 MISO=47");
  gfx->println("CS=45 DC=7 RST=46");
}

}  // namespace

void setup() {
  Serial.begin(115200);

  pinMode(PIN_BL, OUTPUT);
  digitalWrite(PIN_BL, HIGH);

  if (!gfx->begin()) {
    Serial.println("[new_library_tester] Arduino_GFX init failed");
    while (true) {
      delay(1000);
    }
  }

  gfx->fillScreen(BLACK);
  drawStatus();
  report_start_ms = millis();
  Serial.println("[new_library_tester] Arduino_GFX initialized with SPI DMA");
}

void loop() {
  const uint32_t frame_start_us = micros();

  // Color-cycle strip confirms continuous rendering without touch/LVGL dependencies.
  const uint16_t barY = 110;
  const uint16_t barH = 40;
  const uint16_t width = gfx->width();

  for (uint16_t x = 0; x < width; x++) {
    uint16_t color = gfx->color565((x + hue) & 0xFF, (x * 2 + hue) & 0xFF, (255 - x + hue) & 0xFF);
    gfx->drawFastVLine(x, barY, barH, color);
  }

  hue += 3;
  gfx->setTextColor(WHITE, BLACK);
  gfx->setCursor(14, 162);
  gfx->setTextSize(1);
  gfx->printf("tick: %lu    ", millis() / 1000UL);

  const uint32_t frame_time_us = micros() - frame_start_us;
  frame_count++;
  frame_time_sum_us += frame_time_us;

  const uint32_t now_ms = millis();
  const uint32_t elapsed_ms = now_ms - report_start_ms;
  if (elapsed_ms >= 1000) {
    const uint32_t fps_x10 = (frame_count * 10000UL) / elapsed_ms;
    const uint32_t avg_frame_us = frame_time_sum_us / frame_count;

    gfx->setTextColor(CYAN, BLACK);
    gfx->setCursor(14, 182);
    gfx->printf("fps: %lu.%lu    ", fps_x10 / 10, fps_x10 % 10);

    gfx->setTextColor(YELLOW, BLACK);
    gfx->setCursor(14, 200);
    gfx->printf("frame: %lu.%02lu ms    ", avg_frame_us / 1000, (avg_frame_us % 1000) / 10);

    Serial.printf("[perf] fps=%lu.%lu avg_frame=%lu us\n", fps_x10 / 10, fps_x10 % 10, avg_frame_us);

    frame_count = 0;
    frame_time_sum_us = 0;
    report_start_ms = now_ms;
  }

  delay(80);
}
