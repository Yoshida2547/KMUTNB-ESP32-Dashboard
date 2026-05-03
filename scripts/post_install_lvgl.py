Import("env")
import os
import shutil

PROJECT_DIR = env.get('PROJECT_DIR')
PIOENV = env.get('PIOENV')
SCRIPTS_DIR = os.path.join(PROJECT_DIR, 'scripts')
LIBDEPS_DIR = os.path.join(PROJECT_DIR, '.pio', 'libdeps', PIOENV)

print(f"\n[POST_INSTALL_LVGL] Script loaded for environment: {PIOENV}")
print(f"[POST_INSTALL_LVGL] Project directory: {PROJECT_DIR}")


def find_library(lib_name):
    if not os.path.exists(LIBDEPS_DIR):
        return None

    for item in os.listdir(LIBDEPS_DIR):
        if lib_name in item:
            lib_path = os.path.join(LIBDEPS_DIR, item)
            if os.path.isdir(lib_path):
                return lib_path
    return None


def copy_lvgl_config():
    print("\n" + "=" * 70)
    print("LVGL CONFIGURATION SETUP")
    print("=" * 70)

    lvgl_lib = find_library('lvgl')
    if not lvgl_lib:
        print("⚠️  lvgl library not found in libdeps!")
        print("=" * 70 + "\n")
        return

    src_path = os.path.join(SCRIPTS_DIR, 'lv_conf.h')
    dst_path = os.path.join(lvgl_lib, 'lv_conf.h')
    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
    shutil.copy2(src_path, dst_path)
    with open(dst_path, 'r', encoding='utf-8') as handle:
        config_text = handle.read()
    config_text = config_text.replace('#define LV_USE_TFT_ESPI         1', '#define LV_USE_TFT_ESPI         0')
    with open(dst_path, 'w', encoding='utf-8') as handle:
        handle.write(config_text)
    print(f"✓ Copied lv_conf.h to {dst_path}")
    print("=" * 70 + "\n")


copy_lvgl_config()
