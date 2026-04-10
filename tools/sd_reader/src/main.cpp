// Waveshare ESP32-S3-Touch-LCD-3.5B-C — Dashcam v0.6
// Continuous MJPEG AVI recording to SD card
// Core 0 = LCD preview (JPEG decode) + HUD overlay
// Core 1 = JPEG capture → AVI segments (1 min each)
// BOOT button = snapshot 5MP JPEG to /DCIM/photos/
#include <Arduino.h>
#include <Wire.h>
#include <SD_MMC.h>
#include "esp_camera.h"
#include "img_converters.h"
#include <rom/tjpgd.h>
#include <Arduino_GFX_Library.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

// ── Pins ──
#define I2C_SDA   8
#define I2C_SCL   7
#define TCA9554_ADDR  0x20
#define TCA9554_OUTPUT_REG  0x01
#define TCA9554_CONFIG_REG  0x03
#define SD_CLK    11
#define SD_CMD    10
#define SD_D0     9
#define CAM_XCLK  38
#define CAM_SIOD  8
#define CAM_SIOC  7
#define CAM_D7    21
#define CAM_D6    39
#define CAM_D5    40
#define CAM_D4    42
#define CAM_D3    46
#define CAM_D2    48
#define CAM_D1    47
#define CAM_D0    45
#define CAM_VSYNC 17
#define CAM_HREF  18
#define CAM_PCLK  41
#define LCD_CS    12
#define LCD_CLK   5
#define LCD_D0_PIN 1
#define LCD_D1_PIN 2
#define LCD_D2_PIN 3
#define LCD_D3_PIN 4
#define LCD_BL    6
#define BTN_PIN   0

// ── Config ──
#define SEGMENT_SECONDS   60      // AVI segment length
#define JPEG_QUALITY      12      // 1-63, lower = better
#define TARGET_FPS        8       // recording FPS target
#define SD_FREE_MIN_MB    500     // auto-delete when below this
#define MAX_AVI_FRAMES    900     // index array size (>= SEGMENT_SECONDS * TARGET_FPS)

// ── TCA9554 ──
void tca_write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(TCA9554_ADDR);
    Wire.write(reg); Wire.write(val); Wire.endTransmission();
}
uint8_t tca_read_reg(uint8_t reg) {
    Wire.beginTransmission(TCA9554_ADDR);
    Wire.write(reg); Wire.endTransmission();
    Wire.requestFrom(TCA9554_ADDR, (uint8_t)1);
    return Wire.read();
}
void tca_pin_mode(uint8_t pin, uint8_t mode) {
    uint8_t c = tca_read_reg(TCA9554_CONFIG_REG);
    if (mode == OUTPUT) c &= ~(1 << pin); else c |= (1 << pin);
    tca_write_reg(TCA9554_CONFIG_REG, c);
}
void tca_write(uint8_t pin, uint8_t val) {
    uint8_t o = tca_read_reg(TCA9554_OUTPUT_REG);
    if (val) o |= (1 << pin); else o &= ~(1 << pin);
    tca_write_reg(TCA9554_OUTPUT_REG, o);
}

// ══════════════════════════════════════════════════════════════════════════
// MJPEG AVI WRITER
// Writes a valid RIFF AVI with MJPG codec. Frames are raw JPEG blobs.
// Call begin() → addFrame() in a loop → end() to finalize headers.
// ══════════════════════════════════════════════════════════════════════════
class AVIWriter {
public:
    bool begin(const char* path, uint16_t w, uint16_t h, uint8_t fps) {
        _f = SD_MMC.open(path, FILE_WRITE);
        if (!_f) return false;
        _w = w; _h = h; _fps = fps; _nFrames = 0;
        _offsets = (uint32_t*)ps_malloc(MAX_AVI_FRAMES * sizeof(uint32_t));
        _sizes   = (uint32_t*)ps_malloc(MAX_AVI_FRAMES * sizeof(uint32_t));
        if (!_offsets || !_sizes) { _f.close(); return false; }
        writeHeader();
        _moviStart = _f.position();
        return true;
    }

