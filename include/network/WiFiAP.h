#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// WiFi Access Point — Toggled by double-press, auto-disables on inactivity
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "core/EventBus.h"
#include "core/Logger.h"

class WiFiAP {
public:
    void begin() {
        // Generate SSID from MAC address last 4 hex digits
        uint8_t mac[6];
        WiFi.macAddress(mac);
        snprintf(_ssid, sizeof(_ssid), "DASHCAM-%02X%02X", mac[4], mac[5]);
        Logger::info("WiFi", "SSID ready: %s", _ssid);
    }

    bool start() {
        if (_active) return true;

        WiFi.mode(WIFI_AP);
        WiFi.softAP(_ssid, DCAM_WIFI_PASSWORD, DCAM_WIFI_CHANNEL, 0, DCAM_WIFI_MAX_CLIENTS);

        _active = true;
        _lastActivityMs = millis();

        Logger::info("WiFi", "AP started: %s @ %s",
            _ssid, WiFi.softAPIP().toString().c_str());

        EventBus::instance().publish(EventType::WIFI_AP_STARTED);
        return true;
    }

    void stop() {
        if (!_active) return;

        WiFi.softAPdisconnect(true);
        WiFi.mode(WIFI_OFF);
        _active = false;

        Logger::info("WiFi", "AP stopped");
        EventBus::instance().publish(EventType::WIFI_AP_STOPPED);
    }

    void toggle() {
        if (_active) stop(); else start();
    }

    // Call in main loop to handle auto-disable
    void update() {
        if (!_active) return;

        // Check for connected clients
        uint8_t clients = WiFi.softAPgetStationNum();
        if (clients > 0) {
            _lastActivityMs = millis();
        }

        // Auto-disable after inactivity
        if (millis() - _lastActivityMs > DCAM_WIFI_INACTIVITY_MS) {
            Logger::info("WiFi", "Auto-disabling AP (no activity for %d s)",
                DCAM_WIFI_INACTIVITY_MS / 1000);
            stop();
        }
    }

    bool isActive() const { return _active; }
    const char* getSSID() const { return _ssid; }
    uint8_t getClientCount() const { return _active ? WiFi.softAPgetStationNum() : 0; }

private:
    bool _active = false;
    char _ssid[20] = {};
    uint32_t _lastActivityMs = 0;
};
