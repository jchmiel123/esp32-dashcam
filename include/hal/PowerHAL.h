#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Power Management Abstraction
// Waveshare: AXP2101 PMIC via XPowersLib
// XIAO: Basic ADC voltage monitoring
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>

class PowerHAL {
public:
    virtual ~PowerHAL() = default;

    virtual bool begin() = 0;
    virtual int getBatteryPercent() = 0;     // 0-100, or -1 if unknown
    virtual float getBatteryVoltage() = 0;   // volts
    virtual bool isCharging() = 0;
    virtual bool isOnExternalPower() = 0;
    virtual void powerOff() = 0;
    virtual void sleep() = 0;
};

// ── AXP2101 Implementation (Waveshare) ───────────────────────────────────────
class PowerAXP2101 : public PowerHAL {
public:
    bool begin() override {
        // TODO: Initialize AXP2101 via XPowersLib
        // Pattern from watch project:
        //   XPowersAXP2101 pmu;
        //   pmu.begin(Wire, I2C_ADDR_AXP2101, I2C_SDA, I2C_SCL);
        //   pmu.setChargingLedMode(XPOWERS_CHG_LED_OFF);
        //   pmu.disableTSPinMeasure();
        //   pmu.enableBattVoltageMeasure();
        //   pmu.enableBattDetection();
        Serial.println("[Power] AXP2101 init — TODO");
        return false;
    }

    int getBatteryPercent() override { return -1; }    // TODO
    float getBatteryVoltage() override { return 0.0f; } // TODO
    bool isCharging() override { return false; }        // TODO
    bool isOnExternalPower() override { return true; }  // TODO
    void powerOff() override { /* TODO: pmu.shutdown() */ }
    void sleep() override { /* TODO: deep sleep with wakeup */ }
};

// ── Basic ADC Implementation (XIAO) ─────────────────────────────────────────
class PowerBasic : public PowerHAL {
public:
    bool begin() override {
        Serial.println("[Power] Basic power monitor (no PMIC)");
        return true;
    }

    int getBatteryPercent() override { return -1; }     // unknown without PMIC
    float getBatteryVoltage() override { return 0.0f; } // TODO: ADC read if battery connected
    bool isCharging() override { return false; }
    bool isOnExternalPower() override { return true; }  // assume USB power
    void powerOff() override { esp_deep_sleep_start(); }
    void sleep() override { esp_deep_sleep_start(); }
};
