// ============================================================================
//  SDLogger.h  --  micro-SD card management
//
//  Built on top of the user's verified SPI wiring:
//      CS=5  SCK=18  MISO=19  MOSI=23
//
//  CSV format is internationalised RFC 4180:
//      - field separator  : ','
//      - decimal separator: '.'
//      - line terminator  : '\r\n'
//      - timestamp format : ISO 8601 UTC (e.g. 2026-05-04T12:34:56Z)
//
//  Two log files live side-by-side on the card:
//
//   /snowlog*.csv    -- main per-sample log (rotation-aware)
//      Header: timestamp,distance_cm,depth_cm,outlier,event,
//              lat,lon,alt_m,speed_kmh,sats,hdop
//      "event" is empty most of the time; says "FALL" or "IMPACT" on rows
//      whose sample coincides with a freshly fired BMI160 event.
//
//   /falllog.csv     -- dedicated fall/impact log (single file, ever-growing)
//      Header: timestamp,event,magnitude_g,ax_g,ay_g,az_g,duration_ms,
//              lat,lon,alt_m,sats
//      Append-only.  Survives clearing the snow log.  Survives rotation
//      changes.  This is the redundant copy that guarantees a fall is
//      never lost even if a snowlog write happened to fail.
// ============================================================================
#pragma once

#include <Arduino.h>
#include <vector>

struct SDFileInfo {
    String name;
    size_t size;
};

class SDLogger {
public:
    void  begin();
    bool  isReady()              const { return _ready; }
    uint32_t rowCount()          const { return _rowCount; }
    bool  isNearlyFull()         const { return _nearlyFull; }
    void  printStats()           const;

    // Path of the file we are currently appending to (after rotation).
    String activeFile() const          { return _activeFile; }

    uint64_t totalBytes()        const;
    uint64_t usedBytes()         const;
    uint64_t freeBytes()         const;

    // Append one CSV row to the active snow log using the current ISO 8601
    // timestamp.  `event` is empty for normal rows or "FALL"/"IMPACT" when
    // the sample coincides with a fresh BMI160 event.
    bool  logRow(float distanceCm, float depthCm,
                 bool  outlierFlag = false,
                 const char* event = "",
                 float lat = NAN, float lon = NAN,
                 float altM = NAN, float speedKmh = NAN,
                 uint32_t sats = 0, float hdop = NAN,
                 float tempC = NAN, float humPct = NAN);

    // Last successfully written CSV row (no trailing CRLF).  Used by the
    // LoRa transmitter to mirror the row over the air.  Empty until the
    // first successful logRow() call.
    const String& lastRow() const   { return _lastRow; }

    // Append one entry to the dedicated /falllog.csv.  Called immediately
    // by the IMU when it fires a fall or impact event.
    bool  logFallEvent(const char* type, float mag,
                       float ax, float ay, float az,
                       uint32_t durationMs);

    // File operations
    std::vector<SDFileInfo> listCsvFiles();
    String                  previewFile(const char* path, size_t maxLines = 50);

    // Dangerous: erase only the active log file (header rewritten).
    bool  clearLog();

    // Re-emit the header into the given file if missing.
    void  writeHeaderIfNeeded(const char* path);

    // Periodic call from loop() -- checks free space, retries init.
    void  loop();

private:
    bool     _ready          = false;
    uint32_t _rowCount       = 0;
    bool     _nearlyFull     = false;
    uint32_t _lastFullCheck  = 0;
    uint32_t _lastInitAttempt = 0;
    String   _activeFile     = "/snowlog.csv";   // updated by rotation logic
    String   _lastRow;                            // most recent CSV line
                                                  // (no trailing \r\n)

    bool     _tryInit();
    void     _checkSpace();
    uint32_t _countRows(const char* path);
    String   _decideActiveFile();                // based on AppSettings
    void     _ensureFallLogHeader();             // /falllog.csv
};

extern SDLogger sdLogger;
