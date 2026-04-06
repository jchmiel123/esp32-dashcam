#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Audio Abstraction
// Waveshare: ES8311 codec via I2S
// XIAO: Onboard PDM microphone
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>

class AudioHAL {
public:
    virtual ~AudioHAL() = default;

    virtual bool begin() = 0;
    virtual bool startRecording() = 0;
    virtual void stopRecording() = 0;
    virtual bool isRecording() const = 0;

    // Read audio samples into buffer, returns bytes read
    virtual size_t readSamples(int16_t* buffer, size_t maxSamples) = 0;
};

// ── ES8311 Implementation (Waveshare) ────────────────────────────────────────
class AudioES8311 : public AudioHAL {
public:
    bool begin() override {
        // TODO: Initialize ES8311 codec + I2S bus
        // Pattern from watch Audio.h:
        //   Configure I2S with I2S_MCLK, I2S_BCLK, I2S_LRCK, I2S_DIN, I2S_DOUT
        //   ES8311 is controlled via I2C at 0x18
        Serial.println("[Audio] ES8311 init — TODO");
        return false;
    }

    bool startRecording() override { _recording = true; return true; }
    void stopRecording() override { _recording = false; }
    bool isRecording() const override { return _recording; }

    size_t readSamples(int16_t* buffer, size_t maxSamples) override {
        // TODO: i2s_read()
        return 0;
    }

private:
    bool _recording = false;
};

// ── PDM Microphone Implementation (XIAO) ────────────────────────────────────
class AudioPDM : public AudioHAL {
public:
    bool begin() override {
        // TODO: Initialize I2S in PDM mode on MIC_PDM_CLK / MIC_PDM_DATA
        Serial.println("[Audio] PDM mic init — TODO");
        return false;
    }

    bool startRecording() override { _recording = true; return true; }
    void stopRecording() override { _recording = false; }
    bool isRecording() const override { return _recording; }

    size_t readSamples(int16_t* buffer, size_t maxSamples) override {
        // TODO: i2s_read() in PDM mode
        return 0;
    }

private:
    bool _recording = false;
};
