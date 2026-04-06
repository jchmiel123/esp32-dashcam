#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Pin Definitions — Waveshare ESP32-S3-Touch-LCD-3.5B-C
// Ref: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5B-C
// NOTE: Some pins are TBD — verify with schematic when board arrives
// ─────────────────────────────────────────────────────────────────────────────

#ifdef PLATFORM_HINT_WAVESHARE

// ── Camera (OV5640 on FPC connector) ─────────────────────────────────────────
// TBD: Confirm pin mapping from Waveshare schematic
#define CAM_PIN_PWDN    -1      // TBD
#define CAM_PIN_RESET   -1      // TBD
#define CAM_PIN_XCLK    40      // TBD
#define CAM_PIN_SIOD    17      // TBD — SCCB SDA
#define CAM_PIN_SIOC    18      // TBD — SCCB SCL
#define CAM_PIN_D7      39      // TBD
#define CAM_PIN_D6      41      // TBD
#define CAM_PIN_D5      42      // TBD
#define CAM_PIN_D4      12      // TBD
#define CAM_PIN_D3      3       // TBD
#define CAM_PIN_D2      14      // TBD
#define CAM_PIN_D1      47      // TBD
#define CAM_PIN_D0      13      // TBD
#define CAM_PIN_VSYNC   21      // TBD
#define CAM_PIN_HREF    38      // TBD
#define CAM_PIN_PCLK    11      // TBD

// ── SD Card (SPI) ────────────────────────────────────────────────────────────
// TBD: Confirm from schematic
#define SD_PIN_CS       10      // TBD
#define SD_PIN_MOSI     11      // TBD
#define SD_PIN_MISO     13      // TBD
#define SD_PIN_SCK      12      // TBD

// ── I2C (main bus — AXP2101, QMI8658, PCF85063, ES8311, touch) ──────────────
// TBD: Verify — likely same as other Waveshare boards
#define I2C_SDA         7       // TBD
#define I2C_SCL         8       // TBD

// ── Display (QSPI — AXS15231B driver) ────────────────────────────────────────
// TBD: Confirm QSPI pins
#define DISP_QSPI_CS   45      // TBD
#define DISP_QSPI_SCK  47      // TBD
#define DISP_QSPI_D0   21      // TBD
#define DISP_QSPI_D1   48      // TBD
#define DISP_QSPI_D2   40      // TBD
#define DISP_QSPI_D3   39      // TBD
#define DISP_RST        -1      // TBD
#define DISP_BL         -1      // TBD — backlight (may be PMIC controlled)

// ── Touch (capacitive, I2C) ──────────────────────────────────────────────────
#define TOUCH_RST       -1      // TBD
#define TOUCH_INT       -1      // TBD

// ── Audio (ES8311 codec via I2S) ─────────────────────────────────────────────
// TBD: Confirm I2S pins
#define I2S_MCLK        -1      // TBD
#define I2S_BCLK        -1      // TBD
#define I2S_LRCK        -1      // TBD
#define I2S_DOUT        -1      // TBD — speaker
#define I2S_DIN         -1      // TBD — mic

// ── Button ───────────────────────────────────────────────────────────────────
#define BUTTON_PIN      0       // TBD — BOOT button or external

// ── AXP2101 Interrupt ────────────────────────────────────────────────────────
#define PMU_IRQ_PIN     -1      // TBD

#endif // PLATFORM_HINT_WAVESHARE
