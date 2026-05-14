// ============================================================================
//  Battery.cpp
// ============================================================================
#include "Battery.h"
#include "Config.h"
#include <esp_adc_cal.h>

Battery battery;

// ----------------------------------------------------------------------------
void Battery::begin() {
    // ADC1 channel 7 (GPIO35) is selected automatically by analogRead().
    // 12-bit width keeps the math simple; default attenuation 11dB gives a
    // ~0..3.1V input range, plenty for our 3.3V max.
    analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC_PIN, ADC_11db);
    pinMode(BATTERY_ADC_PIN, INPUT);

    DBG("BAT", "monitor on GPIO%d (uPesy on-board divider, ratio=%.3f)",
        BATTERY_ADC_PIN, BATTERY_DIVIDER_RATIO);

    // Take one reading so the dashboard isn't blank on first poll
    _voltage = _readOnce();
    _onUsb   = _voltage > BATTERY_USB_GUARD_V;
    _samples = 1;
    _lastReadMs = millis();
}

// ----------------------------------------------------------------------------
//  Multi-sample average to fight ADC noise.  ESP32 ADC is famously noisy
//  even at full 12-bit resolution; 32 samples averaged gets us within a
//  few mV which is plenty for SoC display.
// ----------------------------------------------------------------------------
float Battery::_readOnce() const {
    uint32_t acc = 0;
    for (int i = 0; i < BATTERY_SAMPLES; i++) {
        acc += analogRead(BATTERY_ADC_PIN);
        delayMicroseconds(50);          // let the S/H cap settle
    }
    float raw = (float)acc / (float)BATTERY_SAMPLES;
    return BATTERY_DIVIDER_RATIO * (raw / BATTERY_ADC_RESOLUTION) * BATTERY_VREF;
}

// ----------------------------------------------------------------------------
void Battery::loop() {
    if (millis() - _lastReadMs < BATTERY_READ_INTERVAL_MS) return;
    _lastReadMs = millis();

    float v = _readOnce();
    // Light low-pass filter on top of the per-call averaging
    if (isnan(_voltage)) {
        _voltage = v;
    } else {
        _voltage = 0.85f * _voltage + 0.15f * v;
    }
    _rawAvg  = _voltage;
    _onUsb   = _voltage > BATTERY_USB_GUARD_V;
    _samples++;

    // Push to rolling history once per BATTERY_HISTORY_INTERVAL_MS.  We
    // only record samples when running off the battery; USB-connected
    // samples would corrupt the discharge rate.
    if (!_onUsb && (_lastHistMs == 0 ||
        millis() - _lastHistMs >= BATTERY_HISTORY_INTERVAL_MS)) {
        _lastHistMs = millis();
        _pushHistory(percent());
    }
    // If we've just been plugged in, clear the history so the regression
    // doesn't mix charge/discharge phases when we unplug again.
    if (_onUsb && _histFill > 0) {
        _histFill = _histHead = 0;
        _lastHistMs = 0;
    }
}

// ----------------------------------------------------------------------------
void Battery::_pushHistory(float pct) {
    _history[_histHead] = { millis(), pct };
    _histHead = (_histHead + 1) % BATTERY_HISTORY_SIZE;
    if (_histFill < BATTERY_HISTORY_SIZE) _histFill++;
}

// ----------------------------------------------------------------------------
//  Linear regression (least-squares) of percent vs time over the rolling
//  history.  Returns slope in %/hour.  NaN if not enough samples yet.
// ----------------------------------------------------------------------------
float Battery::dischargeRatePctPerHour() const {
    if (_histFill < BATTERY_HISTORY_MIN_SAMPLES) return NAN;

    // Sum_x and sum_y in the rolling buffer.  Use minutes as the x-axis
    // to keep numbers small, then convert slope to %/hour at the end.
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    int n = _histFill;

    // The samples in _history are ordered from oldest (at _histHead..)
    // to newest (just before _histHead).  We don't actually need to walk
    // them in order for least squares -- the formula doesn't care.
    for (int i = 0; i < n; i++) {
        double tMin = (double)_history[i].bootMs / 60000.0;
        double y    = (double)_history[i].pct;
        sx  += tMin;
        sy  += y;
        sxx += tMin * tMin;
        sxy += tMin * y;
    }
    double denom = (n * sxx) - (sx * sx);
    if (fabs(denom) < 1e-9) return NAN;
    double slopePerMin = ((n * sxy) - (sx * sy)) / denom;
    return (float)(slopePerMin * 60.0);    // -> %/hour
}

// ----------------------------------------------------------------------------
float Battery::hoursRemaining() const {
    if (_onUsb) return NAN;
    float rate = dischargeRatePctPerHour();
    if (isnan(rate) || rate >= -0.05f) return NAN;       // not discharging meaningfully
    float p = percent();
    if (p <= 0.5f) return 0.0f;
    return p / (-rate);
}

// ----------------------------------------------------------------------------
uint32_t Battery::historyMinutesAvailable() const {
    if (_histFill < 2) return 0;
    // Span between oldest and newest sample
    int oldestIdx = _histHead - _histFill;
    if (oldestIdx < 0) oldestIdx += BATTERY_HISTORY_SIZE;
    int newestIdx = (_histHead + BATTERY_HISTORY_SIZE - 1) % BATTERY_HISTORY_SIZE;
    uint32_t span = _history[newestIdx].bootMs - _history[oldestIdx].bootMs;
    return span / 60000UL;
}

// ----------------------------------------------------------------------------
//  Linear SoC% between EMPTY and FULL.  Real Li-Po curves are non-linear
//  (the discharge curve is mostly flat between 3.7-4.0V then steep below
//  3.6V), but for a UI bar a linear approximation is fine and avoids
//  surprising the user with a "stuck at 80%" plateau.
// ----------------------------------------------------------------------------
float Battery::percent() const {
    if (isnan(_voltage)) return 0.0f;
    if (_onUsb)          return 100.0f;        // USB plugged -> "powered"
    float p = (_voltage - BATTERY_EMPTY_V)
            / (BATTERY_FULL_V - BATTERY_EMPTY_V) * 100.0f;
    if (p < 0.0f)   p = 0.0f;
    if (p > 100.0f) p = 100.0f;
    return p;
}

// ----------------------------------------------------------------------------
void Battery::printStats() const {
    if (!ready()) {
        Serial.println(F("[ BAT   ] not yet read"));
        return;
    }
    if (_onUsb) {
        Serial.printf("[ BAT   ] USB    raw=%.2fV  (board powered from USB-C, "
                      "battery voltage unreadable)\n", _voltage);
        return;
    }

    float rate = dischargeRatePctPerHour();
    float hrs  = hoursRemaining();
    if (isnan(rate)) {
        Serial.printf("[ BAT   ] %-5s  %.2fV  (%.0f%%)  hist=%lumin (warming up)\n",
                      isLow() ? "LOW" : "ok",
                      _voltage, percent(),
                      (unsigned long)historyMinutesAvailable());
    } else if (isnan(hrs)) {
        Serial.printf("[ BAT   ] %-5s  %.2fV  (%.0f%%)  rate=%+.2f%%/h  (charging or stable)\n",
                      isLow() ? "LOW" : "ok",
                      _voltage, percent(), rate);
    } else {
        Serial.printf("[ BAT   ] %-5s  %.2fV  (%.0f%%)  rate=%+.2f%%/h  ETA=%.1fh%s\n",
                      isLow() ? "LOW" : "ok",
                      _voltage, percent(), rate, hrs,
                      isLow() ? "  !! consider charging" : "");
    }
}