    bool addFrame(const uint8_t* jpeg, size_t len) {
        if (!_f || _nFrames >= MAX_AVI_FRAMES) return false;
        _offsets[_nFrames] = _f.position() - _moviStart;
        _sizes[_nFrames] = len;
        _f.write((const uint8_t*)"00dc", 4);
        uint32_t sz = len;
        _f.write((const uint8_t*)&sz, 4);
        _f.write(jpeg, len);
        if (len & 1) { uint8_t pad = 0; _f.write(&pad, 1); }
        _nFrames++;
        return true;
    }

    void end() {
        if (!_f) return;
        // Update movi LIST size
        uint32_t moviEnd = _f.position();
        uint32_t moviListSize = moviEnd - _moviStart + 4;

        // Write idx1 chunk
        _f.write((const uint8_t*)"idx1", 4);
        uint32_t idxSize = _nFrames * 16;
        _f.write((const uint8_t*)&idxSize, 4);
        for (uint32_t i = 0; i < _nFrames; i++) {
            _f.write((const uint8_t*)"00dc", 4);
            uint32_t flags = 0x10; // AVIIF_KEYFRAME
            _f.write((const uint8_t*)&flags, 4);
            _f.write((const uint8_t*)&_offsets[i], 4);
            _f.write((const uint8_t*)&_sizes[i], 4);
        }

        // Patch RIFF size
        uint32_t fileSize = _f.position() - 8;
        _f.seek(4); _f.write((const uint8_t*)&fileSize, 4);

        // Patch avih total frames
        _f.seek(_avihFramePos); _f.write((const uint8_t*)&_nFrames, 4);

        // Patch strh length (frame count)
        _f.seek(_strhLenPos); _f.write((const uint8_t*)&_nFrames, 4);

        // Patch movi LIST size
        _f.seek(_moviStart - 4); _f.write((const uint8_t*)&moviListSize, 4);

        _f.close();
        free(_offsets); _offsets = nullptr;
        free(_sizes);   _sizes = nullptr;
    }

    uint32_t frames() const { return _nFrames; }
    bool isOpen() const { return (bool)_f; }

private:
    File _f;
    uint16_t _w, _h;
    uint8_t _fps;
    uint32_t _nFrames;
    uint32_t _moviStart;
    uint32_t _avihFramePos, _strhLenPos;
    uint32_t *_offsets, *_sizes;

    void w4(const char* s) { _f.write((const uint8_t*)s, 4); }
    void w32(uint32_t v) { _f.write((const uint8_t*)&v, 4); }
    void w16(uint16_t v) { _f.write((const uint8_t*)&v, 2); }

    void writeHeader() {
        // RIFF header
        w4("RIFF"); w32(0); w4("AVI ");

        // LIST hdrl
        w4("LIST"); uint32_t hdrlSzPos = _f.position(); w32(0); w4("hdrl");

        // avih — Main AVI Header (56 bytes)
        w4("avih"); w32(56);
        w32(1000000 / _fps);         // microseconds per frame
        w32(500000);                  // max bytes per sec
        w32(0);                       // padding granularity
        w32(0x10);                    // flags: AVIF_HASINDEX
        _avihFramePos = _f.position();
        w32(0);                       // total frames (patched on close)
        w32(0);                       // initial frames
        w32(1);                       // streams
        w32(_w * _h * 3);            // suggested buffer size
        w32(_w); w32(_h);            // dimensions
        w32(0); w32(0); w32(0); w32(0); // reserved

        // LIST strl
        w4("LIST"); uint32_t strlSzPos = _f.position(); w32(0); w4("strl");

        // strh — Stream Header (56 bytes)
        w4("strh"); w32(56);
        w4("vids");                   // stream type
        w4("MJPG");                   // codec
        w32(0);                       // flags
        w16(0); w16(0);              // priority, language
        w32(0);                       // initial frames
        w32(1);                       // scale
        w32(_fps);                    // rate
        w32(0);                       // start
        _strhLenPos = _f.position();
        w32(0);                       // length (patched on close)
        w32(_w * _h * 3);            // suggested buffer size
        w32(0xFFFFFFFF);             // quality
        w32(0);                       // sample size
        w16(0); w16(0);              // frame rect left, top
        w16(_w); w16(_h);            // frame rect right, bottom

        // strf — Stream Format (BITMAPINFOHEADER, 40 bytes)
        w4("strf"); w32(40);
        w32(40);                      // biSize
        w32(_w); w32(_h);
        w16(1);                       // planes
        w16(24);                      // bpp
        w4("MJPG");                   // compression
        w32(_w * _h * 3);            // image size
        w32(0); w32(0);              // pixels per meter
        w32(0); w32(0);              // colors used/important

        // Patch hdrl and strl sizes
        uint32_t here = _f.position();
        uint32_t hdrlSz = here - hdrlSzPos - 4;
        _f.seek(hdrlSzPos); w32(hdrlSz); _f.seek(here);

        uint32_t strlSz = here - strlSzPos - 4;
        _f.seek(strlSzPos); w32(strlSz); _f.seek(here);

        // LIST movi
        w4("LIST"); w32(0); w4("movi");  // size patched on close
    }
};

