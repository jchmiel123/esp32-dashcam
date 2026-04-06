#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SettingsScreen — Configuration UI (Platform B only)
// Accessible via swipe-right from viewfinder
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/Screen.h"
#include "network/WiFiAP.h"

class SettingsScreen : public Screen {
public:
    SettingsScreen(DisplayHAL* display, WiFiAP* wifiAP)
        : Screen(display, "Settings"), _wifiAP(wifiAP) {}

    void setup() override {}

    void update() override {}

    void draw() override {
        if (!_display) return;

        if (_needsFullRedraw) {
            _display->fillScreen(0x0000);
            _display->drawText(10, 10, "Settings", 0xFFFF, 2);

            // TODO: Render settings UI
            // - Resolution selector
            // - Frame rate selector
            // - IMU threshold slider
            // - WiFi toggle button
            // - Format SD card button (with confirmation)
            // - WiFi password display/change
            int y = 50;
            _display->drawText(10, y, "Resolution: 1280x720", 0xBDF7, 1); y += 20;
            _display->drawText(10, y, "Frame Rate: 15 fps", 0xBDF7, 1); y += 20;
            _display->drawText(10, y, "Impact Threshold: 2.0 G", 0xBDF7, 1); y += 20;

            if (_wifiAP && _wifiAP->isActive()) {
                _display->drawText(10, y, "WiFi: ON", 0x07E0, 1); y += 20;
                _display->drawText(10, y, _wifiAP->getSSID(), 0xBDF7, 1); y += 20;
            } else {
                _display->drawText(10, y, "WiFi: OFF", 0xF800, 1); y += 20;
            }

            _needsFullRedraw = false;
        }
    }

private:
    WiFiAP* _wifiAP;
};
