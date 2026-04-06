#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// TimestampOverlay — Burn timestamp text into JPEG frames
//
// Phase 1: Simple approach — text drawn directly on decoded framebuffer
// Future optimization: Manipulate JPEG MCU blocks directly to avoid
//   full decode/re-encode cycle (much faster but more complex)
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>

class TimestampOverlay {
public:
    // Apply timestamp overlay to a JPEG buffer in PSRAM
    // Returns new JPEG buffer (caller must free) and updates len
    // For Phase 1: returns original buffer unchanged (passthrough)
    static uint8_t* apply(const uint8_t* jpegIn, size_t jpegInLen,
                          const char* text, size_t* jpegOutLen) {
        // TODO: Phase 2 implementation
        // 1. Decode JPEG to RGB565 using esp_jpg_decode()
        // 2. Draw text overlay using bitmap font (white text, black outline)
        // 3. Re-encode to JPEG using esp_jpg_encode() or fmt2jpg()
        //
        // Position: bottom-left corner with padding
        // Font: 8x16 bitmap font for readability at dashcam resolutions
        // Style: white text with 1px black outline for contrast

        // Phase 1: passthrough — no overlay yet
        *jpegOutLen = jpegInLen;
        uint8_t* copy = (uint8_t*)ps_malloc(jpegInLen);
        if (copy) {
            memcpy(copy, jpegIn, jpegInLen);
        }
        return copy;
    }
};
