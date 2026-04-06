#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// ButtonHandler — Single-button with multi-function detection
// Press = event mark, Double = WiFi toggle, Long = standby
// Fully implemented — timing logic only, no hardware dependency beyond GPIO read
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "config.h"
#include "core/EventBus.h"
#include "core/Logger.h"

class ButtonHandler {
public:
    void begin(uint8_t pin) {
        _pin = pin;
        pinMode(_pin, INPUT_PULLUP);  // active LOW
        _lastState = HIGH;
        _state = BTN_IDLE;
        Logger::info("Button", "Initialized on GPIO %d", _pin);
    }

    // Call every loop iteration
    void update() {
        bool raw = digitalRead(_pin);
        uint32_t now = millis();

        // Debounce
        if (raw != _lastRaw) {
            _lastDebounceMs = now;
            _lastRaw = raw;
        }
        if (now - _lastDebounceMs < DCAM_BTN_DEBOUNCE_MS) return;

        bool pressed = (raw == LOW);  // active LOW

        switch (_state) {
            case BTN_IDLE:
                if (pressed && !_wasPressed) {
                    _pressStartMs = now;
                    _state = BTN_PRESSED;
                }
                break;

            case BTN_PRESSED:
                if (!pressed) {
                    // Released — was it a short press?
                    uint32_t duration = now - _pressStartMs;
                    if (duration < DCAM_BTN_PRESS_MAX_MS) {
                        _state = BTN_WAIT_DOUBLE;
                        _releaseMs = now;
                    } else {
                        _state = BTN_IDLE;
                    }
                } else if (now - _pressStartMs >= DCAM_BTN_LONG_PRESS_MS) {
                    // Long press detected
                    Logger::info("Button", "Long press -> Standby");
                    EventBus::instance().publish(EventType::BUTTON_LONG_PRESS);
                    _state = BTN_WAIT_RELEASE;
                }
                break;

            case BTN_WAIT_DOUBLE:
                if (pressed && !_wasPressed) {
                    // Second press within window — double press!
                    if (now - _releaseMs <= DCAM_BTN_DOUBLE_GAP_MS) {
                        Logger::info("Button", "Double press -> WiFi toggle");
                        EventBus::instance().publish(EventType::BUTTON_DOUBLE_PRESS);
                        _state = BTN_WAIT_RELEASE;
                    }
                } else if (!pressed && (now - _releaseMs > DCAM_BTN_DOUBLE_GAP_MS)) {
                    // Timeout waiting for second press — it was a single press
                    Logger::info("Button", "Single press -> Event mark");
                    EventBus::instance().publish(EventType::BUTTON_PRESS);
                    _state = BTN_IDLE;
                }
                break;

            case BTN_WAIT_RELEASE:
                if (!pressed) {
                    _state = BTN_IDLE;
                }
                break;
        }

        _wasPressed = pressed;
        _lastState = raw;
    }

private:
    uint8_t _pin = 0;
    bool _lastState = HIGH;
    bool _lastRaw = HIGH;
    bool _wasPressed = false;

    enum State : uint8_t {
        BTN_IDLE,
        BTN_PRESSED,
        BTN_WAIT_DOUBLE,
        BTN_WAIT_RELEASE
    } _state = BTN_IDLE;

    uint32_t _pressStartMs = 0;
    uint32_t _releaseMs = 0;
    uint32_t _lastDebounceMs = 0;
};
