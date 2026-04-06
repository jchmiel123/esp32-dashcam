# ESP32-S3 Dashcam System — Project Specification

## Overview

A DIY always-on dashcam built on ESP32-S3 hardware, recording continuous looping video to microSD with timestamp overlay, impact detection, and wireless clip retrieval. A single physical button toggles the system between active recording and standby. The firmware is a unified codebase that auto-detects which hardware platform it's running on at boot and configures itself accordingly.

---

## Hardware Platforms

### Platform A — Seeed Studio XIAO ESP32S3 Sense (Prototype / Secondary Unit)

- **SoC:** ESP32-S3R8 — dual-core Xtensa LX7 @ 240 MHz
- **Memory:** 8 MB PSRAM, 8 MB Flash
- **Camera:** OV2640 (detachable), max 1600x1200
- **Audio:** Onboard digital microphone
- **Storage:** MicroSD card slot
- **Connectivity:** 2.4 GHz Wi-Fi (802.11 b/g/n), BLE 5.0
- **Power:** USB-C, battery charge circuit (MX1.25 header)
- **Form Factor:** 21 x 17.5 mm — thumb-sized

### Platform B — Waveshare ESP32-S3-Touch-LCD-3.5B-C (Primary Unit)

- **SoC:** ESP32-S3R8 — dual-core Xtensa LX7 @ 240 MHz
- **Memory:** 8 MB PSRAM, 16 MB Flash
- **Camera:** OV5640 5MP autofocus (included)
- **Display:** 3.5" IPS capacitive touch, 320x480, QSPI (AXS15231B driver)
- **Audio:** ES8311 audio codec (record + playback)
- **Storage:** MicroSD (TF) card slot
- **RTC:** PCF85063 — battery-backed via AXP2101 PMIC (backup battery header)
- **IMU:** QMI8658 — 3-axis accelerometer + 3-axis gyroscope
- **Power Management:** AXP2101 PMIC, MX1.25 LiPo charge/discharge header
- **Connectivity:** 2.4 GHz Wi-Fi (802.11 b/g/n), BLE 5.0
- **Form Factor:** 3.5" cased unit with touchscreen

---

## Runtime Hardware Detection

At boot the firmware probes I2C for the following peripherals:

| I2C Device   | Address | Present = Platform B | Absent = Platform A |
|--------------|---------|----------------------|---------------------|
| QMI8658 IMU  | 0x6B    | Enable impact detection | Skip |
| PCF85063 RTC | 0x51    | Use hardware RTC | Use NTP fallback |
| AXP2101 PMIC | 0x34    | Enable battery monitoring | Skip |
| ES8311 Codec | 0x18    | Enable audio codec | Use PDM mic |
| AXS15231B Display | (QSPI) | Enable touch UI | Headless |

---

## Single-Button Operation

| Action | Behavior |
|--------|----------|
| Power on | Boot directly into active recording |
| Single press | Mark event — protect current + previous segment |
| Double press | Toggle Wi-Fi AP on/off |
| Long press (3s) | Enter standby / wake |

---

## Recording Behavior

### Continuous Loop Recording
- MJPEG frames in AVI container to microSD
- 1-minute segment files: `REC_YYYYMMDD_HHMMSS.avi`
- Platform A: 800x600 @ 10-12 fps
- Platform B: 1280x720 @ 10-15 fps
- Auto-delete oldest unprotected at 80% SD usage

### Event Protection
- Button press or IMU impact (>2G) marks current + previous segment
- Protected files renamed `EVT_` prefix, moved to `/events/`
- Never auto-deleted

---

## Wi-Fi / Clip Retrieval

- SSID: `DASHCAM-XXXX` (last 4 of MAC)
- Default password: `dashcam1234`
- HTTP server on `192.168.4.1`
- Endpoints: live preview, clip list, download, status, settings
- Auto-disable after 5 min inactivity

---

## Display UI (Platform B Only)

- Default: Live viewfinder with recording indicator, timestamp, SD %, battery
- Swipe left: Event log
- Swipe right: Settings
- Tap center: Mark event

---

## File System Layout (MicroSD)

```
/DCAM/
  /segments/     — continuous recordings, auto-deleted oldest first
  /events/       — protected clips, never auto-deleted
  /config/       — settings.json
  /log/          — boot log, errors
```
