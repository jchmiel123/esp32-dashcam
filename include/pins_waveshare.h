#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Pin Definitions — Waveshare ESP32-S3-Touch-LCD-3.5B-C
// VERIFIED against ON7KGK reference and Waveshare wiki (2025)
// Ref: https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-3.5B
// ─────────────────────────────────────────────────────────────────────────────

#ifdef PLATFORM_HINT_WAVESHARE

// ── Camera (OV5640 on FPC connector) ─────────────────────────────────────────
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    38
#define CAM_PIN_SIOD    8       // SCCB SDA (shared I2C bus)
#define CAM_PIN_SIOC    7       // SCCB SCL (shared I2C bus)
#define CAM_PIN_D7      21      // Y9
#define CAM_PIN_D6      39      // Y8
#define CAM_PIN_D5      40      // Y7
#define CAM_PIN_D4      42      // Y6
#define CAM_PIN_D3      46      // Y5
#define CAM_PIN_D2      48      // Y4
#define CAM_PIN_D1      47      // Y3
#define CAM_PIN_D0      45      // Y2
#define CAM_PIN_VSYNC   17
#define CAM_PIN_HREF    18
#define CAM_PIN_PCLK    41

// ── SD Card (SD_MMC 1-bit mode — NOT SPI) ────────────────────────────────────
#define SD_MMC_CLK      11      // Clock
#define SD_MMC_CMD      10      // Command
#define SD_MMC_D0       9       // Data 0
#define SD_MODE_1BIT    true    // 1-bit mode only (D1-D3 not connected)
#define SD_FREQ_KHZ     20000   // 20 MHz

// ── I2C (main bus — shared: touch, PMIC, IMU, RTC, audio, camera SCCB) ──────
#define I2C_SDA         8       // GPIO8
#define I2C_SCL         7       // GPIO7

// ── I2C Device Addresses ─────────────────────────────────────────────────────
#define TCA9554_ADDR    0x20    // I/O expander (LCD reset, touch reset)
#define TOUCH_ADDR      0x3B    // AXS15231B touch controller
#define ES8311_ADDR     0x18    // Audio codec
#define QMI8658_ADDR    0x6B    // IMU 6-axis
#define PCF85063_ADDR   0x51    // RTC
#define AXP2101_ADDR    0x34    // Power management

// ── Display (QSPI — AXS15231B driver) ────────────────────────────────────────
#define DISP_QSPI_CS    12
#define DISP_QSPI_SCK   5
#define DISP_QSPI_D0    1
#define DISP_QSPI_D1    2
#define DISP_QSPI_D2    3
#define DISP_QSPI_D3    4
#define DISP_BL         6       // Backlight PWM

// Display parameters
#define SCREEN_WIDTH    320
#define SCREEN_HEIGHT   480

// ── Touch (capacitive, I2C via AXS15231B) ────────────────────────────────────
// Touch INT is on TCA9554 P2 (not a direct GPIO)
#define TOUCH_INT_TCA_PIN  2    // TCA9554 pin for touch interrupt

// ── Audio (ES8311 codec via I2S) ─────────────────────────────────────────────
#define I2S_MCLK        44      // Master Clock
#define I2S_BCLK        13      // Bit Clock
#define I2S_LRCK        15      // Left/Right Clock (WS)
#define I2S_DOUT        16      // Data Out (speaker)
#define I2S_DIN         14      // Data In (mic)
#define PA_CTRL         11      // Power Amplifier control

// ── Button ───────────────────────────────────────────────────────────────────
#define BUTTON_PIN      0       // BOOT button

// ── TCA9554 I/O Expander Pin Assignments ─────────────────────────────────────
// The TCA9554 at 0x20 controls several board functions:
// P0 = LCD reset
// P1 = Touch reset
// P2 = Touch interrupt (input)
// (Other pins may vary by board revision)

// ── USB ──────────────────────────────────────────────────────────────────────
#define USB_DN          19
#define USB_DP          20

// ── UART (exposed on connector) ──────────────────────────────────────────────
#define UART_TX         44
#define UART_RX         43

#endif // PLATFORM_HINT_WAVESHARE
