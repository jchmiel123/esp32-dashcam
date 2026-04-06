#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Screen — Base class for display screens (Platform B only)
// Pattern adapted from watch project ScreenManager
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "hal/DisplayHAL.h"

class Screen {
public:
    Screen(DisplayHAL* display, const char* title)
        : _display(display), _title(title) {}

    virtual ~Screen() = default;

    virtual void setup() = 0;
    virtual void update() = 0;
    virtual void draw() = 0;

    virtual void onEnter() {}   // called when screen becomes active
    virtual void onExit() {}    // called when screen is about to leave

    // Touch/gesture handling — return true if consumed
    virtual bool handleTouch(int x, int y) { return false; }
    virtual bool handleSwipe(int dx, int dy) { return false; }

    const char* getTitle() const { return _title; }

    void setNeedsRedraw() { _needsRedraw = true; }
    void forceFullRedraw() { _needsFullRedraw = true; }

protected:
    DisplayHAL* _display;
    const char* _title;
    bool _needsRedraw = true;
    bool _needsFullRedraw = true;
};
