#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// ClipProtector — Mark clips as event-protected (immune to auto-deletion)
// Renames REC_ prefix to EVT_ and moves to events directory
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <SD.h>
#include "config.h"
#include "hal/StorageHAL.h"
#include "core/Logger.h"

class ClipProtector {
public:
    // Protect a recording by moving it to the events directory
    bool protect(const String& filePath, StorageHAL* storage) {
        if (!storage || filePath.length() == 0) return false;
        if (!SD.exists(filePath.c_str())) return false;

        // Already protected?
        if (filePath.indexOf("/events/") >= 0) {
            Logger::info("Protect", "Already protected: %s", filePath.c_str());
            return true;
        }

        // Extract filename, change prefix from REC_ to EVT_
        int lastSlash = filePath.lastIndexOf('/');
        String filename = filePath.substring(lastSlash + 1);
        if (filename.startsWith("REC_")) {
            filename = "EVT_" + filename.substring(4);
        }

        // Move to events directory
        String destPath = String(DCAM_EVENTS_DIR) + "/" + filename;

        if (SD.rename(filePath.c_str(), destPath.c_str())) {
            Logger::info("Protect", "Protected: %s -> %s", filePath.c_str(), destPath.c_str());
            return true;
        } else {
            Logger::error("Protect", "Failed to protect: %s", filePath.c_str());
            return false;
        }
    }
};
