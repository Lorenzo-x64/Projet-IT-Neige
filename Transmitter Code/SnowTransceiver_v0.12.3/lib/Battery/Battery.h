// ============================================================================
//  Battery.h  --  Li-Po battery monitor for uPesy ESP32 LP board
//
//  No external wiring required: the uPesy LP board has the divider on-board
//  on GPIO35.  We just read the ADC and apply the official conversion ratio.
//
//  Reported values:
//      voltage()         -- battery voltage in V (or NaN if not yet read)
//      percent()         -- 0..100 SoC%, linear interpolation between
//                           BATTERY_EMPTY_V (0%) and BATTERY_FULL_V (100%)
//      onUsb()           -- true when the raw voltage looks like V_BUS rather
//                           than V_BAT (i.e. USB-C cable plugged in).
//      isLow()           -- voltage < BATTERY_LOW_V (false when on USB)
//
//      dischargeRatePctPerHour() -- linear regression slope of SoC over the
//          rolling history window.  Negative when discharging, positive when
//          charging.  NaN until the window has enough samples.
//      hoursRemaining()  -- pct / |rate| if discharging.  NaN otherwise.
//      historyMinutesAvailable() -- how many minutes of history we have so
//          the UI knows whether to label the estimate as "rough" or "stable".
// ============================================================================
#pragma once

#include <Arduino.h>
#include "Config.h"

// ----------------------------------------------------------------------------
//  Rolling-history sizing.  We sample SoC at one-minute intervals and keep
//  60 samples = the last hour.  Linear regression over a one-hour window is
//  noisy on Li-Po flat-zone discharge but fine for an at-a-glance estimate.
//  60 samples * 8 bytes = 480 bytes RAM -- trivial.
// ----------------------------------------------------------------------------
#define BATTERY_HISTORY_INTERVAL_MS    60000UL   // 1 sample per minute
#define BATTERY_HISTORY_SIZE           60        // 60 minutes total
#define BATTERY_HISTORY_MIN_SAMPLES    5         // need 5 min before extrapolating

class Battery {
public:
    void  begin();
    void  loop();
    void  printStats() const;

    bool   ready()    const { return _samples > 0; }
    float  voltage()  const { return _voltage; }
    float  percent()  const;
    bool   onUsb()    const { return _onUsb; }
    bool   isLow()    const { return !_onUsb && _voltage < BATTERY_LOW_V; }

    // Smoothed running average of the most recent reading (V)
    float  raw()      const { return _rawAvg; }

    // Rolling estimates -- NaN until enough history accumulated
    float  dischargeRatePctPerHour() const;     // negative = discharging
    float  hoursRemaining() const;              // NaN if not discharging
    uint32_t historyMinutesAvailable() const;

private:
    uint32_t _lastReadMs    = 0;
    uint32_t _samples       = 0;
    float    _voltage       = NAN;
    float    _rawAvg        = 0;
    bool     _onUsb         = false;

    // Rolling SoC history
    struct Sample { uint32_t bootMs; float pct; };
    Sample   _history[BATTERY_HISTORY_SIZE];
    uint8_t  _histHead      = 0;     // next slot to write
    uint8_t  _histFill      = 0;     // 0..BATTERY_HISTORY_SIZE
    uint32_t _lastHistMs    = 0;

    float _readOnce() const;
    void  _pushHistory(float pct);
};

extern Battery battery;