// ══════════════════════════════════════════════════════════════════════════
// FAST JPEG → RGB565 DECODER (TJpgDec ROM, 1/2 scale, row-doubled)
// Decodes 640x480 JPEG → 320x240 → writes 320x480 to canvas directly.
// No intermediate buffer. ~4x faster than fmt2rgb888 + conversion.
// ══════════════════════════════════════════════════════════════════════════
// Row buffer in SRAM for batching PSRAM writes from decode callback
static uint16_t jpgRowBuf[320];

volatile uint8_t previewScale = 1;  // 1=half (2 FPS), 2=quarter (fast)

struct JpgCtx {
    const uint8_t* src;
    size_t len;
    size_t pos;
    uint16_t* dst;       // canvas framebuffer (320x480)
    uint8_t scale;       // 1=half(320x240), 2=quarter(160x120)
};

static UINT jpgReadCb(JDEC* jd, BYTE* buf, UINT len) {
    JpgCtx* ctx = (JpgCtx*)jd->device;
    UINT avail = ctx->len - ctx->pos;
    if (len > avail) len = avail;
    if (buf) memcpy(buf, ctx->src + ctx->pos, len);
    ctx->pos += len;
    return len;
}

static UINT jpgWriteCb(JDEC* jd, void* bitmap, JRECT* rect) {
    JpgCtx* ctx = (JpgCtx*)jd->device;
    uint8_t* rgb = (uint8_t*)bitmap;
    int blockW = rect->right - rect->left + 1;

    if (ctx->scale == 1) {
        // Half scale: 320x240 → row-double to 320x480
        for (int y = rect->top; y <= rect->bottom; y++) {
            uint16_t* row = &jpgRowBuf[rect->left];
            for (int i = 0; i < blockW; i++) {
                uint16_t r5 = (*rgb++) >> 3;
                uint16_t g6 = (*rgb++) >> 2;
                uint16_t b5 = (*rgb++) >> 3;
                row[i] = (r5 << 11) | (g6 << 5) | b5;
            }
            int dy = y * 2;
            memcpy(&ctx->dst[dy * 320 + rect->left], row, blockW * 2);
            if (dy + 1 < 480)
                memcpy(&ctx->dst[(dy + 1) * 320 + rect->left], row, blockW * 2);
        }
    } else {
        // Quarter scale: 160x120 → 2x horiz + 4x vert to 320x480
        for (int y = rect->top; y <= rect->bottom; y++) {
            // Convert + double horizontally: 160 → 320
            int dstX = rect->left * 2;
            for (int i = 0; i < blockW; i++) {
                uint16_t r5 = (*rgb++) >> 3;
                uint16_t g6 = (*rgb++) >> 2;
                uint16_t b5 = (*rgb++) >> 3;
                uint16_t px = (r5 << 11) | (g6 << 5) | b5;
                jpgRowBuf[dstX + i * 2]     = px;
                jpgRowBuf[dstX + i * 2 + 1] = px;
            }
            // Write 4 copies vertically
            int dy = y * 4;
            int rowBytes = blockW * 2 * 2;  // doubled width in bytes
            for (int r = 0; r < 4 && dy + r < 480; r++) {
                memcpy(&ctx->dst[(dy + r) * 320 + dstX], &jpgRowBuf[dstX], rowBytes);
            }
        }
    }
    return 1;
}

