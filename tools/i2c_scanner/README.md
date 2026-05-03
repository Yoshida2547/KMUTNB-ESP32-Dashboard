# I2C Scanner Tool

A PlatformIO utility tool for scanning and detecting I2C slave devices on the ESP32.

## Usage

### Build
```bash
platformio run -e esp32-s3-devkitc-1-i2c-scanner
```

### Upload
```bash
platformio run -e esp32-s3-devkitc-1-i2c-scanner -t upload
```

### Monitor
```bash
platformio device monitor
```

## Features

- Automatically scans I2C bus every 5 seconds
- Press 's' in serial monitor to trigger manual scan
- Displays all found devices in decimal and hexadecimal format
- Configurable I2C pins (SDA, SCL)
- Configurable clock frequency

## Example Output

```
=== I2C Device Scan ===
Found 4 device(s):
Address | Hex
--------|-----
41      | 0x29
64      | 0x40
83      | 0x53
92      | 0x5C
=======================
```

## Configuration

Edit `tools/i2c_scanner/main.cpp` to modify:
- `I2C_SDA_PIN`: SDA pin (default: SDA)
- `I2C_SCL_PIN`: SCL pin (default: SCL)
- Scan interval in the loop delay

See `include/i2c_scanner.h` for the reusable library API.
