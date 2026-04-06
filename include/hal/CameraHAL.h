#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Camera Hardware Abstraction
// Wraps esp32-camera driver for OV2640 (XIAO) and OV5640 (Waveshare)
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "esp_camera.h"

class CameraHAL {
public:
    virtual ~CameraHAL() = default;

    virtual bool begin() = 0;
    virtual camera_fb_t* capture() = 0;
    virtual void release(camera_fb_t* fb) = 0;

    virtual bool setResolution(framesize_t size) = 0;
    virtual bool setQuality(int quality) = 0;   // 0-63
    virtual const char* getSensorName() const = 0;
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
};

// ── OV2640 Implementation (XIAO) ────────────────────────────────────────────
class CameraOV2640 : public CameraHAL {
public:
    bool begin() override {
        camera_config_t config = {};
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer   = LEDC_TIMER_0;
        config.pin_d0       = CAM_PIN_D0;
        config.pin_d1       = CAM_PIN_D1;
        config.pin_d2       = CAM_PIN_D2;
        config.pin_d3       = CAM_PIN_D3;
        config.pin_d4       = CAM_PIN_D4;
        config.pin_d5       = CAM_PIN_D5;
        config.pin_d6       = CAM_PIN_D6;
        config.pin_d7       = CAM_PIN_D7;
        config.pin_xclk     = CAM_PIN_XCLK;
        config.pin_pclk     = CAM_PIN_PCLK;
        config.pin_vsync    = CAM_PIN_VSYNC;
        config.pin_href     = CAM_PIN_HREF;
        config.pin_sccb_sda = CAM_PIN_SIOD;
        config.pin_sccb_scl = CAM_PIN_SIOC;
        config.pin_pwdn     = CAM_PIN_PWDN;
        config.pin_reset    = CAM_PIN_RESET;
        config.xclk_freq_hz = 20000000;
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size   = FRAMESIZE_SVGA;   // 800x600
        config.jpeg_quality = DCAM_JPEG_QUALITY;
        config.fb_count     = 2;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
        config.grab_mode    = CAMERA_GRAB_LATEST;

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("[Camera] OV2640 init failed: 0x%x\n", err);
            return false;
        }
        _sensor = esp_camera_sensor_get();
        Serial.println("[Camera] OV2640 initialized");
        return true;
    }

    camera_fb_t* capture() override { return esp_camera_fb_get(); }
    void release(camera_fb_t* fb) override { esp_camera_fb_return(fb); }

    bool setResolution(framesize_t size) override {
        if (_sensor) { _sensor->set_framesize(_sensor, size); return true; }
        return false;
    }

    bool setQuality(int quality) override {
        if (_sensor) { _sensor->set_quality(_sensor, quality); return true; }
        return false;
    }

    const char* getSensorName() const override { return "OV2640"; }
    int getWidth() const override { return _width; }
    int getHeight() const override { return _height; }

private:
    sensor_t* _sensor = nullptr;
    int _width = DCAM_RES_XIAO_W;
    int _height = DCAM_RES_XIAO_H;
};

// ── OV5640 Implementation (Waveshare) ────────────────────────────────────────
class CameraOV5640 : public CameraHAL {
public:
    bool begin() override {
        camera_config_t config = {};
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer   = LEDC_TIMER_0;
        config.pin_d0       = CAM_PIN_D0;
        config.pin_d1       = CAM_PIN_D1;
        config.pin_d2       = CAM_PIN_D2;
        config.pin_d3       = CAM_PIN_D3;
        config.pin_d4       = CAM_PIN_D4;
        config.pin_d5       = CAM_PIN_D5;
        config.pin_d6       = CAM_PIN_D6;
        config.pin_d7       = CAM_PIN_D7;
        config.pin_xclk     = CAM_PIN_XCLK;
        config.pin_pclk     = CAM_PIN_PCLK;
        config.pin_vsync    = CAM_PIN_VSYNC;
        config.pin_href     = CAM_PIN_HREF;
        config.pin_sccb_sda = CAM_PIN_SIOD;
        config.pin_sccb_scl = CAM_PIN_SIOC;
        config.pin_pwdn     = CAM_PIN_PWDN;
        config.pin_reset    = CAM_PIN_RESET;
        config.xclk_freq_hz = 20000000;
        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size   = FRAMESIZE_HD;      // 1280x720
        config.jpeg_quality = DCAM_JPEG_QUALITY;
        config.fb_count     = 2;
        config.fb_location  = CAMERA_FB_IN_PSRAM;
        config.grab_mode    = CAMERA_GRAB_LATEST;

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("[Camera] OV5640 init failed: 0x%x\n", err);
            return false;
        }
        _sensor = esp_camera_sensor_get();
        Serial.println("[Camera] OV5640 initialized");
        return true;
    }

    camera_fb_t* capture() override { return esp_camera_fb_get(); }
    void release(camera_fb_t* fb) override { esp_camera_fb_return(fb); }

    bool setResolution(framesize_t size) override {
        if (_sensor) { _sensor->set_framesize(_sensor, size); return true; }
        return false;
    }

    bool setQuality(int quality) override {
        if (_sensor) { _sensor->set_quality(_sensor, quality); return true; }
        return false;
    }

    const char* getSensorName() const override { return "OV5640"; }
    int getWidth() const override { return _width; }
    int getHeight() const override { return _height; }

private:
    sensor_t* _sensor = nullptr;
    int _width = DCAM_RES_WAVESHARE_W;
    int _height = DCAM_RES_WAVESHARE_H;
};
