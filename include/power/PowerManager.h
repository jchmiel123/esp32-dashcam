#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// PowerManager — System power state management
// States: RECORDING (active) → STANDBY (low power) → SHUTDOWN (cleanup)
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "config.h"
#include "hal/PowerHAL.h"
#include "core/EventBus.h"
#include "core/Logger.h"

enum class DashcamPowerState : uint8_t {
    RECORDING,  // Active — camera running, recording to SD
    STANDBY,    // Low power — screen off, recording paused, WiFi off
    SHUTDOWN    // Finalizing files before power off
};

class PowerManager {
public:
    void begin(PowerHAL* powerHAL) {
        _power = powerHAL;
        _state = DashcamPowerState::RECORDING;

        // Subscribe to button long press for standby
        EventBus::instance().subscribe(EventType::BUTTON_LONG_PRESS, [this](const Event&) {
            if (_state == DashcamPowerState::RECORDING) {
                enterStandby();
            } else if (_state == DashcamPowerState::STANDBY) {
                exitStandby();
            }
        });

        Logger::info("Power", "Power manager initialized");
    }

    void update() {
        if (!_power) return;

        // Monitor battery level
        int pct = _power->getBatteryPercent();
        if (pct >= 0) {
            if (pct <= DCAM_BATTERY_CRITICAL_PCT && _state != DashcamPowerState::SHUTDOWN) {
                Logger::warn("Power", "Battery critical: %d%%", pct);
                EventBus::instance().publish(EventType::POWER_CRITICAL, (int32_t)pct);
            } else if (pct <= DCAM_BATTERY_LOW_PCT) {
                EventBus::instance().publish(EventType::POWER_LOW, (int32_t)pct);
            }
        }
    }

    void enterStandby() {
        Logger::info("Power", "Entering standby");
        _state = DashcamPowerState::STANDBY;
        // The main loop checks state and pauses recording/disables screen
    }

    void exitStandby() {
        Logger::info("Power", "Exiting standby");
        _state = DashcamPowerState::RECORDING;
    }

    void requestShutdown() {
        Logger::info("Power", "Shutdown requested");
        _state = DashcamPowerState::SHUTDOWN;
        EventBus::instance().publish(EventType::SHUTDOWN_REQUESTED);
    }

    DashcamPowerState getState() const { return _state; }
    bool isRecording() const { return _state == DashcamPowerState::RECORDING; }

private:
    PowerHAL* _power = nullptr;
    DashcamPowerState _state = DashcamPowerState::RECORDING;
};
