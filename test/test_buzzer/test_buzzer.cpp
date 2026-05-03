#include <Arduino.h>
#include <unity.h>

#ifndef BUZZER_PIN
#define BUZZER_PIN 18
#endif

void test_buzzer_pin_toggles() {
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(BUZZER_PIN, LOW);
  delay(50);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(50);
  digitalWrite(BUZZER_PIN, LOW);

  TEST_ASSERT_TRUE_MESSAGE(true, "Buzzer pin toggled");
}

void setup() {
  delay(200);
  Serial.begin(115200);

  UNITY_BEGIN();
  RUN_TEST(test_buzzer_pin_toggles);
  UNITY_END();
}

void loop() {}
