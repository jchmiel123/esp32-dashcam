#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Recorder — Recording state machine and file rotation
// Manages the capture → write pipeline using FreeRTOS tasks
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include "config.h"
#include "hal/CameraHAL.h"
#include "hal/StorageHAL.h"
#include "hal/RTCHAL.h"
#include "recording/MJPEGWriter.h"
#include "storage/FileManager.h"
#include "storage/ClipProtector.h"
#include "core/EventBus.h"
#include "core/Logger.h"

enum class RecordingState : uint8_t {
    IDLE,
    RECORDING,
    PAUSED,
    ERROR
};

class Recorder {
public:
    bool begin(CameraHAL* camera, StorageHAL* storage, RTCHAL* rtc) {
        _camera = camera;
        _storage = storage;
        _rtc = rtc;
        _fileManager.begin(storage, rtc);
        _state = RecordingState::IDLE;
        Logger::info("Rec", "Recorder initialized");
        return true;
    }

    bool startRecording() {
        if (!_camera || !_storage || !_storage->isReady()) {
            Logger::error("Rec", "Cannot start — camera or storage not ready");
            _state = RecordingState::ERROR;
            return false;
        }

        // Open new segment file
        String path = _fileManager.nextSegmentPath();
        if (!_writer.open(SD, path.c_str(),
                _camera->getWidth(), _camera->getHeight(),
                _targetFps)) {
            Logger::error("Rec", "Failed to open segment file");
            _state = RecordingState::ERROR;
            return false;
        }

        _segmentStartMs = millis();
        _frameCount = 0;
        _state = RecordingState::RECORDING;
        _currentPath = path;
        _previousPath = "";

        EventBus::instance().publish(EventType::RECORDING_STARTED);
        Logger::info("Rec", "Recording started: %s", path.c_str());
        return true;
    }

    // Call this in the main loop or from a FreeRTOS task
    void update() {
        if (_state != RecordingState::RECORDING) return;

        // Check if it's time to rotate
        if (millis() - _segmentStartMs >= (uint32_t)DCAM_SEGMENT_DURATION_S * 1000) {
            rotateSegment();
        }

        // Capture and write a frame
        camera_fb_t* fb = _camera->capture();
        if (fb) {
            if (fb->format == PIXFORMAT_JPEG) {
                _writer.writeFrame(fb->buf, fb->len);
                _frameCount++;
            }
            _camera->release(fb);
        }

        // Check SD space
        if (_storage->usedPercent() >= DCAM_SD_CLEANUP_THRESHOLD) {
            EventBus::instance().publish(EventType::SD_SPACE_LOW);
        }
    }

    void stopRecording() {
        if (_state != RecordingState::RECORDING) return;

        _writer.close();
        _state = RecordingState::IDLE;
        EventBus::instance().publish(EventType::RECORDING_STOPPED);
        Logger::info("Rec", "Recording stopped: %u frames", _frameCount);
    }

    void protectCurrentClip() {
        if (_state == RecordingState::RECORDING) {
            // Protect current segment
            _clipProtector.protect(_currentPath, _storage);

            // Protect previous segment too (impact may have started before rotation)
            if (_previousPath.length() > 0) {
                _clipProtector.protect(_previousPath, _storage);
            }

            EventBus::instance().publish(EventType::CLIP_PROTECTED, _currentPath.c_str());
            Logger::info("Rec", "Clips protected: current + previous");
        }
    }

    RecordingState getState() const { return _state; }
    uint32_t getFrameCount() const { return _frameCount; }
    const String& getCurrentPath() const { return _currentPath; }

    void setTargetFps(uint8_t fps) { _targetFps = fps; }

private:
    CameraHAL*  _camera = nullptr;
    StorageHAL* _storage = nullptr;
    RTCHAL*     _rtc = nullptr;

    MJPEGWriter   _writer;
    FileManager   _fileManager;
    ClipProtector _clipProtector;

    RecordingState _state = RecordingState::IDLE;
    uint32_t _segmentStartMs = 0;
    uint32_t _frameCount = 0;
    uint8_t  _targetFps = DCAM_TARGET_FPS_XIAO;

    String _currentPath;
    String _previousPath;

    void rotateSegment() {
        // Close current file
        _writer.close();
        _previousPath = _currentPath;

        EventBus::instance().publish(EventType::FILE_ROTATED, _currentPath.c_str());

        // Open new segment
        String path = _fileManager.nextSegmentPath();
        _writer.open(SD, path.c_str(),
            _camera->getWidth(), _camera->getHeight(), _targetFps);

        _segmentStartMs = millis();
        _frameCount = 0;
        _currentPath = path;

        Logger::info("Rec", "Rotated to: %s", path.c_str());
    }
};
