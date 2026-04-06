#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// EventBus — Lightweight publish/subscribe system for decoupled communication
// Fully implemented — no hardware dependencies
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <functional>
#include <vector>

// ── Event Types ──────────────────────────────────────────────────────────────
enum class EventType : uint8_t {
    // Recording
    RECORDING_STARTED,
    RECORDING_STOPPED,
    FILE_ROTATED,
    CLIP_PROTECTED,

    // Input
    BUTTON_PRESS,
    BUTTON_DOUBLE_PRESS,
    BUTTON_LONG_PRESS,
    IMPACT_DETECTED,

    // System
    WIFI_AP_STARTED,
    WIFI_AP_STOPPED,
    SD_CARD_ERROR,
    SD_SPACE_LOW,
    POWER_LOW,
    POWER_CRITICAL,
    SHUTDOWN_REQUESTED,

    // UI
    SCREEN_CHANGED,

    EVENT_COUNT  // sentinel
};

// ── Event Payload ────────────────────────────────────────────────────────────
struct Event {
    EventType type;
    uint32_t  timestamp;    // millis()
    union {
        float    fValue;    // e.g., impact G-force
        int32_t  iValue;    // e.g., battery percent
        const char* sValue; // e.g., filename
    };

    Event() : type(EventType::EVENT_COUNT), timestamp(0), iValue(0) {}
    Event(EventType t) : type(t), timestamp(millis()), iValue(0) {}
    Event(EventType t, float f) : type(t), timestamp(millis()), fValue(f) {}
    Event(EventType t, int32_t i) : type(t), timestamp(millis()), iValue(i) {}
    Event(EventType t, const char* s) : type(t), timestamp(millis()), sValue(s) {}
};

// ── Callback type ────────────────────────────────────────────────────────────
using EventCallback = std::function<void(const Event&)>;

// ── EventBus Singleton ───────────────────────────────────────────────────────
class EventBus {
public:
    static EventBus& instance() {
        static EventBus bus;
        return bus;
    }

    // Subscribe to a specific event type
    void subscribe(EventType type, EventCallback callback) {
        if ((uint8_t)type < (uint8_t)EventType::EVENT_COUNT) {
            _listeners[(uint8_t)type].push_back(callback);
        }
    }

    // Publish an event to all subscribers
    void publish(const Event& event) {
        uint8_t idx = (uint8_t)event.type;
        if (idx < (uint8_t)EventType::EVENT_COUNT) {
            for (auto& cb : _listeners[idx]) {
                cb(event);
            }
        }
    }

    // Convenience: publish with just a type
    void publish(EventType type) {
        publish(Event(type));
    }

    // Convenience: publish with float payload
    void publish(EventType type, float value) {
        publish(Event(type, value));
    }

    // Convenience: publish with int payload
    void publish(EventType type, int32_t value) {
        publish(Event(type, value));
    }

    // Convenience: publish with string payload
    void publish(EventType type, const char* value) {
        publish(Event(type, value));
    }

    // Get event type name for logging
    static const char* typeName(EventType type) {
        switch (type) {
            case EventType::RECORDING_STARTED:  return "RECORDING_STARTED";
            case EventType::RECORDING_STOPPED:  return "RECORDING_STOPPED";
            case EventType::FILE_ROTATED:       return "FILE_ROTATED";
            case EventType::CLIP_PROTECTED:     return "CLIP_PROTECTED";
            case EventType::BUTTON_PRESS:       return "BUTTON_PRESS";
            case EventType::BUTTON_DOUBLE_PRESS:return "BUTTON_DOUBLE_PRESS";
            case EventType::BUTTON_LONG_PRESS:  return "BUTTON_LONG_PRESS";
            case EventType::IMPACT_DETECTED:    return "IMPACT_DETECTED";
            case EventType::WIFI_AP_STARTED:    return "WIFI_AP_STARTED";
            case EventType::WIFI_AP_STOPPED:    return "WIFI_AP_STOPPED";
            case EventType::SD_CARD_ERROR:      return "SD_CARD_ERROR";
            case EventType::SD_SPACE_LOW:       return "SD_SPACE_LOW";
            case EventType::POWER_LOW:          return "POWER_LOW";
            case EventType::POWER_CRITICAL:     return "POWER_CRITICAL";
            case EventType::SHUTDOWN_REQUESTED: return "SHUTDOWN_REQUESTED";
            case EventType::SCREEN_CHANGED:     return "SCREEN_CHANGED";
            default: return "UNKNOWN";
        }
    }

private:
    EventBus() = default;
    std::vector<EventCallback> _listeners[(uint8_t)EventType::EVENT_COUNT];
};
