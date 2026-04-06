# CLAUDE.md

This file provides guidance to Claude Code when working with this repository.

## Project Overview

ESP32-S3 PlatformIO project for a DIY always-on dashcam. Unified codebase that auto-detects which hardware platform it's running on at boot via I2C probing.

**Status:** Phase 1 scaffolding complete — awaiting hardware for bring-up
**Framework:** Arduino via PlatformIO

## Hardware Platforms

### Platform A — Seeed XIAO ESP32S3 Sense (Prototype / Rear Camera)
- **MCU:** ESP32-S3R8 (240MHz, 8MB PSRAM, 8MB Flash)
- **Camera:** OV2640 (detachable), max 1600x1200
- **Audio:** Onboard PDM microphone
- **Storage:** MicroSD (SPI)
- **No display** — headless operation

### Platform B — Waveshare ESP32-S3-Touch-LCD-3.5B-C (Primary Unit)
- **MCU:** ESP32-S3R8 (240MHz, 8MB PSRAM, 16MB Flash)
- **Camera:** OV5640 5MP autofocus
- **Display:** 3.5" IPS capacitive touch, 320x480, QSPI (AXS15231B)
- **Audio:** ES8311 codec
- **RTC:** PCF85063 (battery-backed)
- **IMU:** QMI8658 (accelerometer + gyroscope)
- **PMIC:** AXP2101

### I2C Device Addresses
- AXP2101 PMU: 0x34
- ES8311 Audio: 0x18
- PCF85063 RTC: 0x51
- QMI8658 IMU: 0x6B

## Build Commands

```bash
# PlatformIO path (if not in PATH)
/c/Users/jchmiel/.platformio/penv/Scripts/pio.exe

# Build for XIAO
pio run -e xiao-s3

# Build for Waveshare
pio run -e waveshare-lcd35

# Upload + monitor
pio run -e xiao-s3 -t upload && pio device monitor
pio run -e waveshare-lcd35 -t upload && pio device monitor
```

## Architecture

### Hardware Abstraction Layer (HAL)
- `HAL::instance()` singleton detects platform at boot via I2C probing
- Each subsystem has a virtual interface with platform-specific implementations
- `PlatformCaps` struct tracks which hardware is present
- Check `hal.hasDisplay()`, `hal.hasIMU()` before using optional hardware

### EventBus
- Pub/sub system for decoupled inter-module communication
- Button → BUTTON_PRESS → Recorder (protect clip)
- IMU → IMPACT_DETECTED → Recorder (protect clip)
- Double press → BUTTON_DOUBLE_PRESS → WiFi (toggle AP)
- SD low → SD_SPACE_LOW → FileManager (cleanup)

### Recording Pipeline
- Camera → JPEG → MJPEGWriter (AVI container) → SD card
- 1-minute segment files, named by timestamp
- MJPEGWriter writes proper AVI RIFF headers
- FileManager handles rotation and auto-cleanup at 80% SD usage

### File Layout on SD
```
/DCAM/
  /segments/    REC_YYYYMMDD_HHMMSS.avi  (auto-deleted oldest first)
  /events/      EVT_YYYYMMDD_HHMMSS.avi  (protected, never auto-deleted)
  /config/      settings.json
  /log/         boot.log
```

### Single Button
- Press (<300ms): Mark event — protect current + previous clip
- Double press: Toggle WiFi AP
- Long press (>3s): Enter/exit standby

## Key Files
```
platformio.ini              — Dual build environments (xiao-s3, waveshare-lcd35)
include/config.h            — All constants and thresholds
include/pins_xiao.h         — XIAO pin map
include/pins_waveshare.h    — Waveshare pin map (TBD placeholders)
include/hal/HAL.h           — Central singleton, platform detection
include/hal/Platform.h      — Detection logic, capabilities struct
include/core/EventBus.h     — Event system (fully implemented)
include/recording/MJPEGWriter.h  — AVI file writer (fully implemented)
include/recording/Recorder.h     — Recording state machine
include/input/ButtonHandler.h    — Multi-function button (fully implemented)
include/storage/FileManager.h    — File naming, rotation, cleanup
include/network/WiFiAP.h        — Wi-Fi AP management
include/network/WebServer.h     — HTTP clip browser
src/main.cpp                     — Entry point and main loop
```

## Libraries
- `lewisxhe/XPowersLib` — AXP2101 PMIC driver
- `lewisxhe/SensorLib` — QMI8658 IMU driver
- `jchmiel123/esp32-i2c-bus` — I2C bus management
- `moononournation/GFX Library for Arduino` — Display (Waveshare only)

## Phase Status
- [x] Phase 1: Project scaffolding, architecture, interfaces
- [ ] Phase 2: Camera init + JPEG capture to SD (XIAO)
- [ ] Phase 3: Continuous recording with file rotation (XIAO)
- [ ] Phase 4: WiFi AP + HTTP file browser (XIAO)
- [ ] Phase 5: Hardware detection + platform abstraction (Both)
- [ ] Phase 6: RTC, IMU, display UI (Waveshare)
- [ ] Phase 7: Power-loss detection + graceful shutdown (Both)
- [ ] Phase 8: Timestamp overlay on frames (Both)

## Known TBDs
- Waveshare pin mappings in `pins_waveshare.h` — need schematic verification
- Display driver init in `DisplayHAL.h` — QSPI setup for AXS15231B
- Audio implementations — ES8311 codec and PDM mic I2S setup
- TimestampOverlay — currently passthrough, needs JPEG decode/encode
