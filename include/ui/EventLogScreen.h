#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// EventLogScreen — List of protected clips with timestamps
// Accessible via swipe-left from viewfinder
// ─────────────────────────────────────────────────────────────────────────────

#include "ui/Screen.h"
#include "storage/FileManager.h"

class EventLogScreen : public Screen {
public:
    EventLogScreen(DisplayHAL* display, FileManager* fileManager)
        : Screen(display, "Events"), _fileManager(fileManager) {}

    void setup() override {
        // Refresh clip list on screen entry
    }

    void update() override {}

    void draw() override {
        if (!_display) return;

        if (_needsFullRedraw) {
            _display->fillScreen(0x0000);
            _display->drawText(10, 10, "Protected Clips", 0xFFFF, 2);

            // TODO: List event clips with timestamps
            // - Show EVT_ files from events directory
            // - Display filename (timestamp) and file size
            // - Highlight most recent event
            _display->drawText(10, 50, "No events recorded yet", 0x8410, 1);

            _needsFullRedraw = false;
        }
    }

private:
    FileManager* _fileManager;
};
