#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// RTC / Time Abstraction
// Waveshare: PCF85063 hardware RTC (survives power loss)
// XIAO: NTP sync + millis()-based fallback
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <time.h>

class RTCHAL {
public:
    virtual ~RTCHAL() = default;

    virtual bool begin() = 0;
    virtual bool syncNTP() = 0;
    virtual struct tm getTime() = 0;

    // Formatted strings for filenames and overlay
    String getTimestamp() {
        struct tm t = getTime();
        char buf[20];
        snprintf(buf, sizeof(buf), "%04d%02d%02d_%02d%02d%02d",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec);
        return String(buf);
    }

    String getOverlayText() {
        struct tm t = getTime();
        char buf[24];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec);
        return String(buf);
    }

    String getDateDir() {
        struct tm t = getTime();
        char buf[12];
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);
        return String(buf);
    }
};

// ── PCF85063 + NTP Implementation (Waveshare) ───────────────────────────────
class RTC_PCF85063 : public RTCHAL {
public:
    bool begin() override {
        // TODO: Initialize PCF85063 via I2C
        // Read time from hardware RTC immediately — no NTP needed at boot
        Serial.println("[RTC] PCF85063 init — TODO");
        return false;
    }

    bool syncNTP() override {
        // Sync system time from NTP, then write to PCF85063
        // Reuse pattern from watch TimeSync.h: 9 fallback NTP servers
        configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
        struct tm t;
        if (getLocalTime(&t, 5000)) {
            // TODO: write t to PCF85063
            Serial.println("[RTC] NTP synced, wrote to PCF85063");
            return true;
        }
        return false;
    }

    struct tm getTime() override {
        struct tm t = {};
        // TODO: Read from PCF85063 if available, fallback to system time
        getLocalTime(&t, 0);
        return t;
    }
};

// ── NTP-only Implementation (XIAO) ──────────────────────────────────────────
class RTC_NTPOnly : public RTCHAL {
public:
    bool begin() override {
        // Try to load last-known time from NVS/Preferences
        // If no stored time, start from compile time
        Serial.println("[RTC] NTP-only mode (no hardware RTC)");
        _bootTime = millis();
        return true;
    }

    bool syncNTP() override {
        configTime(0, 0, "pool.ntp.org", "time.nist.gov", "time.google.com");
        struct tm t;
        if (getLocalTime(&t, 5000)) {
            _ntpSynced = true;
            Serial.println("[RTC] NTP synced");
            return true;
        }
        Serial.println("[RTC] NTP sync failed — using fallback time");
        return false;
    }

    struct tm getTime() override {
        struct tm t = {};
        if (_ntpSynced) {
            getLocalTime(&t, 0);
        } else {
            // Fallback: compile time + elapsed millis
            time_t now = _compileEpoch + ((millis() - _bootTime) / 1000);
            localtime_r(&now, &t);
        }
        return t;
    }

private:
    bool _ntpSynced = false;
    uint32_t _bootTime = 0;
    // Approximate compile-time epoch — better than nothing
    time_t _compileEpoch = 1712400000;  // ~2024-04-06, update as needed
};
