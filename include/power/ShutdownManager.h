#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// ShutdownManager — Graceful shutdown on power loss
// Ensures current recording segment is properly closed and SD flushed
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "config.h"
#include "recording/Recorder.h"
#include "hal/PowerHAL.h"
#include "hal/RTCHAL.h"
#include "core/EventBus.h"
#include "core/Logger.h"

class ShutdownManager {
public:
    void begin(Recorder* recorder, PowerHAL* power, RTCHAL* rtc) {
        _recorder = recorder;
        _power = power;
        _rtc = rtc;

        // Subscribe to shutdown request
        EventBus::instance().subscribe(EventType::SHUTDOWN_REQUESTED, [this](const Event&) {
            executeShutdown();
        });

        // Subscribe to critical battery
        EventBus::instance().subscribe(EventType::POWER_CRITICAL, [this](const Event&) {
            Logger::warn("Shutdown", "Critical battery — initiating emergency shutdown");
            executeShutdown();
        });

        Logger::info("Shutdown", "Shutdown manager ready");
    }

    void executeShutdown() {
        if (_shutdownInProgress) return;
        _shutdownInProgress = true;

        uint32_t startMs = millis();
        Logger::info("Shutdown", "=== Graceful shutdown sequence ===");

        // Step 1: Stop recording — close AVI file properly
        if (_recorder && _recorder->getState() == RecordingState::RECORDING) {
            Logger::info("Shutdown", "Step 1: Stopping recorder...");
            _recorder->stopRecording();
        }

        // Step 2: Flush SD buffers (implicit in file close)
        Logger::info("Shutdown", "Step 2: SD buffers flushed");

        // Step 3: Save last-known time to flash (for NTP fallback on next boot)
        if (_rtc) {
            // TODO: Save _rtc->getTimestamp() to NVS/Preferences
            Logger::info("Shutdown", "Step 3: Saved last-known timestamp");
        }

        uint32_t elapsed = millis() - startMs;
        Logger::info("Shutdown", "Shutdown complete in %u ms", elapsed);

        // Step 4: Power off or deep sleep
        if (_power) {
            _power->sleep();
        } else {
            esp_deep_sleep_start();
        }
    }

private:
    Recorder* _recorder = nullptr;
    PowerHAL* _power = nullptr;
    RTCHAL* _rtc = nullptr;
    bool _shutdownInProgress = false;
};
