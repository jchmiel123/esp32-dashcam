# CLAUDE.md

This file provides guidance to Claude Code when working with this repository.

## Project Overview

ESP32-S3 dashcam firmware for the Waveshare ESP32-S3-Touch-LCD-3.5B-C board. Continuous MJPEG AVI video recording with live LCD preview.

**Status:** v0.6 — Continuous MJPEG recording, dual-core architecture, working on hardware (2026-04-10)
**Framework:** Arduino via PlatformIO

## Hardware — Waveshare ESP32-S3-Touch-LCD-3.5B-C

- **MCU:** ESP32-S3R8 (240MHz, 8MB OPI PSRAM, 16MB Flash)
- **Camera:** OV5640 5MP autofocus (DVP interface)
- **Display:** 3.5" IPS 320x480, QSPI AXS15231B controller
- **PMIC:** AXP2101 (LiPo charging, BLDO1/BLDO2 power rails)
- **IO Expander:** TCA9554 at 0x20 (LCD reset on P1)
- **SD:** SD_MMC 1-bit mode (CLK=11, CMD=10, D0=9) — FAT32 only, not exFAT
- **Audio:** ES8311 codec (not used yet)
- **RTC:** PCF85063 (not used yet)
- **IMU:** QMI8658 (not used yet)
- **Button:** BOOT (GPIO 0)

### Critical Hardware Details (VERIFIED)

- **PMU must init before camera**: AXP2101 BLDO1(1.5V) + BLDO2(2.8V) power the OV5640
- **Canvas rotation MUST be 0**: Direct framebuffer writes only work with rotation=0. Rotation=1 only affects drawPixel/text, not buffer access. The QSPI flush breaks with MADCTL MV bit.
- **Camera byte order**: DVP outputs big-endian RGB565. Canvas expects little-endian. Byte swap `(px >> 8) | (px << 8)` required for raw RGB565.
- **JPEG decode byte order**: `fmt2rgb888` outputs BGR888 (not RGB). TJpgDec ROM outputs RGB888 (standard).
- **Camera orientation**: DVP wiring naturally transposes — no software rotation needed. VGA (640x480) center-crop to 320x480 fills the portrait display.
- **Sensor vflip**: `set_vflip(s, 1)` required after every camera init for correct orientation.
- **SD_MMC 1-bit mode**: `SD_MMC.begin("/sdcard", true)` — the `true` flag is critical.
- **PSRAM bus contention**: Both cores share one OPI PSRAM bus. JPEG decode on Core 0 slows 2-4x when Core 1 is actively writing AVI frames. Mitigated by copying JPEG to SRAM before decode.

## Architecture (v0.6)

### Dual-Core FreeRTOS Design

| Core | Task | Priority | What It Does |
|------|------|----------|--------------|
| Core 0 | Main loop | 1 | JPEG decode → LCD preview + HUD overlay |
| Core 1 | recordingTask | 2 | JPEG capture → MJPEG AVI files on SD |

### Data Flow
```
Camera (JPEG VGA 8fps) → Core 1 → AVI file on SD (/DCIM/video/)
                             ↓
                     shared preview buffer (PSRAM, mutex-protected)
                             ↓
                     Core 0 → SRAM copy → TJpgDec decode → canvas → LCD
```

### MJPEG AVI Writer
- Custom `AVIWriter` class writes valid RIFF AVI with MJPG codec
- 1-minute segments: `REC_{uptime_seconds}.avi`
- Headers patched on segment close (RIFF size, frame count, movi size)
- Frame index (idx1) stored in PSRAM, written at close
- Auto-cleanup: deletes oldest video when free space < 500MB

### Preview Pipeline
- TJpgDec from ESP32-S3 ROM — no library needed, `#include <rom/tjpgd.h>`
- Two quality modes (toggled by button):
  - **HQ (1/2 scale)**: 640x480 → 320x240, row-doubled to 320x480. ~2 FPS.
  - **FAST (1/4 scale)**: 640x480 → 160x120, 2x horiz + 4x vert. ~4+ FPS.
