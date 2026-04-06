#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Pin Definitions — Seeed XIAO ESP32S3 Sense
// Ref: https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/
// ─────────────────────────────────────────────────────────────────────────────

#ifdef PLATFORM_HINT_XIAO

// ── Camera (OV2640 on Sense expansion board) ─────────────────────────────────
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    10
#define CAM_PIN_SIOD    40      // SCCB SDA
#define CAM_PIN_SIOC    39      // SCCB SCL
#define CAM_PIN_D7      48
#define CAM_PIN_D6      11
#define CAM_PIN_D5      12
#define CAM_PIN_D4      14
#define CAM_PIN_D3      16
#define CAM_PIN_D2      18
#define CAM_PIN_D1      17
#define CAM_PIN_D0      15
#define CAM_PIN_VSYNC   38
#define CAM_PIN_HREF    47
#define CAM_PIN_PCLK    13

// ── SD Card (SPI on Sense expansion board) ───────────────────────────────────
#define SD_PIN_CS       21
#define SD_PIN_MOSI     9       // D9
#define SD_PIN_MISO     8       // D8
#define SD_PIN_SCK      7       // D7

// ── I2C (main bus) ───────────────────────────────────────────────────────────
#define I2C_SDA         5       // D4
#define I2C_SCL         6       // D5

// ── Button ───────────────────────────────────────────────────────────────────
#define BUTTON_PIN      1       // D0 — external momentary pushbutton

// ── Microphone (onboard PDM) ─────────────────────────────────────────────────
#define MIC_PDM_CLK     42
#define MIC_PDM_DATA    41

#endif // PLATFORM_HINT_XIAO
