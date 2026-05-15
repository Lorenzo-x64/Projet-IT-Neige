// ============================================================================
//  OutlierDetector.cpp
// ============================================================================
#include "OutlierDetector.h"
#include <math.h>

OutlierDetector outlier;

void OutlierDetector::begin() {
    _idx          = 0;
    _filled       = 0;
    _lastVal      = NAN;
    _lastFlagged  = false;
    _totalFlagged = 0;
}

bool OutlierDetector::evaluate(float v) {
    bool flagged = false;

    // ---- Check (a): jump from previous reading ---------------------------
    if (!isnan(_lastVal) && _filled >= 4) {
        float diff = fabsf(v - _lastVal);
        float ratio = (_lastVal > 0.1f) ? diff / _lastVal : (diff > 0.1f ? 99.9f : 0.0f);
        if (diff > OUTLIER_JUMP_CM && ratio > OUTLIER_JUMP_RATIO) flagged = true;
    }

    // ---- Check (b): rolling-window sigma rule ----------------------------
    if (_filled >= OUTLIER_WINDOW) {
        // Mean
        float mean = 0.0f;
        for (uint8_t i = 0; i < OUTLIER_WINDOW; i++) mean += _buf[i];
        mean /= (float)OUTLIER_WINDOW;

        // Standard deviation
        float ss = 0.0f;
        for (uint8_t i = 0; i < OUTLIER_WINDOW; i++) {
            float d = _buf[i] - mean;
            ss += d * d;
        }
        float stdev = sqrtf(ss / (float)OUTLIER_WINDOW);
        if (stdev > 0.001f) {                          // ignore zero-variance window
            float zabs = fabsf(v - mean) / stdev;
            if (zabs > OUTLIER_SIGMA) flagged = true;
        }
    }

    // ---- Update history --------------------------------------------------
    // We always feed the rolling buffer, even on flagged samples -- snow
    // genuinely does change in real life and we want to adapt over time.
    _buf[_idx] = v;
    _idx = (_idx + 1) % OUTLIER_WINDOW;
    if (_filled < OUTLIER_WINDOW) _filled++;
    _lastVal     = v;
    _lastFlagged = flagged;
    if (flagged) _totalFlagged++;
    return flagged;
}
