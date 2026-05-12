// ============================================================================
//  OutlierDetector.h  --  flags "strange" snow-depth readings
//
//  A reading is flagged when EITHER:
//      (a) it differs from the previous reading by more than OUTLIER_JUMP_CM
//          AND the relative change exceeds OUTLIER_JUMP_RATIO
//      (b) it sits more than OUTLIER_SIGMA standard deviations away from
//          the rolling mean of the last OUTLIER_WINDOW samples
//
//  Both checks suppress false positives at startup until the rolling window
//  is fully populated.
// ============================================================================
#pragma once

#include <Arduino.h>
#include "Config.h"

class OutlierDetector {
public:
    void  begin();
    bool  evaluate(float valueCm);            // returns true if flagged
    bool  lastFlagged() const { return _lastFlagged; }
    uint32_t totalFlagged() const { return _totalFlagged; }

private:
    float    _buf[OUTLIER_WINDOW] = {0};
    uint8_t  _idx                 = 0;
    uint8_t  _filled              = 0;        // 0..OUTLIER_WINDOW
    float    _lastVal             = NAN;
    bool     _lastFlagged         = false;
    uint32_t _totalFlagged        = 0;
};

extern OutlierDetector outlier;
