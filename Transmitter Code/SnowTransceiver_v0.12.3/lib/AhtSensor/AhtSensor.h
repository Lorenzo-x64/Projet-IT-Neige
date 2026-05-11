// ============================================================================
//  AhtSensor.h  --  AHT10 Temperature & Humidity sensor driver (no library)
//
//  Uses the shared I2C bus (GPIO21/GPIO22) at address 0x38.
//  Protocol summary (AHT10 datasheet):
//      1. Power-on reset delay ~40 ms
//      2. Init command: 0xBE 0x08 0x00  (wait 10 ms)
//      3. Trigger measurement: 0xAC 0x33 0x00  (wait 80 ms)
//      4. Read 6 bytes: [status][hum[19:12]][hum[11:4]][hum[3:0]|temp[19:16]]
//                       [temp[15:8]][temp[7:0]]
//      Convert:
//          hum%  = (hum_raw  / 1048576.0) * 100
//          temp  = (temp_raw / 1048576.0) * 200 - 50
// ============================================================================
#pragma once

#include <Arduino.h>
#include "Config.h"

class AhtSensor {
public:
    void  begin();
    void  loop();
    void  printStats() const;

    bool  ready()   const { return _ready; }
    bool  isFresh() const {
        return _ready && _lastGoodMs > 0 &&
               (millis() - _lastGoodMs) < AHT10_STALE_MS;
    }

    float tempC()    const { return _tempC; }
    float humidity() const { return _humPct; }

    // Convenience: convert to Fahrenheit for the UI (when imperial units on)
    float tempF()    const { return _tempC * 9.0f / 5.0f + 32.0f; }

private:
    bool     _ready      = false;
    float    _tempC      = NAN;
    float    _humPct     = NAN;
    uint32_t _lastReadMs = 0;
    uint32_t _lastGoodMs = 0;
    uint32_t _errCount   = 0;

    bool _initChip();
    bool _triggerAndRead();
};

extern AhtSensor aht;