// ── Globals ──
XPowersPMU pmu;
Arduino_DataBus *bus = nullptr;
Arduino_GFX *lcd = nullptr;
Arduino_Canvas *canvas = nullptr;
bool sdReady = false;
volatile int segmentCount = 0;           // AVI segments this session
volatile bool wipeRequested = false;     // BOOT button: wipe SD confirmed
volatile bool recording = false;         // true while AVI is open
volatile bool recordingEnabled = true;   // user can pause/resume recording
volatile uint32_t recFrames = 0;         // frames in current segment
volatile float recFPS = 0;              // measured recording FPS

// HUD status message (shown for a few seconds after button action)
char hudMsg[32] = {};
unsigned long hudMsgExpiry = 0;

// Preview shared buffer (Core 1 writes, Core 0 reads)
uint8_t  *previewBuf = nullptr;         // JPEG data (PSRAM)
volatile size_t previewLen = 0;
volatile bool previewReady = false;
SemaphoreHandle_t previewMutex;

// TJpgDec work buffer — MUST be in SRAM (not PSRAM) for speed
uint8_t *jpgWorkBuf = nullptr;
#define JPG_WORK_SIZE 4096


// Camera mutex (protects esp_camera_* calls)
SemaphoreHandle_t camMutex;

// ── Camera config helper ──
void fillCamPins(camera_config_t &cfg) {
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer = LEDC_TIMER_0;
    cfg.pin_d0 = CAM_D0;  cfg.pin_d1 = CAM_D1;
    cfg.pin_d2 = CAM_D2;  cfg.pin_d3 = CAM_D3;
    cfg.pin_d4 = CAM_D4;  cfg.pin_d5 = CAM_D5;
    cfg.pin_d6 = CAM_D6;  cfg.pin_d7 = CAM_D7;
    cfg.pin_xclk = CAM_XCLK; cfg.pin_pclk = CAM_PCLK;
    cfg.pin_vsync = CAM_VSYNC; cfg.pin_href = CAM_HREF;
    cfg.pin_sccb_sda = -1; cfg.pin_sccb_scl = -1;
    cfg.sccb_i2c_port = 0;
    cfg.pin_pwdn = -1; cfg.pin_reset = -1;
    cfg.xclk_freq_hz = 20000000;
    cfg.fb_location = CAMERA_FB_IN_PSRAM;
}

// ── PMU ──
bool initPMU() {
    if (!pmu.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) return false;
    pmu.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);
    pmu.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);
    pmu.setBLDO1Voltage(1500);
    pmu.setBLDO2Voltage(2800);
    pmu.enableBLDO1();
    pmu.enableBLDO2();
    pmu.enableBattDetection();
    pmu.enableBattVoltageMeasure();
    pmu.disableTSPinMeasure();
    delay(100);
    Serial.printf("PMU OK (0x%x)\n", pmu.getChipID());
    return true;
}

// ── Display ──
bool initDisplay() {
    tca_pin_mode(1, OUTPUT);
    tca_write(1, 1); delay(10);
    tca_write(1, 0); delay(10);
    tca_write(1, 1); delay(200);

    bus = new Arduino_ESP32QSPI(LCD_CS, LCD_CLK, LCD_D0_PIN, LCD_D1_PIN, LCD_D2_PIN, LCD_D3_PIN);
    lcd = new Arduino_AXS15231B(bus, -1, 0, false, 320, 480);
    canvas = new Arduino_Canvas(320, 480, lcd, 0, 0, 0);  // rotation=0 — MUST stay 0

    if (!canvas->begin()) return false;
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);
    canvas->fillScreen(BLACK);
    canvas->flush();
    Serial.println("Display OK");
    return true;
}

// ── SD ──
bool initSD() {
    SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0);
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("SD FAILED");
        return false;
    }
    if (!SD_MMC.exists("/DCIM")) SD_MMC.mkdir("/DCIM");
    if (!SD_MMC.exists("/DCIM/video")) SD_MMC.mkdir("/DCIM/video");
    if (!SD_MMC.exists("/DCIM/photos")) SD_MMC.mkdir("/DCIM/photos");

    Serial.printf("SD OK: %.1f GB free\n",
        (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / 1073741824.0);
    return true;
}

