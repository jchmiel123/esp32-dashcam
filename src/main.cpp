// ─────────────────────────────────────────────────────────────────────────────
// ESP32-S3 Dashcam — Main Entry Point
//
// Boot sequence:
// 1. Serial init
// 2. HAL detection + subsystem init (I2C probe determines platform)
// 3. EventBus wiring (connect events to handlers)
// 4. Button handler init
// 5. Recording start (if SD ready)
// 6. Display UI setup (Platform B only)
// 7. Main loop: button, recorder, WiFi, display, power
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "hal/HAL.h"
#include "core/EventBus.h"
#include "core/Logger.h"
#include "recording/Recorder.h"
#include "network/WiFiAP.h"
#include "network/WebServer.h"
#include "input/ButtonHandler.h"
#include "power/PowerManager.h"
#include "power/ShutdownManager.h"
#include "storage/FileManager.h"

#ifdef PLATFORM_HINT_WAVESHARE
#include "ui/ScreenManager.h"
#include "ui/ViewfinderScreen.h"
#include "ui/EventLogScreen.h"
#include "ui/SettingsScreen.h"
#endif

// ── Global instances ─────────────────────────────────────────────────────────
static Recorder         recorder;
static WiFiAP           wifiAP;
static DashcamWebServer webServer;
static ButtonHandler    button;
static PowerManager     powerManager;
static ShutdownManager  shutdownManager;
static FileManager      fileManager;

#ifdef PLATFORM_HINT_WAVESHARE
static ScreenManager    screenManager;
#endif

// ── Event wiring ─────────────────────────────────────────────────────────────
void setupEventHandlers() {
    auto& bus = EventBus::instance();

    // Button press → protect current clip
    bus.subscribe(EventType::BUTTON_PRESS, [](const Event&) {
        recorder.protectCurrentClip();
    });

    // Double press → toggle WiFi
    bus.subscribe(EventType::BUTTON_DOUBLE_PRESS, [](const Event&) {
        wifiAP.toggle();
        if (wifiAP.isActive()) {
            webServer.begin(HAL::instance().camera, &fileManager);
        } else {
            webServer.stop();
        }
    });

    // SD space low → cleanup old segments
    bus.subscribe(EventType::SD_SPACE_LOW, [](const Event&) {
        fileManager.cleanupOldSegments();
    });

    // Impact detected (Platform B) → protect clip
    bus.subscribe(EventType::IMPACT_DETECTED, [](const Event& e) {
        Logger::info("Main", "Impact detected: %.2f G — protecting clip", e.fValue);
        recorder.protectCurrentClip();
    });

    Logger::info("Main", "Event handlers wired");
}

// ── Setup ────────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(1000);  // wait for USB-CDC

    // Initialize HAL — detects platform and inits all subsystems
    HAL& hal = HAL::instance();
    if (!hal.begin()) {
        Logger::error("Main", "HAL init had errors — check warnings above");
    }

    // Wire up event handlers
    setupEventHandlers();

    // Initialize button
    button.begin(BUTTON_PIN);

    // Initialize WiFi AP (doesn't start it — just prepares SSID)
    wifiAP.begin();

    // Initialize power manager
    powerManager.begin(hal.power);

    // Initialize file manager
    fileManager.begin(hal.storage, hal.rtc);

    // Initialize shutdown manager
    shutdownManager.begin(&recorder, hal.power, hal.rtc);

    // Initialize recorder
    uint8_t fps = (hal.platform.type == PlatformType::WAVESHARE_LCD_35B_C)
        ? DCAM_TARGET_FPS_WAVESHARE
        : DCAM_TARGET_FPS_XIAO;
    recorder.setTargetFps(fps);

    if (recorder.begin(hal.camera, hal.storage, hal.rtc)) {
        // Start recording immediately — always-on dashcam
        if (hal.storage->isReady()) {
            recorder.startRecording();
        } else {
            Logger::warn("Main", "SD not ready — recording deferred");
        }
    }

    // Setup display UI (Platform B only)
    #ifdef PLATFORM_HINT_WAVESHARE
    if (hal.hasDisplay()) {
        screenManager.addScreen(
            new ViewfinderScreen(hal.display, hal.camera, hal.storage, hal.power));
        screenManager.addScreen(
            new EventLogScreen(hal.display, &fileManager));
        screenManager.addScreen(
            new SettingsScreen(hal.display, &wifiAP));
        screenManager.begin();
        Logger::info("Main", "Display UI initialized with %d screens",
            screenManager.getScreenCount());
    }
    #endif

    // Try NTP sync if WiFi becomes available
    if (hal.rtc) {
        hal.rtc->syncNTP();  // will fail gracefully if no WiFi
    }

    Logger::info("Main", "═══ Dashcam ready ═══");
    Serial.printf("[Main] Platform: %s\n", hal.platform.name);
    Serial.printf("[Main] Camera:   %s\n", hal.platform.cameraName);
    Serial.printf("[Main] PSRAM:    %lu KB free\n", ESP.getFreePsram() / 1024);
    Serial.printf("[Main] Heap:     %lu KB free\n", ESP.getFreeHeap() / 1024);
}

// ── Main Loop ────────────────────────────────────────────────────────────────
void loop() {
    HAL& hal = HAL::instance();

    // Always: handle button input
    button.update();

    // Always: power monitoring
    powerManager.update();

    // State-dependent behavior
    if (powerManager.isRecording()) {
        // Active recording
        recorder.update();

        // IMU polling (Platform B — impact detection)
        if (hal.hasIMU() && hal.imu->impactDetected()) {
            EventBus::instance().publish(EventType::IMPACT_DETECTED,
                hal.imu->readAccel().magnitude);
        }
    }

    // WiFi AP + web server
    if (wifiAP.isActive()) {
        wifiAP.update();
        webServer.update();
    }

    // Display update (Platform B only)
    #ifdef PLATFORM_HINT_WAVESHARE
    if (hal.hasDisplay()) {
        screenManager.update();
        screenManager.draw();
    }
    #endif

    // Small yield to prevent watchdog
    delay(1);
}
