#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// MJPEGWriter — Write JPEG frames into AVI container files
//
// AVI format chosen because:
// - ESP32 camera outputs JPEG natively — no transcoding needed
// - AVI is a simple RIFF container — just sequential JPEGs with headers
// - Playable in VLC, browsers, and most video players
// - Low CPU overhead — critical for a real-time embedded system
//
// Structure: RIFF → AVI → hdrl (headers) → movi (frame data) → idx1 (index)
// On close(), we seek back to update frame count and file size in the headers.
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <FS.h>

class MJPEGWriter {
public:
    bool open(fs::FS& fs, const char* path, uint16_t width, uint16_t height, uint8_t fps) {
        _file = fs.open(path, FILE_WRITE);
        if (!_file) {
            Serial.printf("[MJPEG] Failed to open: %s\n", path);
            return false;
        }

        _width = width;
        _height = height;
        _fps = fps;
        _frameCount = 0;
        _moviSize = 0;

        // Write AVI RIFF header with placeholders
        _writeHeader();

        Serial.printf("[MJPEG] Recording to %s (%dx%d @ %d fps)\n", path, width, height, fps);
        return true;
    }

    bool writeFrame(const uint8_t* jpegData, size_t jpegLen) {
        if (!_file) return false;

        // Each frame in movi list: "00dc" chunk (compressed video data)
        size_t paddedLen = (jpegLen + 1) & ~1;  // AVI chunks must be 2-byte aligned

        _file.write((const uint8_t*)"00dc", 4);                 // chunk ID
        _writeU32(jpegLen);                                       // chunk size
        _file.write(jpegData, jpegLen);                          // JPEG data
        if (paddedLen > jpegLen) {
            uint8_t pad = 0;
            _file.write(&pad, 1);                                // padding byte
        }

        // Track for index
        if (_frameCount < MAX_INDEX_ENTRIES) {
            _frameOffsets[_frameCount] = _moviSize + 4;  // offset within movi
            _frameSizes[_frameCount] = jpegLen;
        }

        _moviSize += 8 + paddedLen;  // 4 (id) + 4 (size) + paddedLen (data)
        _frameCount++;
        return true;
    }

    bool close() {
        if (!_file) return false;

        // Write idx1 index chunk
        size_t idx1Pos = _file.position();
        _file.write((const uint8_t*)"idx1", 4);
        uint32_t idxSize = _frameCount * 16;
        if (_frameCount > MAX_INDEX_ENTRIES) idxSize = MAX_INDEX_ENTRIES * 16;
        _writeU32(idxSize);

        uint32_t entriesToWrite = min(_frameCount, (uint32_t)MAX_INDEX_ENTRIES);
        for (uint32_t i = 0; i < entriesToWrite; i++) {
            _file.write((const uint8_t*)"00dc", 4);     // chunk ID
            _writeU32(0x10);                              // flags: AVIIF_KEYFRAME
            _writeU32(_frameOffsets[i]);                  // offset
            _writeU32(_frameSizes[i]);                    // size
        }

        // Calculate total file size
        size_t fileSize = _file.position();

        // Seek back and update RIFF size
        _file.seek(4);
        _writeU32(fileSize - 8);

        // Update frame count in avih header (at offset 48)
        _file.seek(48);
        _writeU32(_frameCount);

        // Update movi list size (at _moviListSizePos)
        _file.seek(_moviListSizePos);
        _writeU32(_moviSize + 4);  // +4 for "movi" identifier

        _file.close();
        Serial.printf("[MJPEG] Closed: %u frames, %u bytes\n", _frameCount, (uint32_t)fileSize);
        return true;
    }

    bool isOpen() const { return (bool)_file; }
    uint32_t frameCount() const { return _frameCount; }

private:
    File _file;
    uint16_t _width = 0;
    uint16_t _height = 0;
    uint8_t  _fps = 15;
    uint32_t _frameCount = 0;
    uint32_t _moviSize = 0;
    size_t   _moviListSizePos = 0;

