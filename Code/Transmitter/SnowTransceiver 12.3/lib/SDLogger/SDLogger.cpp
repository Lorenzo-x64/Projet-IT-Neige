// ============================================================================
//  SDLogger.cpp
// ============================================================================
#include "SDLogger.h"
#include "Config.h"
#include "AppSettings.h"
#include "TimeKeeper.h"
#include "Gps.h"
#include <SPI.h>
#include <SD.h>
#include <time.h>

SDLogger sdLogger;

// ----------------------------------------------------------------------------
// CSV column header. Bumping a column? Bump it everywhere or files written
// before the change will not concatenate cleanly with newer ones.
// ----------------------------------------------------------------------------
static const char* CSV_HEADER =
    "timestamp,distance_cm,depth_cm,outlier,event,"
    "lat,lon,alt_m,speed_kmh,sats,hdop,temp_c,humidity_pct\r\n";

// Dedicated fall/impact log -- single file, append-only, append-forever.
static const char* FALL_LOG_FILE   = "/falllog.csv";
static const char* FALL_LOG_HEADER =
    "timestamp,event,magnitude_g,ax_g,ay_g,az_g,duration_ms,"
    "lat,lon,alt_m,sats\r\n";

// ----------------------------------------------------------------------------
//  Internal init helper (multi-strategy SD bring-up).
// ----------------------------------------------------------------------------
bool SDLogger::_tryInit() {
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    delay(50);

    if (SD.begin(SD_CS_PIN)) {
        DBG("SD", "init OK (default mode, CS=GPIO%d)", SD_CS_PIN);
        return true;
    }
    DBG("SD", "default init failed, trying explicit SPI @ 4 MHz...");
    SD.end();
    delay(50);

    SPI.end();
    delay(20);
    SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
    if (SD.begin(SD_CS_PIN, SPI, 4000000UL)) {
        DBG("SD", "init OK (explicit SPI @ 4 MHz)");
        return true;
    }
    DBG("SD", "4 MHz failed, trying 1 MHz (last attempt)...");
    SD.end();
    delay(50);

    if (SD.begin(SD_CS_PIN, SPI, 1000000UL)) {
        DBG("SD", "init OK (explicit SPI @ 1 MHz)");
        return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
void SDLogger::begin() {
    _lastInitAttempt = millis();

    if (!_tryInit()) {
        _ready = false;
        Serial.println();
        Serial.println(F("[ SD    ] ============================================"));
        Serial.println(F("[ SD    ]   !! SD CARD NOT DETECTED !!"));
        Serial.println(F("[ SD    ] --------------------------------------------"));
        Serial.println(F("[ SD    ] Wiring checklist (verify with multimeter):"));
        Serial.printf ( "[ SD    ]   - 3.3 V power present at SD module?\n");
        Serial.printf ( "[ SD    ]   - CS   = GPIO%d  -> orange/yellow wire?\n", SD_CS_PIN);
        Serial.printf ( "[ SD    ]   - MOSI = GPIO%d  -> on SD module 'DI'?\n",  SD_MOSI_PIN);
        Serial.printf ( "[ SD    ]   - MISO = GPIO%d  -> on SD module 'DO'?\n",  SD_MISO_PIN);
        Serial.printf ( "[ SD    ]   - CLK  = GPIO%d  -> on SD module 'CLK'?\n", SD_SCK_PIN);
        Serial.printf ( "[ SD    ]   - GND common between MCU and module?\n");
        Serial.println(F("[ SD    ] Card checklist:"));
        Serial.println(F("[ SD    ]   - Card actually inserted in the slot?"));
        Serial.println(F("[ SD    ]   - Card is FAT32 (NOT exFAT, NOT NTFS)?"));
        Serial.println(F("[ SD    ]   - Card capacity <= 32 GB?"));
        Serial.println(F("[ SD    ]   - Try shorter jumper wires (max ~10 cm)"));
        Serial.println(F("[ SD    ] (Will retry every 30 s in case card is inserted later)"));
        Serial.println(F("[ SD    ] ============================================"));
        return;
    }
    _ready = true;

    uint8_t cardType = SD.cardType();
    const char* typeStr = "UNKNOWN";
    switch (cardType) {
        case CARD_NONE:  typeStr = "NONE";   break;
        case CARD_MMC:   typeStr = "MMC";    break;
        case CARD_SD:    typeStr = "SDSC";   break;
        case CARD_SDHC:  typeStr = "SDHC";   break;
    }
    uint64_t cardSizeMB = SD.cardSize() / (1024ULL * 1024ULL);
    DBG("SD", "type=%s  size=%lluMB  total=%lluKB  used=%lluKB",
        typeStr,
        (unsigned long long)cardSizeMB,
        (unsigned long long)(SD.totalBytes() / 1024ULL),
        (unsigned long long)(SD.usedBytes()  / 1024ULL));

    _activeFile = _decideActiveFile();
    writeHeaderIfNeeded(_activeFile.c_str());
    _ensureFallLogHeader();
    _rowCount = _countRows(_activeFile.c_str());
    DBG("SD", "active log %s contains %u data rows",
        _activeFile.c_str(), _rowCount);
}

// ----------------------------------------------------------------------------
//  Decide which file we should be writing to right now, based on settings
//  and the current wall-clock time.  Returns a path that begins with '/'.
// ----------------------------------------------------------------------------
String SDLogger::_decideActiveFile() {
    Rotation r = settings.rotation();

    // No rotation OR no real wall-clock yet -> single file.
    // (We refuse to rotate by date when timekeeper hasn't been set, otherwise
    // we'd write everything into "snowlog-1970-01-01.csv".)
    if (r == Rotation::None || !timekeeper.hasRealClock()) {
        if (r == Rotation::Rows) {
            // Row-based rotation works even without a clock.
            // Find the highest-indexed file or start at 1.
            // Implemented below; fall through.
        } else {
            return String("/snowlog.csv");
        }
    }

    if (r == Rotation::Daily) {
        return String("/snowlog-") + timekeeper.dateString() + ".csv";
    }
    if (r == Rotation::Weekly) {
        // ISO week number "YYYY-Www"
        time_t now = time(nullptr);
        struct tm t;
        gmtime_r(&now, &t);
        char buf[24];
        // strftime %V is ISO 8601 week number; %G is the ISO week-numbering year
        strftime(buf, sizeof(buf), "/snowlog-%G-W%V.csv", &t);
        return String(buf);
    }
    if (r == Rotation::Monthly) {
        time_t now = time(nullptr);
        struct tm t;
        gmtime_r(&now, &t);
        char buf[24];
        strftime(buf, sizeof(buf), "/snowlog-%Y-%m.csv", &t);
        return String(buf);
    }
    if (r == Rotation::Rows) {
        // Find the highest snowlog-NNNNNN.csv whose row-count is below the
        // limit; if none, advance to a new index.
        uint32_t limit = settings.rotateRows();
        uint32_t bestIdx = 0;
        File root = SD.open("/");
        if (root && root.isDirectory()) {
            File entry = root.openNextFile();
            while (entry) {
                String n = entry.name();
                // Names look like "snowlog-000001.csv"
                int dash = n.indexOf('-');
                int dot  = n.lastIndexOf('.');
                if (n.startsWith("snowlog-") && dash > 0 && dot > dash) {
                    uint32_t idx = (uint32_t) n.substring(dash + 1, dot).toInt();
                    if (idx > bestIdx) bestIdx = idx;
                }
                entry = root.openNextFile();
            }
            root.close();
        }
        if (bestIdx == 0) bestIdx = 1;
        char buf[32];
        snprintf(buf, sizeof(buf), "/snowlog-%06lu.csv", (unsigned long)bestIdx);
        // If the candidate is full, advance one.
        if (_countRows(buf) >= limit) {
            snprintf(buf, sizeof(buf), "/snowlog-%06lu.csv", (unsigned long)(bestIdx + 1));
        }
        return String(buf);
    }
    return String("/snowlog.csv");
}

// ----------------------------------------------------------------------------
void SDLogger::writeHeaderIfNeeded(const char* path) {
    if (!_ready) return;
    if (SD.exists(path)) return;

    File f = SD.open(path, FILE_WRITE);
    if (!f) {
        DBG("SD", "could not create %s", path);
        return;
    }
    f.print(CSV_HEADER);
    f.close();
    DBG("SD", "header written to %s", path);
}

// ----------------------------------------------------------------------------
uint64_t SDLogger::totalBytes() const { return _ready ? SD.totalBytes() : 0; }
uint64_t SDLogger::usedBytes()  const { return _ready ? SD.usedBytes()  : 0; }
uint64_t SDLogger::freeBytes()  const {
    if (!_ready) return 0;
    uint64_t t = SD.totalBytes(), u = SD.usedBytes();
    return (t > u) ? (t - u) : 0;
}

// ----------------------------------------------------------------------------
bool SDLogger::logRow(float distanceCm, float depthCm, bool outlierFlag,
                      const char* event,
                      float lat, float lon, float altM, float speedKmh,
                      uint32_t sats, float hdop,
                      float tempC, float humPct) {
    if (!_ready) return false;

    // Re-evaluate the active filename in case rotation conditions changed
    // (date rolled over, row limit hit, etc.).
    String target = _decideActiveFile();
    if (target != _activeFile) {
        _activeFile = target;
        writeHeaderIfNeeded(_activeFile.c_str());
        _rowCount = _countRows(_activeFile.c_str());
        DBG("SD", "rotated to %s (rows=%u)", _activeFile.c_str(), _rowCount);
    }

    // Build the row into _lastRow first so the LoRa transmitter can reuse
    // it without re-formatting.  No trailing CRLF here -- we add it when
    // writing to disk.
    char buf[24];
    _lastRow = timekeeper.iso8601();
    _lastRow += ',';
    if (!isnan(distanceCm))         { snprintf(buf, sizeof(buf), "%.1f", distanceCm); _lastRow += buf; }
    _lastRow += ',';
    if (!isnan(depthCm) && depthCm >= 0) { snprintf(buf, sizeof(buf), "%.1f", depthCm); _lastRow += buf; }
    _lastRow += ',';
    _lastRow += outlierFlag ? '1' : '0';
    _lastRow += ',';
    if (event && *event)            { _lastRow += event; }
    _lastRow += ',';
    if (!isnan(lat))                { snprintf(buf, sizeof(buf), "%.6f", lat); _lastRow += buf; }
    _lastRow += ',';
    if (!isnan(lon))                { snprintf(buf, sizeof(buf), "%.6f", lon); _lastRow += buf; }
    _lastRow += ',';
    if (!isnan(altM))               { snprintf(buf, sizeof(buf), "%.1f", altM); _lastRow += buf; }
    _lastRow += ',';
    if (!isnan(speedKmh))           { snprintf(buf, sizeof(buf), "%.1f", speedKmh); _lastRow += buf; }
    _lastRow += ',';
    if (sats > 0)                   { snprintf(buf, sizeof(buf), "%lu", (unsigned long)sats); _lastRow += buf; }
    _lastRow += ',';
    if (!isnan(hdop))               { snprintf(buf, sizeof(buf), "%.1f", hdop); _lastRow += buf; }
    _lastRow += ',';
    if (!isnan(tempC))              { snprintf(buf, sizeof(buf), "%.2f", tempC);  _lastRow += buf; }
    _lastRow += ',';
    if (!isnan(humPct))             { snprintf(buf, sizeof(buf), "%.1f", humPct); _lastRow += buf; }

    File f = SD.open(_activeFile.c_str(), FILE_APPEND);
    if (!f) {
        DBG("SD", "open append failed: %s", _activeFile.c_str());
        return false;
    }
    f.print(_lastRow);
    f.print("\r\n");
    f.close();

    _rowCount++;
    Serial.printf("[ LOG   ] row %u%s%s -> dist=%.1fcm depth=%.1fcm  (%s)\n",
                  (unsigned)_rowCount,
                  outlierFlag ? "  *FLAG*" : "       ",
                  (event && *event) ? "  EVENT" : "",
                  distanceCm, depthCm,
                  _activeFile.c_str());
    return true;
}

// ----------------------------------------------------------------------------
//  Append one entry to /falllog.csv.  Reads live GPS to capture position at
//  the moment of the event.  Writes immediately and closes the file so a
//  subsequent power loss after the event can never lose the entry.
// ----------------------------------------------------------------------------
bool SDLogger::logFallEvent(const char* type, float mag,
                            float ax, float ay, float az,
                            uint32_t durationMs) {
    if (!_ready) {
        DBG("SD", "fall event NOT logged -- SD offline (type=%s mag=%.2f)",
            type ? type : "?", mag);
        return false;
    }
    _ensureFallLogHeader();

    File f = SD.open(FALL_LOG_FILE, FILE_APPEND);
    if (!f) {
        DBG("SD", "fall log open failed: %s", FALL_LOG_FILE);
        return false;
    }

    f.print(timekeeper.iso8601());
    f.print(',');
    f.print(type ? type : "");
    f.printf(",%.3f,%+.3f,%+.3f,%+.3f,%lu,",
             mag, ax, ay, az, (unsigned long)durationMs);
    if (!isnan(gps.latitude()))   f.printf("%.6f", gps.latitude());
    f.print(',');
    if (!isnan(gps.longitude()))  f.printf("%.6f", gps.longitude());
    f.print(',');
    if (!isnan(gps.altitudeM()))  f.printf("%.1f", gps.altitudeM());
    f.print(',');
    if (gps.satellites() > 0)     f.printf("%lu", (unsigned long)gps.satellites());
    f.print("\r\n");
    f.close();

    Serial.printf("[ FALLOG] %s %.2fg ax=%+.2f ay=%+.2f az=%+.2f dur=%lums -> %s\n",
                  type, mag, ax, ay, az, (unsigned long)durationMs, FALL_LOG_FILE);
    return true;
}

// ----------------------------------------------------------------------------
void SDLogger::_ensureFallLogHeader() {
    if (!_ready) return;
    if (SD.exists(FALL_LOG_FILE)) return;
    File f = SD.open(FALL_LOG_FILE, FILE_WRITE);
    if (!f) return;
    f.print(FALL_LOG_HEADER);
    f.close();
    DBG("SD", "fall log header written to %s", FALL_LOG_FILE);
}

// ----------------------------------------------------------------------------
std::vector<SDFileInfo> SDLogger::listCsvFiles() {
    std::vector<SDFileInfo> out;
    if (!_ready) return out;

    File root = SD.open("/");
    if (!root || !root.isDirectory()) return out;

    File entry = root.openNextFile();
    while (entry) {
        if (!entry.isDirectory()) {
            String name = entry.name();
            // Hide tiles/* and other internal files; show CSVs only.
            if (name.endsWith(".csv") || name.endsWith(".CSV") || name.endsWith(".txt")) {
                SDFileInfo info;
                info.name = (name.startsWith("/")) ? name : ("/" + name);
                info.size = entry.size();
                out.push_back(info);
            }
        }
        entry = root.openNextFile();
    }
    root.close();
    return out;
}

// ----------------------------------------------------------------------------
String SDLogger::previewFile(const char* path, size_t maxLines) {
    if (!_ready) return String("(SD not ready)");
    File f = SD.open(path, FILE_READ);
    if (!f) return String("(file not found)");

    String all;
    while (f.available()) {
        all += (char)f.read();
        if (all.length() > 32768) {
            all = String("...(truncated)...\r\n") + all.substring(all.length() - 16384);
        }
    }
    f.close();

    int firstNl = all.indexOf('\n');
    String header = (firstNl >= 0) ? all.substring(0, firstNl + 1) : String();
    String body   = (firstNl >= 0) ? all.substring(firstNl + 1)    : all;

    int lineCount = 0;
    int idx = body.length();
    while (idx > 0 && lineCount < (int)maxLines - 1) {
        idx--;
        if (body[idx] == '\n') lineCount++;
    }
    String tail = body.substring(idx);
    return header + tail;
}

// ----------------------------------------------------------------------------
bool SDLogger::clearLog() {
    if (!_ready) return false;
    if (SD.exists(_activeFile.c_str())) SD.remove(_activeFile.c_str());
    _rowCount = 0;
    writeHeaderIfNeeded(_activeFile.c_str());
    DBG("SD", "log file cleared: %s", _activeFile.c_str());
    return true;
}

// ----------------------------------------------------------------------------
void SDLogger::loop() {
    // Retry init every 30 s if not ready
    if (!_ready) {
        if (millis() - _lastInitAttempt < 30000UL) return;
        _lastInitAttempt = millis();
        DBG("SD", "retrying init...");
        if (_tryInit()) {
            _ready = true;
            uint64_t mb = SD.cardSize() / (1024ULL * 1024ULL);
            DBG("SD", "card detected on retry (size=%lluMB)", (unsigned long long)mb);
            _activeFile = _decideActiveFile();
            writeHeaderIfNeeded(_activeFile.c_str());
            _ensureFallLogHeader();
            _rowCount = _countRows(_activeFile.c_str());
        }
        return;
    }

    uint32_t now = millis();
    if (now - _lastFullCheck < SD_FULL_CHECK_INTERVAL) return;
    _lastFullCheck = now;
    _checkSpace();
}

// ----------------------------------------------------------------------------
void SDLogger::_checkSpace() {
    uint64_t total = SD.totalBytes();
    uint64_t used  = SD.usedBytes();
    if (total == 0) return;
    float pct = (float)used / (float)total * 100.0f;
    if (pct > SD_FULL_THRESHOLD_PCT && !_nearlyFull) {
        _nearlyFull = true;
        DBG("SD", "WARNING: %.1f%% full", pct);
    } else if (pct < (SD_FULL_THRESHOLD_PCT - 5.0f) && _nearlyFull) {
        _nearlyFull = false;
    }
}

// ----------------------------------------------------------------------------
uint32_t SDLogger::_countRows(const char* path) {
    if (!_ready) return 0;
    File f = SD.open(path, FILE_READ);
    if (!f) return 0;
    uint32_t n = 0;
    while (f.available()) {
        if (f.read() == '\n') n++;
    }
    f.close();
    return (n > 0) ? (n - 1) : 0;       // first line is the header
}

// ----------------------------------------------------------------------------
void SDLogger::printStats() const {
    if (!_ready) {
        Serial.println(F("[ SD    ] OFFLINE  (card not detected)"));
        return;
    }
    uint64_t total = totalBytes();
    uint64_t used  = usedBytes();
    float pct = total ? (float)used / (float)total * 100.0f : 0.0f;

    auto fmtMB = [](uint64_t b) -> String {
        char buf[16];
        if (b < 1024ULL * 1024ULL)
            snprintf(buf, sizeof(buf), "%luKB", (unsigned long)(b / 1024ULL));
        else if (b < 1024ULL * 1024ULL * 1024ULL)
            snprintf(buf, sizeof(buf), "%luMB", (unsigned long)(b / (1024ULL * 1024ULL)));
        else
            snprintf(buf, sizeof(buf), "%.1fGB", (double)b / (1024.0 * 1024.0 * 1024.0));
        return String(buf);
    };
    Serial.printf("[ SD    ] %-5s  %s / %s (%.1f%%)  rows=%lu  active=%s%s\n",
                  "ok",
                  fmtMB(used).c_str(),
                  fmtMB(total).c_str(),
                  pct,
                  (unsigned long)_rowCount,
                  _activeFile.c_str(),
                  _nearlyFull ? "  WARN: nearly full" : "");
}
