--
Skill: Build-Upload-Monitor
Related skill: agent-customization
Scope: workspace
--

Purpose
-------
Provide a compact, reproducible workflow for building, uploading, and monitoring the KMUTNB ESP32 firmware in this repository to verify changes on-device.

When to use
-----------
- After code changes that affect firmware behavior (drivers, sensors, UI).
- When adding or verifying touch/display calibration changes.

Step-by-step Workflow
---------------------
1. Select the environment
   - Decide which PlatformIO environment to use (default: `env:KMUTNB-ESP32-V1`).
   - If unsure, use `platformio.ini` `default_envs` or ask the repo owner.

2. Build
   - Preferred: run the workspace `platformio` wrapper if available on PATH.
   - Example command:

     ```bash
     platformio run -e KMUTNB-ESP32-V1
     ```

   - Fallback (if `platformio` not on PATH): use the project-specific Python venv path shown in the repo docs, e.g.:

     ```bash
     /home/surasak/.platformio/penv/bin/platformio run -e KMUTNB-ESP32-V1
     ```

3. Upload
   - Auto-detect port (recommended):

     ```bash
     platformio run -e KMUTNB-ESP32-V1 -t upload
     ```

   - Specify a port when necessary:

     ```bash
     platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-port /dev/ttyUSB0
     ```

4. Monitor (serial)
   - Use PlatformIO device monitor to inspect runtime logs at 115200 baud (project default):

     ```bash
     platformio device monitor --baud 115200
     ```

   - Or combine upload + monitor in one command (auto-detect):

     ```bash
     platformio run -e KMUTNB-ESP32-V1 -t upload --monitor-speed 115200
     ```

Decision Points & Branching
---------------------------
- If build fails: capture and attach `platformio` output; check `platformio.ini` environment flags and include paths.
- If upload fails: check device permissions (use `ls /dev/ttyUSB*`), ensure the board is in bootloader mode, and try resetting.
- If serial output is empty: validate the baud rate, wiring, and that the firmware reached `setup()` (look for early initialization logs).

Quality Criteria / Completion Checks
----------------------------------
- Build completes without errors for the chosen environment.
- Upload completes and device reboots into the new firmware.
- Serial monitor shows expected startup logs (e.g., sensor init messages, LVGL init) within a few seconds.
- Touch/display calibration changes are reflected on-screen and touch input responds.

Troubleshooting Tips
--------------------
- If PlatformIO is missing, use the explicit path in the project README or install PlatformIO Core.
- For permission errors on Linux, add your user to the `dialout` group or use `sudo` only for troubleshooting.
- For flaky USB connections, try a different cable or port and disable other serial monitors.

Examples of Useful Prompts For Automation
---------------------------------------
- "Build and upload `env:KMUTNB-ESP32-V1`, then open serial monitor at 115200."
- "Run the `Touch_calibrate` environment and save the resulting calibration to `data/touch_data.json`."
- "After upload, capture the first 30 seconds of serial output and save to `logs/serial_startup.log`."

How to Iterate
--------------
1. Make a single focused change (e.g., touch calibration or a sensor read).
2. Build and upload using the steps above.
3. Monitor serial; if behavior is wrong, collect logs and revert to narrow the cause.
4. Repeat until quality criteria pass.

Notes for Agents
----------------
- This skill is workspace-scoped: it references project-specific commands and the PlatformIO environment documented in the repo.
- Agents running these steps should confirm device availability before attempting upload.
- If `platformio` is not available, the agent should report the fallback path and ask whether to install PlatformIO or proceed using the fallback.

Suggested follow-ups / Next skills
--------------------------------
- `touch-calibration-save`: run calibration, parse output, and update `data/touch_data.json` automatically.
- `ci-flash-and-smoke-test`: CI-style skill to flash firmware and run quick self-tests via serial commands.

--
Generated: May 4, 2026
