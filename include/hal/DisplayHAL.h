#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Display Abstraction (Platform B only — null on XIAO)
// Wraps Arduino_GFX for Waveshare 3.5" LCD (AXS15231B QSPI driver)
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>

class DisplayHAL {
public:
    virtual ~DisplayHAL() = default;

    virtual bool begin() = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual void fillScreen(uint16_t color) = 0;
    virtual void drawText(int x, int y, const char* text, uint16_t color, uint8_t size = 1) = 0;
    virtual void drawRect(int x, int y, int w, int h, uint16_t color) = 0;
    virtual void fillRect(int x, int y, int w, int h, uint16_t color) = 0;
    virtual void drawJpeg(const uint8_t* data, size_t len, int x, int y) = 0;
    virtual void setBrightness(uint8_t level) = 0;  // 0-255
    virtual void displayOn() = 0;
    virtual void displayOff() = 0;
};

#ifdef PLATFORM_HINT_WAVESHARE
#include <Arduino_GFX_Library.h>

class DisplayWaveshare : public DisplayHAL {
public:
    bool begin() override {
        // TODO: Initialize QSPI bus and AXS15231B display driver
        // Pin mapping TBD — fill when board arrives and schematic is verified
        // The display uses QSPI (not SPI), which needs Arduino_GFX QSPI bus setup
        Serial.println("[Display] Waveshare 3.5\" LCD init — TBD");
        return false;  // TODO: implement
    }

    int width() const override { return DCAM_DISPLAY_WIDTH; }
    int height() const override { return DCAM_DISPLAY_HEIGHT; }

    void fillScreen(uint16_t color) override {
        // TODO: _gfx->fillScreen(color);
    }

    void drawText(int x, int y, const char* text, uint16_t color, uint8_t size) override {
        // TODO: _gfx->setCursor(x, y); _gfx->setTextColor(color); _gfx->setTextSize(size); _gfx->print(text);
    }

    void drawRect(int x, int y, int w, int h, uint16_t color) override {
        // TODO: _gfx->drawRect(x, y, w, h, color);
    }

    void fillRect(int x, int y, int w, int h, uint16_t color) override {
        // TODO: _gfx->fillRect(x, y, w, h, color);
    }

    void drawJpeg(const uint8_t* data, size_t len, int x, int y) override {
        // TODO: decode JPEG and blit to framebuffer
    }

    void setBrightness(uint8_t level) override {
        // TODO: may be controlled via AXP2101 DLDO
    }

    void displayOn() override {
        // TODO
    }

    void displayOff() override {
        // TODO
    }

private:
    // Arduino_GFX* _gfx = nullptr;  // TODO: initialize in begin()
};

#endif // PLATFORM_HINT_WAVESHARE
