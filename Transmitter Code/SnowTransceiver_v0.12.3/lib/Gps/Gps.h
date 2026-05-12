// ============================================================================
//  Gps.h  --  u-blox NEO-7M (Velleman VMA430-WPI430) driver
//
//  Hardware:
//      VCC -> 3.3 V        (NOT 5 V)
//      GND -> GND
//      TX  -> ESP32 GPS_RX_PIN  (GPIO16)
//      RX  -> ESP32 GPS_TX_PIN  (GPIO17)   (only used if we want to send
//                                          UBX config commands later)
//
//  The module outputs NMEA at 9600 baud.  TinyGPSPlus parses every sentence
//  and exposes structured fields.  We piggy-back on TimeKeeper:  whenever
//  the GPS gets a fix we push the UTC epoch into TimeKeeper -> all CSV
//  timestamps switch from BOOT-relative to real wall-clock automatically.
// ============================================================================
#pragma once

#include <Arduino.h>

class Gps {
public:
    void  begin();
    void  loop();                          // call as often as possible
    void  printStats() const;

    // Live values (NaN / 0 when no fix)
    bool   hasFix() const         { return _hasFix; }
    bool   isFresh(uint32_t maxAgeMs = 5000) const;

    float  latitude()    const    { return _lat; }
    float  longitude()   const    { return _lon; }
    float  altitudeM()   const    { return _altM; }
    float  speedKmh()    const    { return _speedKmh; }
    float  courseDeg()   const    { return _courseDeg; }
    float  hdop()        const    { return _hdop; }
    uint32_t satellites() const   { return _sats; }

    // Statistics
    uint32_t charsProcessed() const { return _chars; }
    uint32_t sentencesWithFix() const { return _sentencesWithFix; }
    uint32_t failedChecksum()  const { return _failedChecksum; }

private:
    bool     _hasFix         = false;
    float    _lat            = NAN;
    float    _lon            = NAN;
    float    _altM           = NAN;
    float    _speedKmh       = NAN;
    float    _courseDeg      = NAN;
    float    _hdop           = NAN;
    uint32_t _sats           = 0;
    uint32_t _chars          = 0;
    uint32_t _sentencesWithFix = 0;
    uint32_t _failedChecksum = 0;
    uint32_t _lastFixMs      = 0;

    // Cached "have we set the system clock from GPS" flag
    bool     _clockPushed    = false;
};

extern Gps gps;
