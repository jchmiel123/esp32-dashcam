#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// FileManager — File naming, rotation, and automatic cleanup
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <SD.h>
#include <vector>
#include "config.h"
#include "hal/StorageHAL.h"
#include "hal/RTCHAL.h"
#include "core/Logger.h"

class FileManager {
public:
    void begin(StorageHAL* storage, RTCHAL* rtc) {
        _storage = storage;
        _rtc = rtc;
    }

    // Generate next segment file path: /DCAM/segments/YYYYMMDD_HHMMSS.avi
    String nextSegmentPath() {
        String ts = _rtc->getTimestamp();
        String dir = String(DCAM_SEGMENTS_DIR);

        // Ensure directory exists
        if (!SD.exists(dir.c_str())) {
            SD.mkdir(dir.c_str());
        }

        String path = dir + "/REC_" + ts + ".avi";
        return path;
    }

    // Delete oldest unprotected segments until usage drops below threshold
    int cleanupOldSegments() {
        if (!_storage || !_storage->isReady()) return 0;
        if (_storage->usedPercent() < DCAM_SD_CLEANUP_THRESHOLD) return 0;

        int deleted = 0;
        File dir = SD.open(DCAM_SEGMENTS_DIR);
        if (!dir || !dir.isDirectory()) return 0;

        // Collect non-protected files (those starting with REC_, not EVT_)
        std::vector<String> candidates;
        File entry;
        while ((entry = dir.openNextFile())) {
            String name = entry.name();
            if (name.startsWith("REC_") && name.endsWith(".avi")) {
                candidates.push_back(String(DCAM_SEGMENTS_DIR) + "/" + name);
            }
            entry.close();
        }
        dir.close();

        // Files are naturally sorted by timestamp in name — delete oldest first
        // std::sort would work but names are already chronological from the filesystem
        for (auto& path : candidates) {
            if (_storage->usedPercent() < DCAM_SD_CLEANUP_THRESHOLD) break;
            if (SD.remove(path.c_str())) {
                deleted++;
                Logger::info("FileManager", "Deleted: %s", path.c_str());
            }
        }

        if (deleted > 0) {
            Logger::info("FileManager", "Cleaned up %d old segments", deleted);
        }
        return deleted;
    }

    // List all segment files for the web UI
    struct ClipInfo {
        String path;
        String name;
        size_t size;
        bool isProtected;
    };

    std::vector<ClipInfo> listClips() {
        std::vector<ClipInfo> clips;

        // List segments
        _listDir(DCAM_SEGMENTS_DIR, clips);
        // List events
        _listDir(DCAM_EVENTS_DIR, clips);

        return clips;
    }

private:
    StorageHAL* _storage = nullptr;
    RTCHAL* _rtc = nullptr;

    void _listDir(const char* dirPath, std::vector<ClipInfo>& clips) {
        File dir = SD.open(dirPath);
        if (!dir || !dir.isDirectory()) return;

        File entry;
        while ((entry = dir.openNextFile())) {
            if (!entry.isDirectory()) {
                String name = entry.name();
                if (name.endsWith(".avi")) {
                    ClipInfo info;
                    info.path = String(dirPath) + "/" + name;
                    info.name = name;
                    info.size = entry.size();
                    info.isProtected = name.startsWith("EVT_");
                    clips.push_back(info);
                }
            }
            entry.close();
        }
        dir.close();
    }
};
