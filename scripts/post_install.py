Import("env")
import os
import sys
import shutil
import subprocess

# Configuration
PROJECT_DIR = env.get('PROJECT_DIR')
PIOENV = env.get('PIOENV')
SCRIPTS_DIR = os.path.join(PROJECT_DIR, 'scripts')
LIBDEPS_DIR = os.path.join(PROJECT_DIR, '.pio', 'libdeps', PIOENV)

print(f"\n[POST_INSTALL] Script loaded for environment: {PIOENV}")
print(f"[POST_INSTALL] Project directory: {PROJECT_DIR}")

# ============================================================================
# Helper Functions
# ============================================================================

def find_library(lib_name):
    """Find library path by name in libdeps"""
    if not os.path.exists(LIBDEPS_DIR):
        return None
    
    for item in os.listdir(LIBDEPS_DIR):
        if lib_name in item:
            lib_path = os.path.join(LIBDEPS_DIR, item)
            if os.path.isdir(lib_path):
                return lib_path
    return None

def copy_files(files_config, lib_name):
    """Generic file copy function
    
    Args:
        files_config: List of tuples (source_filename, dest_relative_path)
        lib_name: Library name for display purposes
    
    Returns:
        Tuple (copied_count, total_count)
    """
    lib_path = find_library(lib_name)
    
    if not lib_path:
        print(f"⚠️  {lib_name} library not found in libdeps!")
        return 0, len(files_config)
    
    print(f"✓ Found {lib_name} at: {lib_path}")
    
    copied_count = 0
    for src_file, dest_rel_path in files_config:
        src_path = os.path.join(SCRIPTS_DIR, src_file)
        dst_path = os.path.join(lib_path, dest_rel_path)
        dst_dir = os.path.dirname(dst_path)
        
        if os.path.exists(src_path):
            try:
                os.makedirs(dst_dir, exist_ok=True)
                shutil.copy2(src_path, dst_path)
                print(f"  ✓ {src_file} → {dest_rel_path}")
                copied_count += 1
            except Exception as e:
                print(f"  ✗ Failed to copy {src_file}: {e}")
        else:
            print(f"  ⚠ Source not found: {src_file}")
    
    return copied_count, len(files_config)

# ============================================================================
# Library Configuration Functions
# ============================================================================

def copy_tft_espi_configs():
    """Copy TFT_eSPI configuration files"""
    print("\n" + "="*70)
    print("TFT_eSPI CONFIGURATION SETUP")
    print("="*70)
    
    files_config = [
        ('User_Setup_Select.h', 'User_Setup_Select.h'),
        ('Setup21_ILI9488.h', os.path.join('User_Setups', 'Setup21_ILI9488.h'))
    ]
    
    copied, total = copy_files(files_config, 'TFT_eSPI')
    print(f"Result: {copied}/{total} files copied")
    print("="*70 + "\n")

def copy_lvgl_configs():
    """Copy LVGL configuration files"""
    print("\n" + "="*70)
    print("LVGL CONFIGURATION SETUP")
    print("="*70)
    
    files_config = [
        ('lv_conf.h', 'lv_conf.h')
    ]
    
    copied, total = copy_files(files_config, 'lvgl')
    print(f"Result: {copied}/{total} files copied")
    print("="*70 + "\n")

def apply_touch_vspi_patch():
    """Apply Touch.cpp VSPI patch for separate SPI bus"""
    print("\n" + "="*70)
    print("TOUCH.CPP VSPI PATCH APPLICATION")
    print("="*70)
    
    tft_espi_lib = find_library('TFT_eSPI')
    
    if not tft_espi_lib:
        print("⚠️  TFT_eSPI library not found in libdeps!")
        print("="*70 + "\n")
        return False
    
    touch_cpp_path = os.path.join(tft_espi_lib, 'Extensions', 'Touch.cpp')
    apply_script = os.path.join(SCRIPTS_DIR, 'apply_touch_vspi.py')
    
    if not os.path.exists(touch_cpp_path):
        print(f"⚠️  Touch.cpp not found at: {touch_cpp_path}")
        print("="*70 + "\n")
        return False
    
    if not os.path.exists(apply_script):
        print(f"⚠️  Apply script not found at: {apply_script}")
        print("="*70 + "\n")
        return False
    
    print(f"✓ Found Touch.cpp at: {touch_cpp_path}")
    print(f"✓ Found apply script at: {apply_script}")
    
    # Execute the Python script to apply the patch
    try:
        result = subprocess.run(
            [sys.executable, apply_script, touch_cpp_path],
            capture_output=True,
            text=True,
            timeout=10
        )
        
        if result.returncode == 0:
            if result.stdout:
                for line in result.stdout.strip().split('\n'):
                    print(f"  {line}")
            print("="*70 + "\n")
            return True
        else:
            print(f"  ✗ Patch application failed: {result.stderr}")
            if result.stdout:
                print(f"  Output: {result.stdout}")
            print("="*70 + "\n")
            return False
            
    except Exception as e:
        print(f"  ✗ Error applying patch: {e}")
        print("="*70 + "\n")
        return False

# ============================================================================
# Main Execution
# ============================================================================

def execute_setup():
    """Execute configuration setup"""
    print("\n[POST_INSTALL] Starting configuration setup...")
    copy_tft_espi_configs()
    copy_lvgl_configs()
    apply_touch_vspi_patch()
    print("[POST_INSTALL] Configuration setup complete!")

# Run immediately when script loads
execute_setup()