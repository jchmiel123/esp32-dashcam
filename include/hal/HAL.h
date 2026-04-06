#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// HAL Singleton — Central hardware abstraction
// Detects platform at boot and constructs appropriate subsystem implementations
// ─────────────────────────────────────────────────────────────────────────────

#include "config.h"

// Include correct pin definitions based on build environment
#ifdef PLATFORM_HINT_XIAO
    #include "pins_xiao.h"
#elif defined(PLATFORM_HINT_WAVESHARE)
    #include "pins_waveshare.h"
#endif

#include "hal/Platform.h"
#include "hal/CameraHAL.h"
#include "hal/StorageHAL.h"
#include "hal/DisplayHAL.h"
#include "hal/PowerHAL.h"
#include "hal/IMUHAL.h"
#include "hal/RTCHAL.h"
#include "hal/AudioHAL.h"

class HAL {
public:
    static HAL& instance() {
        static HAL hal;
        return hal;
    }

    // Platform capabilities (read-only after begin())
    PlatformCaps platform;

    // Subsystem pointers — always check hasXxx() before using optional ones
    CameraHAL*  camera  = nullptr;  // always present
    StorageHAL* storage = nullptr;  // always present
    DisplayHAL* display = nullptr;  // Platform B only
    PowerHAL*   power   = nullptr;  // always present (different impl)
    IMUHAL*     imu     = nullptr;  // Platform B only
    RTCHAL*     rtc     = nullptr;  // always present (different impl)
    AudioHAL*   audio   = nullptr;  // always present (different impl)

    bool hasDisplay() const { return display != nullptr; }
    bool hasIMU()     const { return imu != nullptr; }
    bool hasRTC()     const { return platform.hasRTC; }
    bool hasPMIC()    const { return platform.hasPMIC; }

    bool begin() {
        Serial.println("╔══════════════════════════════════════╗");
        Serial.println("║     ESP32-S3 Dashcam — Boot         ║");
        Serial.println("╚══════════════════════════════════════╝");

        // Step 1: Detect platform via I2C probing
        Serial.println("\n[HAL] Detecting hardware platform...");
        platform = detectPlatform();
        platform.print();

        // Step 2: Construct subsystems based on detected platform
        bool ok = true;

        // Camera — always needed
        if (platform.type == PlatformType::WAVESHARE_LCD_35B_C) {
            camera = new CameraOV5640();
        } else {
            camera = new CameraOV2640();
        }

        // Storage — same for both platforms (just different pins via #define)
        storage = new StorageHAL();

        // Power
        if (platform.hasPMIC) {
            power = new PowerAXP2101();
        } else {
            power = new PowerBasic();
        }

        // IMU (Waveshare only)
        if (platform.hasIMU) {
            imu = new IMU_QMI8658();
        }

        // RTC / Time
        if (platform.hasRTC) {
            rtc = new RTC_PCF85063();
        } else {
            rtc = new RTC_NTPOnly();
        }

        // Audio
        if (platform.hasAudioCodec) {
            audio = new AudioES8311();
        } else {
            audio = new AudioPDM();
        }

        // Display (Waveshare only)
        #ifdef PLATFORM_HINT_WAVESHARE
        if (platform.hasDisplay) {
            display = new DisplayWaveshare();
        }
        #endif

        // Step 3: Initialize each subsystem
        Serial.println("\n[HAL] Initializing subsystems...");

        if (!storage->begin()) {
            Serial.println("[HAL] WARNING: SD card failed — recording disabled");
            ok = false;
        }

        if (power && !power->begin()) {
            Serial.println("[HAL] WARNING: Power init failed");
        }

        if (rtc && !rtc->begin()) {
            Serial.println("[HAL] WARNING: RTC init failed");
        }

        if (imu && !imu->begin()) {
            Serial.println("[HAL] WARNING: IMU init failed");
        }

        if (audio && !audio->begin()) {
            Serial.println("[HAL] WARNING: Audio init failed");
        }

        if (display && !display->begin()) {
            Serial.println("[HAL] WARNING: Display init failed");
        }

        // Camera last — it reconfigures some shared pins
        if (!camera->begin()) {
            Serial.println("[HAL] CRITICAL: Camera init failed");
            ok = false;
        }

        Serial.printf("\n[HAL] Init complete. Platform: %s\n", platform.name);
        return ok;
    }

private:
    HAL() = default;
    HAL(const HAL&) = delete;
    HAL& operator=(const HAL&) = delete;
};
