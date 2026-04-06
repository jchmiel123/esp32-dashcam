#pragma once
// ─────────────────────────────────────────────────────────────────────────────
// WebServer — HTTP server for clip browsing, live preview, and settings
// Serves on 192.168.4.1 when WiFi AP is active
// ─────────────────────────────────────────────────────────────────────────────

#include <Arduino.h>
#include <WebServer.h>
#include <SD.h>
#include "config.h"
#include "hal/CameraHAL.h"
#include "storage/FileManager.h"
#include "core/Logger.h"

class DashcamWebServer {
public:
    void begin(CameraHAL* camera, FileManager* fileManager) {
        _camera = camera;
        _fileManager = fileManager;

        // Root — dashboard
        _server.on("/", HTTP_GET, [this]() { _handleRoot(); });

        // Clip listing
        _server.on("/clips", HTTP_GET, [this]() { _handleClipList(); });

        // Status
        _server.on("/status", HTTP_GET, [this]() { _handleStatus(); });

        // Live MJPEG stream
        _server.on("/live", HTTP_GET, [this]() { _handleLiveStream(); });

        // File download — handled by onNotFound for /clips/filename
        _server.onNotFound([this]() { _handleNotFound(); });

        _server.begin(DCAM_WEB_PORT);
        Logger::info("Web", "HTTP server started on port %d", DCAM_WEB_PORT);
    }

    void update() {
        _server.handleClient();
    }

    void stop() {
        _server.stop();
    }

private:
    WebServer _server{DCAM_WEB_PORT};
    CameraHAL* _camera = nullptr;
    FileManager* _fileManager = nullptr;

    void _handleRoot() {
        String html = R"(<!DOCTYPE html>
<html><head>
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Dashcam</title>
<style>
body{font-family:system-ui;background:#1a1a2e;color:#eee;margin:0;padding:16px}
h1{color:#e94560;margin:0 0 16px}
a{color:#0f3460;background:#e94560;padding:10px 20px;border-radius:8px;
  text-decoration:none;display:inline-block;margin:4px;color:#fff}
a:hover{background:#c73e54}
.status{background:#16213e;padding:12px;border-radius:8px;margin-bottom:16px}
</style></head><body>
<h1>ESP32-S3 Dashcam</h1>
<div class="status" id="st">Loading...</div>
<a href="/live">Live Preview</a>
<a href="/clips">Browse Clips</a>
<script>
fetch('/status').then(r=>r.json()).then(d=>{
  document.getElementById('st').innerHTML=
    'Recording: '+(d.recording?'YES':'NO')+
    ' | SD: '+d.sdUsedPct+'% | Clips: '+d.clipCount;
});
</script>
</body></html>)";
        _server.send(200, "text/html", html);
    }

    void _handleClipList() {
        auto clips = _fileManager->listClips();
        String json = "[";
        for (size_t i = 0; i < clips.size(); i++) {
            if (i > 0) json += ",";
            json += "{\"name\":\"" + clips[i].name + "\",";
            json += "\"path\":\"" + clips[i].path + "\",";
            json += "\"size\":" + String((uint32_t)clips[i].size) + ",";
            json += "\"protected\":" + String(clips[i].isProtected ? "true" : "false") + "}";
        }
        json += "]";
        _server.send(200, "application/json", json);
    }

    void _handleStatus() {
        String json = "{";
        json += "\"recording\":true,";  // TODO: get from Recorder
        json += "\"sdUsedPct\":" + String(0) + ",";  // TODO: get from StorageHAL
        json += "\"clipCount\":" + String(0);  // TODO: count from FileManager
        json += "}";
        _server.send(200, "application/json", json);
    }

    void _handleLiveStream() {
        // MJPEG over HTTP — sends continuous JPEG frames
        WiFiClient client = _server.client();
        String response = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: multipart/x-mixed-replace;boundary=frame\r\n\r\n";
        client.print(response);

        while (client.connected()) {
            camera_fb_t* fb = _camera->capture();
            if (fb) {
                client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
                client.write(fb->buf, fb->len);
                client.print("\r\n");
                _camera->release(fb);
            }
            delay(100);  // ~10 fps for live preview
        }
    }

    void _handleNotFound() {
        String uri = _server.uri();
        // Serve clip files: /download/filename.avi
        if (uri.startsWith("/download/")) {
            String filename = uri.substring(10);
            // Try segments first, then events
            String path = String(DCAM_SEGMENTS_DIR) + "/" + filename;
            if (!SD.exists(path.c_str())) {
                path = String(DCAM_EVENTS_DIR) + "/" + filename;
            }
            if (SD.exists(path.c_str())) {
                File f = SD.open(path.c_str());
                _server.streamFile(f, "video/x-msvideo");
                f.close();
                return;
            }
        }
        _server.send(404, "text/plain", "Not Found");
    }
};
