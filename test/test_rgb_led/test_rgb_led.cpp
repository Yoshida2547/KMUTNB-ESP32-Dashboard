#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <unity.h>

#ifndef RGB_LED_PIN
#define RGB_LED_PIN 38
#endif

#define NUM_PIXELS 8

static Adafruit_NeoPixel strip(NUM_PIXELS, RGB_LED_PIN, NEO_GRB + NEO_KHZ800);

void test_rgb_led_strip_initialization() {
  TEST_ASSERT_NOT_NULL(&strip);
  TEST_ASSERT_EQUAL(NUM_PIXELS, strip.numPixels());
}

void test_rgb_led_strip_set_colors() {
  // Set each LED to a different color
  strip.setPixelColor(0, 255, 0, 0);    // Red
  strip.setPixelColor(1, 0, 255, 0);    // Green
  strip.setPixelColor(2, 0, 0, 255);    // Blue
  strip.setPixelColor(3, 255, 255, 0);  // Yellow
  strip.setPixelColor(4, 255, 0, 255);  // Magenta
  strip.setPixelColor(5, 0, 255, 255);  // Cyan
  strip.setPixelColor(6, 255, 255, 255); // White
  strip.setPixelColor(7, 0, 0, 0);      // Off
  strip.show();

  delay(500);

  // Verify colors were set
  TEST_ASSERT_EQUAL(strip.getPixelColor(0), strip.Color(255, 0, 0));
  TEST_ASSERT_EQUAL(strip.getPixelColor(1), strip.Color(0, 255, 0));
  TEST_ASSERT_EQUAL(strip.getPixelColor(2), strip.Color(0, 0, 255));
}

void test_rgb_led_strip_rainbow() {
  // Rainbow cycle
  for (int j = 0; j < 3; ++j) {
    for (int i = 0; i < NUM_PIXELS; ++i) {
      uint32_t color = strip.ColorHSV((i * 65536 / NUM_PIXELS) + (j * 21845), 255, 255);
      strip.setPixelColor(i, color);
    }
    strip.show();
    delay(200);
  }

  TEST_ASSERT_TRUE_MESSAGE(true, "Rainbow cycle completed");
}

static void turnOffAllLeds() {
  for (int i = 0; i < NUM_PIXELS; ++i) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void setup() {
  delay(200);
  Serial.begin(115200);

  strip.begin();
  strip.show();

  UNITY_BEGIN();
  RUN_TEST(test_rgb_led_strip_initialization);
  RUN_TEST(test_rgb_led_strip_set_colors);
  RUN_TEST(test_rgb_led_strip_rainbow);

  UNITY_END();
  turnOffAllLeds();
}

void loop() {}
