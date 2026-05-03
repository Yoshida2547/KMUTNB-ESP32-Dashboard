#include <Arduino.h>
#include <Wire.h>
#include <unity.h>

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN SDA
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN SCL
#endif

static const uint8_t kExpectedAddresses[] = {0x29, 0x40, 0x53, 0x5C};

static void scanI2C(bool found[128]) {
  for (int i = 0; i < 128; ++i) {
    found[i] = false;
  }

  for (uint8_t address = 0x08; address <= 0x77; ++address) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      found[address] = true;
    }
  }
}

void test_expected_i2c_addresses_present() {
  bool found[128];
  scanI2C(found);

  for (size_t i = 0; i < sizeof(kExpectedAddresses); ++i) {
    uint8_t address = kExpectedAddresses[i];
    TEST_ASSERT_TRUE_MESSAGE(found[address], "Expected I2C address not found");
  }
}

void setup() {
  delay(200);
  Serial.begin(115200);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  UNITY_BEGIN();
  RUN_TEST(test_expected_i2c_addresses_present);
  UNITY_END();
}

void loop() {}