    // Index entries — cap at reasonable limit for 1-minute segments
    // At 15 fps × 60s = 900 frames per segment
    static constexpr uint32_t MAX_INDEX_ENTRIES = 1024;
    uint32_t _frameOffsets[MAX_INDEX_ENTRIES];
    uint32_t _frameSizes[MAX_INDEX_ENTRIES];

    void _writeU32(uint32_t val) {
        _file.write((const uint8_t*)&val, 4);  // little-endian (native on ESP32)
    }

    void _writeU16(uint16_t val) {
        _file.write((const uint8_t*)&val, 2);
    }

    void _writeHeader() {
        // ── RIFF header ──
        _file.write((const uint8_t*)"RIFF", 4);
        _writeU32(0);                    // placeholder — updated on close()
        _file.write((const uint8_t*)"AVI ", 4);

        // ── hdrl LIST ──
        _file.write((const uint8_t*)"LIST", 4);
        _writeU32(192);                  // hdrl list size (fixed for our simple case)
        _file.write((const uint8_t*)"hdrl", 4);

        // ── avih (main AVI header) ──
        _file.write((const uint8_t*)"avih", 4);
        _writeU32(56);                   // avih struct size
        _writeU32(1000000 / _fps);       // microseconds per frame
        _writeU32(0);                    // max bytes per sec (0 = unknown)
        _writeU32(0);                    // padding
        _writeU32(0x10);                 // flags: AVIF_HASINDEX
        _writeU32(0);                    // total frames — placeholder, updated on close()
        _writeU32(0);                    // initial frames
        _writeU32(1);                    // number of streams
        _writeU32(0);                    // suggested buffer size
        _writeU32(_width);               // width
        _writeU32(_height);              // height
        _writeU32(0); _writeU32(0);      // reserved[4]
        _writeU32(0); _writeU32(0);

        // ── strl LIST (stream header list) ──
        _file.write((const uint8_t*)"LIST", 4);
        _writeU32(116);                  // strl list size
        _file.write((const uint8_t*)"strl", 4);

        // ── strh (stream header) ──
        _file.write((const uint8_t*)"strh", 4);
        _writeU32(56);                   // strh struct size
        _file.write((const uint8_t*)"vids", 4);  // stream type: video
        _file.write((const uint8_t*)"MJPG", 4);  // codec: Motion JPEG
        _writeU32(0);                    // flags
        _writeU16(0);                    // priority
        _writeU16(0);                    // language
        _writeU32(0);                    // initial frames
        _writeU32(1);                    // scale
        _writeU32(_fps);                 // rate (scale/rate = fps)
        _writeU32(0);                    // start
        _writeU32(0);                    // length — placeholder
        _writeU32(0);                    // suggested buffer size
        _writeU32(0);                    // quality
        _writeU32(0);                    // sample size
        _writeU16(0); _writeU16(0);      // frame rect (left, top)
        _writeU16(_width); _writeU16(_height);  // frame rect (right, bottom)

        // ── strf (stream format — BITMAPINFOHEADER) ──
        _file.write((const uint8_t*)"strf", 4);
        _writeU32(40);                   // strf struct size
        _writeU32(40);                   // biSize
        _writeU32(_width);               // biWidth
        _writeU32(_height);              // biHeight
        _writeU16(1);                    // biPlanes
        _writeU16(24);                   // biBitCount
        _file.write((const uint8_t*)"MJPG", 4);  // biCompression
        _writeU32(_width * _height * 3); // biSizeImage
        _writeU32(0);                    // biXPelsPerMeter
        _writeU32(0);                    // biYPelsPerMeter
        _writeU32(0);                    // biClrUsed
        _writeU32(0);                    // biClrImportant

        // ── movi LIST ──
        _file.write((const uint8_t*)"LIST", 4);
        _moviListSizePos = _file.position();  // remember for update
        _writeU32(0);                    // placeholder — updated on close()
        _file.write((const uint8_t*)"movi", 4);
    }
};
