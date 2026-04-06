#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// Logger — Tagged serial output with log levels
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>

enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO  = 1,
    WARN  = 2,
    ERROR = 3
};

class Logger {
public:
    static void setLevel(LogLevel level) { _level = level; }

    static void debug(const char* tag, const char* fmt, ...) {
        if (_level > LogLevel::DEBUG) return;
        va_list args; va_start(args, fmt);
        _log("D", tag, fmt, args);
        va_end(args);
    }

    static void info(const char* tag, const char* fmt, ...) {
        if (_level > LogLevel::INFO) return;
        va_list args; va_start(args, fmt);
        _log("I", tag, fmt, args);
        va_end(args);
    }

    static void warn(const char* tag, const char* fmt, ...) {
        if (_level > LogLevel::WARN) return;
        va_list args; va_start(args, fmt);
        _log("W", tag, fmt, args);
        va_end(args);
    }

    static void error(const char* tag, const char* fmt, ...) {
        va_list args; va_start(args, fmt);
        _log("E", tag, fmt, args);
        va_end(args);
    }

private:
    static inline LogLevel _level = LogLevel::DEBUG;

    static void _log(const char* levelStr, const char* tag, const char* fmt, va_list args) {
        char buf[256];
        vsnprintf(buf, sizeof(buf), fmt, args);
        Serial.printf("[%s][%s] %s\n", levelStr, tag, buf);
    }
};
