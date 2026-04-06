#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// ESP32-S3 Dashcam — Global Configuration
// ─────────────────────────────────────────────────────────────────────────────

// ── Recording ────────────────────────────────────────────────────────────────
#define DCAM_SEGMENT_DURATION_S     60      // seconds per recording segment
#define DCAM_TARGET_FPS_XIAO        12      // OV2640 target fps
#define DCAM_TARGET_FPS_WAVESHARE   15      // OV5640 target fps
#define DCAM_JPEG_QUALITY           12      // 0-63, lower = better quality

// Resolution defaults (can be changed via settings)
#define DCAM_RES_XIAO_W             800
#define DCAM_RES_XIAO_H             600
#define DCAM_RES_WAVESHARE_W        1280
#define DCAM_RES_WAVESHARE_H        720

// ── Storage ──────────────────────────────────────────────────────────────────
#define DCAM_SD_CLEANUP_THRESHOLD   80      // percent used before auto-delete
#define DCAM_BASE_PATH              "/DCAM"
#define DCAM_SEGMENTS_DIR           "/DCAM/segments"
#define DCAM_EVENTS_DIR             "/DCAM/events"
#define DCAM_CONFIG_DIR             "/DCAM/config"
#define DCAM_LOG_DIR                "/DCAM/log"

// ── Button ───────────────────────────────────────────────────────────────────
#define DCAM_BTN_DEBOUNCE_MS        50
#define DCAM_BTN_PRESS_MAX_MS       300     // single press < this
#define DCAM_BTN_DOUBLE_GAP_MS      300     // max gap between double presses
#define DCAM_BTN_LONG_PRESS_MS      3000    // hold for standby

// ── IMU / Impact ─────────────────────────────────────────────────────────────
#define DCAM_IMPACT_THRESHOLD_G     2.0f    // acceleration in G to trigger event
#define DCAM_IMPACT_COOLDOWN_MS     5000    // ignore further impacts for 5s

// ── Wi-Fi AP ─────────────────────────────────────────────────────────────────
#define DCAM_WIFI_PASSWORD          "dashcam1234"
#define DCAM_WIFI_CHANNEL           6
#define DCAM_WIFI_MAX_CLIENTS       2
#define DCAM_WIFI_INACTIVITY_MS     300000  // 5 min auto-disable
#define DCAM_WEB_PORT               80

// ── Display (Platform B) ────────────────────────────────────────────────────
#define DCAM_DISPLAY_WIDTH          480
#define DCAM_DISPLAY_HEIGHT         320
#define DCAM_SCREEN_TIMEOUT_MS      60000   // dim after 1 min

// ── Power ────────────────────────────────────────────────────────────────────
#define DCAM_BATTERY_LOW_PCT        15
#define DCAM_BATTERY_CRITICAL_PCT   5
#define DCAM_SHUTDOWN_TIMEOUT_MS    10000   // max time for graceful shutdown

// ── I2C Addresses ────────────────────────────────────────────────────────────
#define I2C_ADDR_AXP2101            0x34
#define I2C_ADDR_ES8311             0x18
#define I2C_ADDR_PCF85063           0x51
#define I2C_ADDR_QMI8658            0x6B

// ── FreeRTOS Task Config ─────────────────────────────────────────────────────
#define TASK_CAPTURE_CORE           0
#define TASK_CAPTURE_PRIORITY       5
#define TASK_CAPTURE_STACK          4096

#define TASK_WRITER_CORE            1
#define TASK_WRITER_PRIORITY        4
#define TASK_WRITER_STACK           8192

#define TASK_IMU_CORE               0
#define TASK_IMU_PRIORITY           3
#define TASK_IMU_STACK              2048

#define FRAME_QUEUE_SIZE            4       // frames buffered between capture and writer
