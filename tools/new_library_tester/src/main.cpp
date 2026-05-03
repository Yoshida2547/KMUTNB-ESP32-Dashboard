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
  Serial.println("[new_library_tester] Arduino_GFX initialized with SPI DMA");
}

void loop() {
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

  delay(80);
}
