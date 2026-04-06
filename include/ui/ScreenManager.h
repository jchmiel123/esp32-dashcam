#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// ScreenManager — Navigation between screens with swipe gestures
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <vector>
#include "ui/Screen.h"
#include "core/EventBus.h"

class ScreenManager {
public:
    void addScreen(Screen* screen) {
        _screens.push_back(screen);
    }

    void begin() {
        if (!_screens.empty()) {
            _currentIndex = 0;
            _screens[0]->setup();
            _screens[0]->onEnter();
        }
    }

    void update() {
        if (_currentIndex < _screens.size()) {
            _screens[_currentIndex]->update();
        }
    }

    void draw() {
        if (_currentIndex < _screens.size()) {
            _screens[_currentIndex]->draw();
        }
    }

    bool handleTouch(int x, int y) {
        if (_currentIndex < _screens.size()) {
            return _screens[_currentIndex]->handleTouch(x, y);
        }
        return false;
    }

    // Swipe left = next screen, swipe right = previous screen
    bool handleSwipe(int dx, int dy) {
        // Let current screen handle it first
        if (_currentIndex < _screens.size()) {
            if (_screens[_currentIndex]->handleSwipe(dx, dy)) {
                return true;
            }
        }

        // Horizontal swipe navigation
        if (abs(dx) > abs(dy) && abs(dx) > 50) {
            if (dx < 0 && _currentIndex < _screens.size() - 1) {
                switchTo(_currentIndex + 1);
                return true;
            } else if (dx > 0 && _currentIndex > 0) {
                switchTo(_currentIndex - 1);
                return true;
            }
        }
        return false;
    }

    void switchTo(size_t index) {
        if (index >= _screens.size() || index == _currentIndex) return;

        _screens[_currentIndex]->onExit();
        _currentIndex = index;
        _screens[_currentIndex]->setup();
        _screens[_currentIndex]->onEnter();
        _screens[_currentIndex]->forceFullRedraw();

        EventBus::instance().publish(EventType::SCREEN_CHANGED, (int32_t)index);
    }

    size_t getCurrentIndex() const { return _currentIndex; }
    size_t getScreenCount() const { return _screens.size(); }

private:
    std::vector<Screen*> _screens;
    size_t _currentIndex = 0;
};
