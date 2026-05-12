// ============================================================================
//  Imu.cpp  --  implementation
// ============================================================================
#include "Imu.h"
#include "Config.h"
#include "AppSettings.h"
#include "SDLogger.h"
#include "Gps.h"
#include "TimeKeeper.h"

#include <Wire.h>
#include <BMI160Gen.h>
#include <math.h>

Imu imu;

// ----------------------------------------------------------------------------
const char* Imu::eventName(EventType t) {
    switch (t) {
        case EventType::Fall:   return "FALL";
        case EventType::Impact: return "IMPACT";
        default:                return "";
    }
}

// ----------------------------------------------------------------------------
void Imu::begin() {
    Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN, IMU_I2C_FREQ_HZ);

    if (!BMI160.begin(BMI160GenClass::I2C_MODE, IMU_I2C_ADDR)) {
        _ready = false;
        Serial.println();
        Serial.println(F("[ IMU   ] ============================================"));
        Serial.println(F("[ IMU   ]   !! BMI160 NOT DETECTED !!"));
        Serial.println(F("[ IMU   ] --------------------------------------------"));
        Serial.println(F("[ IMU   ] Wiring checklist:"));
        Serial.printf ( "[ IMU   ]   - VIN (or 3V3) -> 3.3V (NOT 5V)\n");
        Serial.printf ( "[ IMU   ]   - GND -> GND\n");
        Serial.printf ( "[ IMU   ]   - SDA -> GPIO%d\n", IMU_SDA_PIN);
        Serial.printf ( "[ IMU   ]   - SCL -> GPIO%d\n", IMU_SCL_PIN);
        Serial.printf ( "[ IMU   ]   - SAO -> 3.3V  (for addr 0x%02X)\n", IMU_I2C_ADDR);
        Serial.printf ( "[ IMU   ]   - CS  -> 3.3V  (forces I2C mode)\n");
        Serial.println(F("[ IMU   ] ============================================"));
        return;
    }

    BMI160.setAccelerometerRange(BMI160_ACCEL_RANGE_2G);
    _ready = true;
    DBG("IMU", "BMI160 initialised  addr=0x%02X  range=+/-2g", IMU_I2C_ADDR);

    _ringHead = _ringFill = 0;
    _lastEventType = EventType::None;
}

// ----------------------------------------------------------------------------
void Imu::resetPeak() {
    _peakMag = _peakX = _peakY = _peakZ = 0;
    _peakMs  = 0;
}

// ----------------------------------------------------------------------------
//  Push a new event into the ring buffer (newest replaces oldest).
// ----------------------------------------------------------------------------
void Imu::_pushEvent(EventType t, float mag, float ax, float ay, float az,
                     uint32_t durationMs) {
    Event& e = _ring[_ringHead];
    e.type        = t;
    e.mag         = mag;
    e.ax          = ax;
    e.ay          = ay;
    e.az          = az;
    e.durationMs  = durationMs;
    e.bootMs      = millis();
    e.timestampUs = (uint64_t)e.bootMs * 1000ULL;
    _ringHead = (_ringHead + 1) % IMU_EVENT_RING_SIZE;
    if (_ringFill < IMU_EVENT_RING_SIZE) _ringFill++;
}

// ----------------------------------------------------------------------------
//  Returns a snapshot of recent events (caller-side reverses for newest-first
//  display).  We return a pointer to internal storage; caller must not retain
//  it after the next loop() iteration.
// ----------------------------------------------------------------------------
const Imu::Event* Imu::recentEvents(size_t& count) const {
    count = _ringFill;
    return _ring;
}

// ----------------------------------------------------------------------------
void Imu::loop() {
    if (!_ready) return;

    // BMI160Gen returns int (16-bit) raw values; convert to g using ±2g range.
    int rawX, rawY, rawZ;
    BMI160.readAccelerometer(rawX, rawY, rawZ);

    _ax  = (float)rawX / IMU_ACCEL_LSB_PER_G;
    _ay  = (float)rawY / IMU_ACCEL_LSB_PER_G;
    _az  = (float)rawZ / IMU_ACCEL_LSB_PER_G;
    _mag = sqrtf(_ax*_ax + _ay*_ay + _az*_az);
    _lastReadMs = millis();

    // Update peak
    if (_mag > _peakMag) {
        _peakMag = _mag;
        _peakX = _ax; _peakY = _ay; _peakZ = _az;
        _peakMs = _lastReadMs;
    }

    const uint32_t now = millis();
    const bool cooldownActive = (now - _lastAlertMs) < settings.alertCooldownMs();

    // ---- Free-fall detection (sustained low magnitude) -------------------
    if (_mag < settings.fallThreshG()) {
        if (!_inFreeFall) {
            _inFreeFall = true;
            _freeFallStartMs = now;
        }
        if ((now - _freeFallStartMs) >= settings.fallWindowMs() && !cooldownActive) {
            uint32_t dur = now - _freeFallStartMs;
            _fallCount++;
            _lastAlertMs = now;
            _lastEventType = EventType::Fall;
            _lastEventMs   = now;
            _pushEvent(EventType::Fall, _mag, _ax, _ay, _az, dur);
            sdLogger.logFallEvent("FALL", _mag, _ax, _ay, _az, dur);
            Serial.printf("[ IMU   ] !! FREE FALL detected: %.3fg over %lu ms\n",
                          _mag, (unsigned long)dur);
            _inFreeFall = false;     // re-arm
        }
    } else {
        _inFreeFall = false;
    }

    // ---- Impact detection (single high-magnitude sample) ----------------
    if (_mag > settings.impactThreshG() && !cooldownActive) {
        _impactCount++;
        _lastAlertMs = now;
        _lastEventType = EventType::Impact;
        _lastEventMs   = now;
        _pushEvent(EventType::Impact, _mag, _ax, _ay, _az, 0);
        sdLogger.logFallEvent("IMPACT", _mag, _ax, _ay, _az, 0);
        Serial.printf("[ IMU   ] !! IMPACT detected: %.2fg\n", _mag);
    }
}

// ----------------------------------------------------------------------------
void Imu::printStats() const {
    if (!_ready) {
        Serial.println(F("[ IMU   ] OFFLINE  (BMI160 not detected)"));
        return;
    }
    Serial.printf("[ IMU   ] ok     a=(%+.2f, %+.2f, %+.2f)g  mag=%.2fg  "
                  "peak=%.2fg  falls=%lu  impacts=%lu\n",
                  _ax, _ay, _az, _mag, _peakMag,
                  (unsigned long)_fallCount, (unsigned long)_impactCount);
}
