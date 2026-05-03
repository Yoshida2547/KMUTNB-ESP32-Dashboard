#include <Arduino.h>
#include <Wire.h>

#ifndef I2C_SDA_PIN
#define I2C_SDA_PIN SDA
#endif

#ifndef I2C_SCL_PIN
#define I2C_SCL_PIN SCL
#endif

// I2C Scanner implementation
class I2CScanner {
 public:
  static void begin(int sda_pin = SDA, int scl_pin = SCL, uint32_t frequency = 100000) {
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(frequency);
  }

  static uint8_t scan(uint8_t addresses[], uint8_t max_devices = 128) {
    uint8_t count = 0;
    
    for (uint8_t address = 0x08; address <= 0x77; address++) {
      Wire.beginTransmission(address);
      uint8_t error = Wire.endTransmission();
      
      if (error == 0) {
        if (count < max_devices) {
          addresses[count++] = address;
        }
      }
    }
    
    return count;
  }

  static void printDevices() {
    uint8_t addresses[128];
    uint8_t count = scan(addresses);
    
    Serial.println("\n=== I2C Device Scan ===");
    Serial.print("Found ");
    Serial.print(count);
    Serial.println(" device(s):");
    
    if (count == 0) {
      Serial.println("No devices found.");
      return;
    }
    
    Serial.println("Address | Hex");
    Serial.println("--------|-----");
    
    for (uint8_t i = 0; i < count; i++) {
      uint8_t addr = addresses[i];
      Serial.print(addr);
      Serial.print("      | 0x");
      if (addr < 0x10) Serial.print("0");
      Serial.println(addr, HEX);
    }
    
    Serial.println("=======================\n");
  }
};

static void scanI2C() {
  I2CScanner::printDevices();
}

void setup() {
  Serial.begin(115200);
  delay(200);

  I2CScanner::begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.println("\n=== I2C Scanner Tool ===");
  Serial.println("Scanning for I2C devices...");
  scanI2C();
}

void loop() {
  // Check for serial input (press 's' to scan)
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      Serial.println("Scan triggered...");
      scanI2C();
    }
  }
  delay(5000);
  scanI2C();
}
