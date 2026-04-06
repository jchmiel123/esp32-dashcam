#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// IMU (Inertial Measurement Unit) Abstraction
// QMI8658 on Waveshare only — used for impact/crash detection
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "config.h"

struct AccelData {
    float x, y, z;     // in G
    float magnitude;    // sqrt(x² + y² + z²)
};

class IMUHAL {
public:
    virtual ~IMUHAL() = default;

    virtual bool begin() = 0;
    virtual AccelData readAccel() = 0;
    virtual bool impactDetected() = 0;

    void setImpactThreshold(float g) { _thresholdG = g; }
    float getImpactThreshold() const { return _thresholdG; }

protected:
    float _thresholdG = DCAM_IMPACT_THRESHOLD_G;
    uint32_t _lastImpactMs = 0;
};

// ── QMI8658 Implementation (Waveshare) ───────────────────────────────────────
class IMU_QMI8658 : public IMUHAL {
public:
    bool begin() override {
        // TODO: Initialize QMI8658 via SensorLib
        // Pattern from watch project:
        //   SensorQMI8658 imu;
        //   imu.begin(Wire, I2C_ADDR_QMI8658, I2C_SDA, I2C_SCL);
        //   imu.configAccelerometer(
        //       SensorQMI8658::ACC_RANGE_4G,
        //       SensorQMI8658::ACC_ODR_250Hz);
        Serial.println("[IMU] QMI8658 init — TODO");
        return false;
    }

    AccelData readAccel() override {
        AccelData data = {0, 0, 0, 0};
        // TODO: Read from QMI8658
        // data.magnitude = sqrtf(data.x*data.x + data.y*data.y + data.z*data.z);
        return data;
    }

    bool impactDetected() override {
        uint32_t now = millis();
        if (now - _lastImpactMs < DCAM_IMPACT_COOLDOWN_MS) return false;

        AccelData accel = readAccel();
        if (accel.magnitude > _thresholdG) {
            _lastImpactMs = now;
            Serial.printf("[IMU] Impact! %.2f G (threshold: %.2f G)\n",
                accel.magnitude, _thresholdG);
            return true;
        }
        return false;
    }
};