// ── SD space management: delete oldest video when low ──
void cleanupOldVideos() {
    uint64_t freeBytes = SD_MMC.totalBytes() - SD_MMC.usedBytes();
    uint64_t minFree = (uint64_t)SD_FREE_MIN_MB * 1024 * 1024;
    if (freeBytes >= minFree) return;

    // Find and delete the oldest REC_*.avi
    File dir = SD_MMC.open("/DCIM/video");
    if (!dir) return;

    char oldest[64] = {};
    File f = dir.openNextFile();
    while (f) {
        if (!oldest[0] || strcmp(f.name(), oldest) < 0) {
            strncpy(oldest, f.name(), sizeof(oldest) - 1);
        }
        f = dir.openNextFile();
    }

    if (oldest[0]) {
        char path[96];
        snprintf(path, sizeof(path), "/DCIM/video/%s", oldest);
        SD_MMC.remove(path);
        Serial.printf("[SD] Deleted oldest: %s (free was %.0fMB)\n",
            path, freeBytes / 1048576.0);
    }
}

// ── Wipe all files in a directory ──
void wipeDir(const char* path) {
    File dir = SD_MMC.open(path);
    if (!dir) return;
    // Collect filenames first (can't delete while iterating on FAT32)
    int count = 0;
    char names[64][32];  // up to 64 files per batch
    File f = dir.openNextFile();
    while (f && count < 64) {
        strncpy(names[count], f.name(), 31);
        names[count][31] = 0;
        count++;
        f.close();
        f = dir.openNextFile();
    }
    dir.close();
    for (int i = 0; i < count; i++) {
        char fp[96];
        snprintf(fp, sizeof(fp), "%s/%s", path, names[i]);
        SD_MMC.remove(fp);
    }
    Serial.printf("[SD] Wiped %s: %d files\n", path, count);
    // If more than 64, recurse
    if (count >= 64) wipeDir(path);
}

