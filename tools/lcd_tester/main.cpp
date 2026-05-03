#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define TFT_LED_PIN 16

void setup() {
  Serial.begin(115200);
  tft.init();

  pinMode(TFT_LED_PIN, OUTPUT);
  digitalWrite(TFT_LED_PIN, HIGH); // Turn on the backlight

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Hello, World!");
}

void loop() {
  Serial.println("Hello, World!");
  delay(1000);
}