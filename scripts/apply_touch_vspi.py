#!/usr/bin/env python3
"""
Apply VSPI patch to Touch.cpp for separate SPI bus
This script directly modifies Touch.cpp to use a separate VSPI bus for the touch controller
"""

import os
import sys

def apply_touch_vspi_patch(touch_cpp_path):
    """Apply VSPI modifications to Touch.cpp"""
    
    if not os.path.exists(touch_cpp_path):
        print(f"Error: Touch.cpp not found at {touch_cpp_path}")
        return False
    
    with open(touch_cpp_path, 'r') as f:
        content = f.read()
    
    # Check if already patched
    if 'static SPIClass spi_touch(VSPI)' in content:
        print("  ✓ Touch.cpp already patched with VSPI support")
        return True
    
    # 1. Add VSPI includes and definitions after the comment "Define TOUCH_CS"
    old_header = """// Define TOUCH_CS is the user setup file to enable this code

// A demo is provided in examples Generic folder"""
    
    new_header = """// Define TOUCH_CS is the user setup file to enable this code

// Separate VSPI bus for touch controller to avoid conflicts with LCD SPI bus
#include <SPI.h>

// VSPI Configuration for Touch Controller
#define VSPI_MISO  TOUCH_MISO
#define VSPI_MOSI  TOUCH_MOSI
#define VSPI_CLK   TOUCH_CLK
#define VSPI_CS    TOUCH_CS

// Create separate SPI object for touch using VSPI
static SPIClass spi_touch(VSPI);

// A demo is provided in examples Generic folder"""
    
    content = content.replace(old_header, new_header)
    
    # 2. Update begin_touch_read_write function
    old_begin = """inline void TFT_eSPI::begin_touch_read_write(void){
  DMA_BUSY_CHECK;
  CS_H; // Just in case it has been left low
  #if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS)
    if (locked) {locked = false; spi.beginTransaction(SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, SPI_MODE0));}
  #else
    spi.setFrequency(SPI_TOUCH_FREQUENCY);
  #endif
  SET_BUS_READ_MODE;
  T_CS_L;
}"""
    
    new_begin = """inline void TFT_eSPI::begin_touch_read_write(void){
  DMA_BUSY_CHECK;
  // Initialize VSPI for touch on first use
  static bool touch_spi_initialized = false;
  if (!touch_spi_initialized) {
    spi_touch.begin(VSPI_CLK, VSPI_MISO, VSPI_MOSI, VSPI_CS);
    touch_spi_initialized = true;
  }

  #if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS)
    spi_touch.beginTransaction(SPISettings(SPI_TOUCH_FREQUENCY, MSBFIRST, SPI_MODE0));
  #else
    spi_touch.setFrequency(SPI_TOUCH_FREQUENCY);
  #endif
  SET_BUS_READ_MODE;
  digitalWrite(VSPI_CS, LOW);
}"""
    
    content = content.replace(old_begin, new_begin)
    
    # 3. Update end_touch_read_write function
    old_end = """inline void TFT_eSPI::end_touch_read_write(void){
  T_CS_H;
  #if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS)
    if(!inTransaction) {if (!locked) {locked = true; spi.endTransaction();}}
  #else
    spi.setFrequency(SPI_FREQUENCY);
  #endif
  //SET_BUS_WRITE_MODE;
}"""
    
    new_end = """inline void TFT_eSPI::end_touch_read_write(void){
  digitalWrite(VSPI_CS, HIGH);

  #if defined (SPI_HAS_TRANSACTION) && defined (SUPPORT_TRANSACTIONS)
    spi_touch.endTransaction();
  #else
    // Frequency management is now handled by dedicated spi_touch object
  #endif
  //SET_BUS_WRITE_MODE;
}"""
    
    content = content.replace(old_end, new_end)
    
    # 4. Replace all spi.transfer calls with spi_touch.transfer in getTouchRaw
    content = content.replace(
        """  begin_touch_read_write();
  
  // Start YP sample request for x position, read 4 times and keep last sample
  spi.transfer(0xd0);                    // Start new YP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0xd0);                    // Read last 8 bits and start new YP conversion

  tmp = spi.transfer(0);                   // Read first 8 bits
  tmp = tmp <<5;
  tmp |= 0x1f & (spi.transfer(0x90)>>3);   // Read last 8 bits and start new XP conversion

  *x = tmp;

  // Start XP sample request for y position, read 4 times and keep last sample
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0x90);                    // Read last 8 bits and start new XP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0x90);                    // Read last 8 bits and start new XP conversion
  spi.transfer(0);                       // Read first 8 bits
  spi.transfer(0x90);                    // Read last 8 bits and start new XP conversion

  tmp = spi.transfer(0);                 // Read first 8 bits
  tmp = tmp <<5;
  tmp |= 0x1f & (spi.transfer(0)>>3);    // Read last 8 bits""",
        """  begin_touch_read_write();

  // Start YP sample request for x position, read 4 times and keep last sample
  spi_touch.transfer(0xd0);                    // Start new YP conversion
  spi_touch.transfer(0);                       // Read first 8 bits
  spi_touch.transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  spi_touch.transfer(0);                       // Read first 8 bits
  spi_touch.transfer(0xd0);                    // Read last 8 bits and start new YP conversion
  spi_touch.transfer(0);                       // Read first 8 bits
  spi_touch.transfer(0xd0);                    // Read last 8 bits and start new YP conversion

  tmp = spi_touch.transfer(0);                   // Read first 8 bits
  tmp = tmp <<5;
  tmp |= 0x1f & (spi_touch.transfer(0x90)>>3);   // Read last 8 bits and start new XP conversion

  *x = tmp;

  // Start XP sample request for y position, read 4 times and keep last sample
  spi_touch.transfer(0);                       // Read first 8 bits
  spi_touch.transfer(0x90);                    // Read last 8 bits and start new XP conversion
  spi_touch.transfer(0);                       // Read first 8 bits
  spi_touch.transfer(0x90);                    // Read last 8 bits and start new XP conversion
  spi_touch.transfer(0);                       // Read first 8 bits
  spi_touch.transfer(0x90);                    // Read last 8 bits and start new XP conversion

  tmp = spi_touch.transfer(0);                 // Read first 8 bits
  tmp = tmp <<5;
  tmp |= 0x1f & (spi_touch.transfer(0)>>3);    // Read last 8 bits"""
    )
    
    # 5. Replace spi calls in getTouchRawZ
    content = content.replace(
        """  begin_touch_read_write();

  // Z sample request
  int16_t tz = 0xFFF;
  spi.transfer(0xb0);               // Start new Z1 conversion
  tz += spi.transfer16(0xc0) >> 3;  // Read Z1 and start Z2 conversion
  tz -= spi.transfer16(0x00) >> 3;  // Read Z2""",
        """  begin_touch_read_write();

  // Z sample request
  int16_t tz = 0xFFF;
  spi_touch.transfer(0xb0);               // Start new Z1 conversion
  tz += spi_touch.transfer16(0xc0) >> 3;  // Read Z1 and start Z2 conversion
  tz -= spi_touch.transfer16(0x00) >> 3;  // Read Z2"""
    )
    
    # Write the modified content back
    with open(touch_cpp_path, 'w') as f:
        f.write(content)
    
    print(f"  ✓ Successfully applied VSPI modifications to Touch.cpp")
    return True

if __name__ == "__main__":
    # Get Touch.cpp path from command line or environment
    if len(sys.argv) > 1:
        touch_cpp_path = sys.argv[1]
    else:
        print("Usage: python3 apply_touch_vspi.py <path_to_touch_cpp>")
        sys.exit(1)
    
    if apply_touch_vspi_patch(touch_cpp_path):
        sys.exit(0)
    else:
        sys.exit(1)
