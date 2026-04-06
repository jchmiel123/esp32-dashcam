#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Platform Detection — Runtime hardware identification
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>

enum class PlatformType : uint8_t {
    UNKNOWN,
    XIAO_S3_SENSE,          // Seeed XIAO ESP32S3 Sense
    WAVESHARE_LCD_35B_C     // Waveshare ESP32-S3-Touch-LCD-3.5B-C
};

struct PlatformCaps {
    PlatformType type       = PlatformType::UNKNOWN;
    bool hasDisplay         = false;
    bool hasTouch           = false;
    bool hasPMIC            = false;   // AXP2101
    bool hasIMU             = false;   // QMI8658
    bool hasRTC             = false;   // PCF85063
    bool hasAudioCodec      = false;   // ES8311
    uint32_t flashSizeKB    = 0;
    uint32_t psramSizeKB    = 0;
    const char* name        = "Unknown";
    const char* cameraName  = "Unknown";

    void print() const {
        Serial.printf("[Platform] %s\n", name);
        Serial.printf("  Camera:  %s\n", cameraName);
        Serial.printf("  Display: %s\n", hasDisplay ? "Yes" : "No");
        Serial.printf("  PMIC:    %s\n", hasPMIC ? "AXP2101" : "No");
        Serial.printf("  IMU:     %s\n", hasIMU ? "QMI8658" : "No");
        Serial.printf("  RTC:     %s\n", hasRTC ? "PCF85063" : "No");
        Serial.printf("  Audio:   %s\n", hasAudioCodec ? "ES8311" : "PDM mic");
        Serial.printf("  Flash:   %lu KB\n", flashSizeKB);
        Serial.printf("  PSRAM:   %lu KB\n", psramSizeKB);
    }
};

// Probe I2C bus and determine platform capabilities
inline PlatformCaps detectPlatform() {
    PlatformCaps caps;

    // Read actual memory sizes
    caps.flashSizeKB = ESP.getFlashChipSize() / 1024;
    caps.psramSizeKB = ESP.getPsramSize() / 1024;

    // Probe I2C for Waveshare-specific peripherals
    Wire.begin(I2C_SDA, I2C_SCL);
    delay(50);  // let bus settle

    // Probe AXP2101 PMIC at 0x34
    Wire.beginTransmission(I2C_ADDR_AXP2101);
    caps.hasPMIC = (Wire.endTransmission() == 0);

    // Probe QMI8658 IMU at 0x6B
    Wire.beginTransmission(I2C_ADDR_QMI8658);
    caps.hasIMU = (Wire.endTransmission() == 0);

    // Probe PCF85063 RTC at 0x51
    Wire.beginTransmission(I2C_ADDR_PCF85063);
    caps.hasRTC = (Wire.endTransmission() == 0);

    // Probe ES8311 Audio Codec at 0x18
    Wire.beginTransmission(I2C_ADDR_ES8311);
    caps.hasAudioCodec = (Wire.endTransmission() == 0);

    // Determine platform
    if (caps.hasPMIC && caps.hasIMU) {
        caps.type = PlatformType::WAVESHARE_LCD_35B_C;
        caps.name = "Waveshare ESP32-S3-Touch-LCD-3.5B-C";
        caps.cameraName = "OV5640";
        caps.hasDisplay = true;     // will verify via QSPI init
        caps.hasTouch = true;
    } else {
        caps.type = PlatformType::XIAO_S3_SENSE;
        caps.name = "Seeed XIAO ESP32S3 Sense";
        caps.cameraName = "OV2640";
        caps.hasDisplay = false;
        caps.hasTouch = false;
    }

    return caps;
}
