#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// SD Card Storage Abstraction
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <SD.h>
#include <SPI.h>

class StorageHAL {
public:
    bool begin() {
        SPI.begin(SD_PIN_SCK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);
        if (!SD.begin(SD_PIN_CS)) {
            Serial.println("[Storage] SD card mount failed");
            return false;
        }

        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Serial.println("[Storage] No SD card detected");
            return false;
        }

        Serial.printf("[Storage] SD card: %s, %llu MB\n",
            cardType == CARD_MMC ? "MMC" :
            cardType == CARD_SD  ? "SD" :
            cardType == CARD_SDHC ? "SDHC" : "Unknown",
            SD.totalBytes() / (1024 * 1024));

        // Create directory structure
        ensureDir(DCAM_BASE_PATH);
        ensureDir(DCAM_SEGMENTS_DIR);
        ensureDir(DCAM_EVENTS_DIR);
        ensureDir(DCAM_CONFIG_DIR);
        ensureDir(DCAM_LOG_DIR);

        _ready = true;
        return true;
    }

    bool isReady() const { return _ready; }
    uint64_t totalBytes() const { return SD.totalBytes(); }
    uint64_t usedBytes() const { return SD.usedBytes(); }
    uint64_t freeBytes() const { return totalBytes() - usedBytes(); }
    uint8_t usedPercent() const {
        uint64_t total = totalBytes();
        return total > 0 ? (uint8_t)((usedBytes() * 100) / total) : 0;
    }

    File open(const char* path, const char* mode = "r") {
        return SD.open(path, mode);
    }

    bool remove(const char* path) { return SD.remove(path); }
    bool exists(const char* path) { return SD.exists(path); }
    bool mkdir(const char* path) { return SD.mkdir(path); }
    bool rename(const char* from, const char* to) { return SD.rename(from, to); }

private:
    bool _ready = false;

    void ensureDir(const char* path) {
        if (!SD.exists(path)) {
            SD.mkdir(path);
        }
    }
};
