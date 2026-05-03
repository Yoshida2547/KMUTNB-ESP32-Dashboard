# Touch.cpp VSPI Patch Documentation

## Overview
This patch modifies `Touch.cpp` to use a separate VSPI (Virtual SPI) bus instead of sharing the same SPI bus with the LCD controller. This eliminates potential bus conflicts and improves reliability of touch input.

## Automatic Application via post_install.py

The patch is **automatically applied** during PlatformIO build when using the integrated `post_install.py` script. No manual action required!

✅ The patch application is integrated into the post-installation setup
✅ Automatic detection of Touch.cpp location
✅ Graceful error handling if patch utility is unavailable
✅ Detailed console output showing patch application status

## Changes Made

### 1. **Separate SPI Bus for Touch**
   - Creates a new `SPIClass spi_touch(VSPI)` object
   - Uses the already-defined touch pins: `TOUCH_CLK`, `TOUCH_MOSI`, `TOUCH_MISO`, `TOUCH_CS`
   - Initializes VSPI on first use for lazy initialization

### 2. **Modified Functions**

#### `begin_touch_read_write()`
   - Initializes VSPI bus with touch-specific pins on first call
   - Sets up SPI transaction with `SPI_TOUCH_FREQUENCY` (2.5 MHz)
   - Uses `digitalWrite()` for CS pin control instead of macro

#### `end_touch_read_write()`
   - Properly ends SPI transaction
   - Releases the VSPI bus

### 3. **SPI Calls Replacement**
   - All `spi.transfer()` calls → `spi_touch.transfer()`
   - All `spi.transfer16()` calls → `spi_touch.transfer16()`
   - Updated in functions: `getTouchRaw()`, `getTouchRawZ()`

## Hardware Configuration

Based on `Setup21_ILI9488.h`:

| Signal | Pin | Bus |
|--------|-----|-----|
| LCD SCLK | 10 | HSPI |
| LCD MOSI | 11 | HSPI |
| LCD MISO | 47 | HSPI |
| LCD CS | 45 | HSPI |
| **Touch CLK** | **4** | **VSPI** |
| **Touch MOSI** | **6** | **VSPI** |
| **Touch MISO** | **5** | **VSPI** |
| **Touch CS** | **17** | **VSPI** |

## Manual Application (if needed)

### Option 1: Using patch command
```bash
cd /path/to/TFT_eSPI/Extensions
patch -p1 < /path/to/Touch_separate_spi.patch
```

### Option 2: Manual Application
1. Open `Touch.cpp` in the TFT_eSPI library
2. Add the following at the top after includes:
   ```cpp
   #include <SPI.h>
   
   #define VSPI_MISO  TOUCH_MISO
   #define VSPI_MOSI  TOUCH_MOSI
   #define VSPI_CLK   TOUCH_CLK
   #define VSPI_CS    TOUCH_CS
   
   static SPIClass spi_touch(VSPI);
   ```

3. Update `begin_touch_read_write()` and `end_touch_read_write()` functions as shown in the patch
4. Replace all `spi.` calls with `spi_touch.` in touch functions

## Benefits

✅ **Eliminates Bus Conflicts**: LCD and touch use independent SPI buses  
✅ **Improved Reliability**: No contention between LCD and touch operations  
✅ **Better Performance**: Touch reads can happen concurrently with LCD updates  
✅ **Cleaner Separation**: Touch bus completely independent from LCD bus  
✅ **Automatic Application**: Integrated into post-installation script  

## Notes

- The patch uses lazy initialization (VSPI initialized on first touch read)
- CS pin control uses `digitalWrite()` instead of macros for reliability
- Compatible with both transactional and non-transactional SPI modes
- Touch frequency remains at 2.5 MHz as before
- Requires `patch` utility to be installed on the system (included on Linux/macOS)

## Troubleshooting

### "patch command not found"
Install the patch utility:
- **Ubuntu/Debian**: `sudo apt-get install patch`
- **macOS**: `brew install gpatch`
- **Windows**: Use Git Bash or install GnuWin32 patch

### Patch already applied
If you see "Hunk already applied", the patch was successfully applied in a previous build. This is expected and harmless.

### Manual rollback

To revert the changes:
```bash
cd /path/to/TFT_eSPI/Extensions
patch -p1 -R < Touch_separate_spi.patch
```

