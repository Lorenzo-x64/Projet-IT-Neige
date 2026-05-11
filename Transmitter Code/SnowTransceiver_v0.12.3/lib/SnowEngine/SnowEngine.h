// ============================================================================
//  SnowEngine.h  --  derives snow depth from raw distance + tare
//
//      snowDepth = max(0, tareValue - liveDistance)
//
//  The tare value is the distance from the sensor to the bare ground
//  (no snow).  As snow accumulates, the live distance shrinks; the
//  difference is the depth of accumulated snow.
//
//  Tare can be set in two ways:
//      1. "Tare now"  -> capture the current live reading
//      2. "Manual"    -> user types a known reference value
// ============================================================================
#pragma once

#include <Arduino.h>

class SnowEngine {
public:
    void begin();

    // Compute the current snow depth (cm).  Returns -1 if no fresh sensor reading.
    float snowDepthCm() const;

    // Set tare from the current live reading.  Returns the captured value,
    // or NAN if the sensor reading isn't fresh.
    float tareNow();

    // Set tare to a manually entered value (cm).
    void  setManualTare(float cm);

    // Reset tare back to 0 (snow depth will then equal -distance, clamped).
    void  resetTare();

    float tareCm() const;
};

extern SnowEngine snow;