void wipeSD() {
    Serial.println("[SD] WIPING ALL DATA...");
    wipeDir("/DCIM/video");
    wipeDir("/DCIM/photos");
    Serial.printf("[SD] Done. %.1f GB free\n",
        (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / 1073741824.0);
}

// ══════════════════════════════════════════════════════════════════════════
// RECORDING TASK — Core 1
// Camera stays in JPEG mode. Captures frames continuously into AVI segments.
// Copies each frame to shared preview buffer for Core 0 to decode.
// ══════════════════════════════════════════════════════════════════════════
void recordingTask(void *param) {
    Serial.println("[REC] Recording task started on core 1");
    delay(2000);  // let setup() finish

    AVIWriter avi;
    unsigned long segStart = 0;
    unsigned long frameTimer = 0;
    uint32_t frameMsTarget = 1000 / TARGET_FPS;
    uint32_t fpsCounter = 0;
    unsigned long fpsTimer = millis();

    while (1) {
        // ── Handle SD wipe request ──
        if (wipeRequested) {
            if (avi.isOpen()) { avi.end(); recording = false; }
            wipeSD();
            wipeRequested = false;
            segmentCount = 0;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // ── Paused? Close current AVI and wait ──
        if (!recordingEnabled) {
            if (avi.isOpen()) {
                avi.end();
                recording = false;
                Serial.println("[REC] Paused — segment closed");
                segmentCount++;
            }
            // Still capture for preview even when paused
            if (xSemaphoreTake(camMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
                camera_fb_t *fb = esp_camera_fb_get();
                if (fb && fb->format == PIXFORMAT_JPEG) {
                    if (xSemaphoreTake(previewMutex, 0) == pdTRUE) {
                        if (fb->len <= 100 * 1024) {
                            memcpy(previewBuf, fb->buf, fb->len);
                            previewLen = fb->len;
                            previewReady = true;
                        }
                        xSemaphoreGive(previewMutex);
                    }
                    esp_camera_fb_return(fb);
                } else if (fb) { esp_camera_fb_return(fb); }
                xSemaphoreGive(camMutex);
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        // ── Start new segment if needed ──
        if (!avi.isOpen()) {
            cleanupOldVideos();
            char fn[64];
            snprintf(fn, sizeof(fn), "/DCIM/video/REC_%lu.avi", millis() / 1000);
            if (avi.begin(fn, 640, 480, TARGET_FPS)) {
                segStart = millis();
                recording = true;
                recFrames = 0;
                Serial.printf("[REC] Started segment: %s\n", fn);
            } else {
                Serial.println("[REC] Failed to create AVI!");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
        }

        // ── Frame rate control ──
        unsigned long now = millis();
        if (now - frameTimer < frameMsTarget) {
            vTaskDelay(pdMS_TO_TICKS(5));
            continue;
        }
        frameTimer = now;

        // ── Capture JPEG frame ──
        if (xSemaphoreTake(camMutex, pdMS_TO_TICKS(500)) != pdTRUE) {
            continue;
        }

        camera_fb_t *fb = esp_camera_fb_get();
        if (fb && fb->format == PIXFORMAT_JPEG) {
            // Write to AVI
            avi.addFrame(fb->buf, fb->len);
            recFrames = avi.frames();

            // Copy to preview buffer for Core 0
            if (xSemaphoreTake(previewMutex, 0) == pdTRUE) {
                if (fb->len <= 100 * 1024) {  // sanity check
                    memcpy(previewBuf, fb->buf, fb->len);
                    previewLen = fb->len;
                    previewReady = true;
                }
                xSemaphoreGive(previewMutex);
            }

            // FPS measurement
            fpsCounter++;
            if (now - fpsTimer >= 2000) {
                recFPS = fpsCounter * 1000.0 / (now - fpsTimer);
                fpsCounter = 0;
                fpsTimer = now;
            }

            esp_camera_fb_return(fb);
        } else {
            if (fb) esp_camera_fb_return(fb);
        }

        xSemaphoreGive(camMutex);

        // ── End segment after SEGMENT_SECONDS ──
        if (millis() - segStart >= SEGMENT_SECONDS * 1000UL) {
            avi.end();
            recording = false;
            Serial.printf("[REC] Segment done: %u frames, %.1f FPS avg\n",
                recFrames, recFrames / (float)SEGMENT_SECONDS);
            segmentCount++;
            // Next loop iteration will start a new segment
        }
    }
}

// ══════════════════════════════════════════════════════════════════════════
// SETUP
// ══════════════════════════════════════════════════════════════════════════
void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n=== Dashcam v0.6 (MJPEG video) ===\n");

    pinMode(BTN_PIN, INPUT_PULLUP);
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.setClock(400000);

    initPMU();
    initDisplay();

    canvas->setTextSize(2);
    canvas->setTextColor(WHITE);
    canvas->setCursor(10, 20);
    canvas->print("Dashcam v0.6");
    canvas->flush();

    sdReady = initSD();
    canvas->setCursor(10, 50);
    canvas->printf("SD: %s", sdReady ? "OK" : "NO SD");
    canvas->flush();

    // Allocate PSRAM buffers
    previewBuf = (uint8_t*)ps_malloc(100 * 1024);   // 100KB for JPEG preview (PSRAM)
    jpgWorkBuf = (uint8_t*)malloc(JPG_WORK_SIZE);    // 4KB TJpgDec work area (SRAM!)
    if (!previewBuf || !jpgWorkBuf) {
        canvas->setCursor(10, 80);
        canvas->print("PSRAM FAIL");
        canvas->flush();
        Serial.println("PSRAM allocation failed!");
        while (1) delay(1000);
    }
    Serial.printf("PSRAM: %u KB free\n", ESP.getFreePsram() / 1024);

    // Init camera in JPEG mode (stays in JPEG for recording)
    camera_config_t cfg = {};
    fillCamPins(cfg);
    cfg.pixel_format = PIXFORMAT_JPEG;
    cfg.frame_size = FRAMESIZE_VGA;
    cfg.jpeg_quality = JPEG_QUALITY;
    cfg.fb_count = 2;
    cfg.grab_mode = CAMERA_GRAB_LATEST;

    canvas->setCursor(10, 80);
    if (esp_camera_init(&cfg) != ESP_OK) {
        canvas->print("Camera: FAIL");
        canvas->flush();
        while (1) delay(1000);
    }

    sensor_t *s = esp_camera_sensor_get();
    if (s) s->set_vflip(s, 1);

    canvas->print("Camera: OK (JPEG)");
    canvas->setCursor(10, 110);
    canvas->print("Recording...");
    canvas->flush();
    delay(1500);

    // Create mutexes
    camMutex = xSemaphoreCreateMutex();
    previewMutex = xSemaphoreCreateMutex();

    // Launch recording task on Core 1
    if (sdReady) {
        xTaskCreatePinnedToCore(recordingTask, "rec", 12288, NULL, 2, NULL, 1);
    }

    Serial.println("Ready! Preview on core 0, recording on core 1");
}

// ══════════════════════════════════════════════════════════════════════════
// MAIN LOOP — Core 0 — LCD preview + HUD
// Decodes JPEG from shared buffer → RGB565 → canvas → LCD
// ══════════════════════════════════════════════════════════════════════════
unsigned long lastPreviewDecode = 0;
float previewFPS = 0;
uint32_t previewFrameCount = 0;
unsigned long previewFPSTimer = 0;

void loop() {
    // ── Decode preview JPEG if available ──
    // Uses TJpgDec ROM at 1/2 scale: 640x480→320x240, row-doubled to 320x480
    // Direct RGB565 to canvas — no intermediate buffer needed
    if (previewReady && xSemaphoreTake(previewMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        // Copy JPEG from PSRAM → SRAM to avoid bus contention during decode
        size_t jpgLen = previewLen;
        uint8_t* sramJpg = (uint8_t*)malloc(jpgLen);
        if (sramJpg) memcpy(sramJpg, previewBuf, jpgLen);
        previewReady = false;
        xSemaphoreGive(previewMutex);  // release early — we have a local copy

        unsigned long tDec = millis();
        uint16_t *canvasBuf = (uint16_t *)canvas->getFramebuffer();
        if (sramJpg) {
            uint8_t scale = previewScale;
            JpgCtx ctx = { sramJpg, jpgLen, 0, canvasBuf, scale };
            JDEC jd;
            if (jd_prepare(&jd, jpgReadCb, jpgWorkBuf, JPG_WORK_SIZE, &ctx) == JDR_OK) {
                jd_decomp(&jd, jpgWriteCb, scale);
            }
            free(sramJpg);
        }
        tDec = millis() - tDec;

        previewFrameCount++;
        unsigned long now = millis();
        if (now - previewFPSTimer >= 5000) {
            previewFPS = previewFrameCount * 1000.0 / (now - previewFPSTimer);
            previewFrameCount = 0;
            previewFPSTimer = now;
            Serial.printf("[LCD] %.1f fps (dec=%lums) | [REC] %.1f fps | scale=%d\n",
                previewFPS, tDec, recFPS, previewScale);
        }
    }

    // ── HUD overlay ──
    uint16_t *canvasBuf = (uint16_t *)canvas->getFramebuffer();

    // Dark bar background at top (y=0..30 in portrait = left strip in landscape)
    for (int y = 0; y < 34; y++)
        for (int x = 0; x < 320; x++)
            canvasBuf[y * 320 + x] = 0x0000;

    canvas->setTextSize(1);

    // Row 1: Recording indicator + segment info
    canvas->setCursor(4, 2);
    if (!recordingEnabled) {
        canvas->setTextColor(YELLOW, BLACK);
        canvas->print("PAUSED");
    } else if (recording) {
        canvas->setTextColor(RED, BLACK);
        canvas->print("REC");
        canvas->setTextColor(WHITE, BLACK);
        canvas->printf(" %03u frm %.0ffps", recFrames, recFPS);
    } else {
        canvas->setTextColor(YELLOW, BLACK);
        canvas->print("STARTING...");
    }

    // Row 2: Battery + power
    canvas->setCursor(4, 12);
    canvas->setTextColor(WHITE, BLACK);
    if (pmu.isBatteryConnect()) {
        float batV = pmu.getBattVoltage() / 1000.0;
        int batPct = pmu.getBatteryPercent();
        bool charging = pmu.isCharging();
        uint16_t batColor = batPct > 20 ? GREEN : RED;
        canvas->setTextColor(batColor, BLACK);
        canvas->printf("BAT %d%%", batPct);
        canvas->setTextColor(WHITE, BLACK);
        canvas->printf(" %.2fV", batV);
        if (charging) {
            canvas->setTextColor(YELLOW, BLACK);
            canvas->print(" CHG");
        }
    } else {
        canvas->setTextColor(WHITE, BLACK);
        canvas->printf("USB PWR");
    }

    // Row 3: Storage + counts
    canvas->setCursor(4, 22);
    canvas->setTextColor(WHITE, BLACK);
    float freeGB = (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / 1073741824.0;
    uint16_t sdColor = freeGB > 1.0 ? WHITE : RED;
    canvas->setTextColor(sdColor, BLACK);
    canvas->printf("%.1fGB", freeGB);
    canvas->setTextColor(WHITE, BLACK);
    unsigned long upSec = millis() / 1000;
    canvas->printf(" %02lu:%02lu:%02lu", upSec / 3600, (upSec / 60) % 60, upSec % 60);

    // Preview FPS (small, bottom area)
    canvas->setCursor(4, 470);
    canvas->setTextColor(DARKGREY, BLACK);
    canvas->printf("LCD:%.0ffps %s", previewFPS, previewScale == 1 ? "HQ" : "FAST");

    // ── HUD status message (temporary, replaces FPS line when active) ──
    if (hudMsgExpiry > 0 && millis() < hudMsgExpiry) {
        canvas->setCursor(4, 470);
        canvas->setTextColor(YELLOW, BLACK);
        canvas->printf("%-28s", hudMsg);
    }

    canvas->flush();

    // ── Multi-function button ──
    // Short press (<1s): 5MP photo
    // Double press (2x within 500ms): toggle recording
    // Long press (>3s): wipe SD (then confirm with another press)
    static bool wipeConfirmPending = false;
    static unsigned long wipeConfirmExpiry = 0;

    if (digitalRead(BTN_PIN) == LOW) {
        unsigned long pressStart = millis();

        // Wait for release or long-press threshold
        while (digitalRead(BTN_PIN) == LOW && (millis() - pressStart) < 3000) {
            // Show hold progress on HUD
            if (millis() - pressStart > 500) {
                unsigned long held = millis() - pressStart;
                snprintf(hudMsg, sizeof(hudMsg), "HOLD %lus... (3s=WIPE)", held / 1000);
                hudMsgExpiry = millis() + 500;
            }
            delay(50);
        }

        unsigned long pressDur = millis() - pressStart;

        if (pressDur >= 3000) {
            // ── Long press: wipe SD ──
            if (wipeConfirmPending) {
                // Second long press = confirmed!
                wipeConfirmPending = false;
                wipeRequested = true;
                snprintf(hudMsg, sizeof(hudMsg), "WIPING SD...");
                hudMsgExpiry = millis() + 5000;
                Serial.println("SD WIPE CONFIRMED");
            } else {
                // First long press = ask for confirmation
                wipeConfirmPending = true;
                wipeConfirmExpiry = millis() + 5000;
                snprintf(hudMsg, sizeof(hudMsg), "WIPE? Hold 3s again!");
                hudMsgExpiry = millis() + 5000;
                Serial.println("SD wipe requested — hold again to confirm");
            }
        } else {
            wipeConfirmPending = false;  // any short press cancels wipe

            // Check for double press
            unsigned long releaseTime = millis();
            bool doublePress = false;
            while (millis() - releaseTime < 400) {
                if (digitalRead(BTN_PIN) == LOW) {
                    doublePress = true;
                    delay(200);  // debounce second press
                    break;
                }
                delay(10);
            }

            if (doublePress) {
                // ── Double press: toggle recording ──
                recordingEnabled = !recordingEnabled;
                snprintf(hudMsg, sizeof(hudMsg), "REC: %s",
                    recordingEnabled ? "ON" : "PAUSED");
                hudMsgExpiry = millis() + 3000;
                Serial.printf("Recording %s\n", recordingEnabled ? "enabled" : "paused");
            } else {
                // ── Single press: toggle preview quality ──
                previewScale = (previewScale == 1) ? 2 : 1;
                snprintf(hudMsg, sizeof(hudMsg), "PREVIEW: %s",
                    previewScale == 1 ? "HQ (1/2)" : "FAST (1/4)");
                hudMsgExpiry = millis() + 2000;
                Serial.printf("Preview scale: %d\n", previewScale);
            }
        }

        delay(100);  // final debounce
    }

    // Expire wipe confirmation
    if (wipeConfirmPending && millis() > wipeConfirmExpiry) {
        wipeConfirmPending = false;
        snprintf(hudMsg, sizeof(hudMsg), "Wipe cancelled");
        hudMsgExpiry = millis() + 2000;
    }
}
