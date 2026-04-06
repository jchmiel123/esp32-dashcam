#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// ViewfinderScreen — Live camera preview with recording overlay
// Default screen on Platform B
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/Screen.h"
#include "hal/CameraHAL.h"
#include "hal/StorageHAL.h"
#include "hal/PowerHAL.h"

class ViewfinderScreen : public Screen {
public:
    ViewfinderScreen(DisplayHAL* display, CameraHAL* camera,
                     StorageHAL* storage, PowerHAL* power)
        : Screen(display, "Viewfinder"),
          _camera(camera), _storage(storage), _power(power) {}

    void setup() override {}

    void update() override {
        // TODO: Capture frame and display on screen
        // - Camera capture at reduced resolution for preview
        // - Draw recording indicator (red dot, pulsing)
        // - Draw timestamp overlay
        // - Draw SD usage percentage
        // - Draw battery level (if PMIC available)
    }

    void draw() override {
        if (!_display) return;

        if (_needsFullRedraw) {
            _display->fillScreen(0x0000);  // black
            _needsFullRedraw = false;
        }

        // TODO: Blit camera frame to display
        // TODO: Draw overlay text (timestamp, recording indicator, battery)
    }

    bool handleTouch(int x, int y) override {
        // Tap center = mark event (same as button press)
        int cx = _display->width() / 2;
        int cy = _display->height() / 2;
        int dist = abs(x - cx) + abs(y - cy);
        if (dist < 80) {
            EventBus::instance().publish(EventType::BUTTON_PRESS);
            return true;
        }
        return false;
    }

private:
    CameraHAL*  _camera;
    StorageHAL* _storage;
    PowerHAL*   _power;
};