- JPEG source copied PSRAM → SRAM before decode to avoid bus contention
- Work buffer in SRAM (not PSRAM) for decode performance

### Button Functions
| Action | How | What |
|--------|-----|------|
| Toggle preview quality | Single press | Switches between HQ (1/2) and FAST (1/4) scale |
| Toggle recording | Double press | Pauses/resumes AVI recording (preview continues) |
| Wipe SD card | Hold 3s, release, hold 3s again | Deletes all videos and photos |

### HUD Overlay (3 rows, top of screen)
1. Recording status: `REC 123 frm 8fps` / `PAUSED` / `STARTING...`
2. Battery: `BAT 85% 4.12V CHG` or `USB PWR`
3. Storage: `31.8GB 00:05:23` (free space + uptime)
4. Bottom: `LCD:2fps HQ` (preview FPS + scale mode)

## Build & Flash

```bash
cd tools/sd_reader
pio run -t upload          # Build and flash (COM15)
pio device monitor         # Serial monitor (115200)
```

### platformio.ini Key Settings
```ini
platform = pioarduino (ESP32 v51.03.07)
board = esp32-s3-devkitc-1
board_build.arduino.memory_type = qio_opi
board_build.flash_size = 16MB
board_build.psram_type = opi
build_flags = -DARDUINO_USB_CDC_ON_BOOT=1 -DBOARD_HAS_PSRAM -O2
lib_deps = GFX Library for Arduino@1.5.0, XPowersLib@^0.2.6
```

### Serial Monitor Notes
- USB CDC resets on DTR toggle — use `DtrEnable = $false` when reading without reset
- Boot messages: PMU OK → Display OK → SD OK → PSRAM → Camera → Recording
- Periodic: `[LCD] 2.1 fps (dec=392ms) | [REC] 7.9 fps | scale=1`
- Segment transitions: `[REC] Segment done: 474 frames, 7.9 FPS avg`

## File Layout on SD Card
```
/DCIM/
  /video/     REC_7.avi, REC_67.avi, ...  (1-min MJPEG segments)
  /photos/    (reserved for future use)
```

## Key Files
```
tools/sd_reader/src/main.cpp       — THE firmware (single file, ~800 lines)
tools/sd_reader/platformio.ini     — Build config
tools/format_fat32.bat             — Format SD as FAT32 (diskpart)
tools/format_fat32.ps1             — Format SD as FAT32 (PowerShell)
CLAUDE.md                          — This file
```

## Performance Characteristics
| Metric | Value |
|--------|-------|
| Recording FPS | 7.4–7.9 (target 8) |
| Preview FPS (HQ) | 1–2 FPS |
| Preview FPS (FAST) | 3–5 FPS (estimated) |
| JPEG decode time | 400–1600ms (varies with PSRAM contention) |
| AVI segment size | ~18MB/minute at VGA quality 12 |
| Recording capacity | ~30 hours on 32GB FAT32 |
| PSRAM free | ~7.7MB (of 8MB) |
| SRAM free | ~300KB (of 320KB) |
| Boot time | ~5 seconds |

## Known Limitations
- **AVI not crash-safe**: Power loss mid-segment leaves headers unpatched (file unplayable). Could add periodic header flush.
- **No RTC timestamps**: Filenames use `millis()` uptime, not real time. PCF85063 RTC is present but unused.
- **No audio**: ES8311 codec available but not integrated.
- **PSRAM bus contention**: Preview FPS varies 0.5–2.5 depending on SD write activity.
- **Preview colors**: TJpgDec ROM outputs RGB; verified correct. If colors look wrong, the byte order in `jpgWriteCb` is the place to check.

## Future Phases
- [ ] PCF85063 RTC integration for real timestamps
- [ ] QMI8658 IMU for impact/G-force detection
- [ ] ES8311 audio recording
- [ ] WiFi AP + HTTP file browser for downloading clips
- [ ] AVI crash recovery (periodic header flush)
- [ ] Touch screen UI for playback/settings
