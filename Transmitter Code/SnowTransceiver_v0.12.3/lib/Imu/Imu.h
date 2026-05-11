// ============================================================================
//  Imu.h  --  BMI160 6-DoF accelerometer driver + fall/impact detection
//
//  Hardware:
//      VIN -> 3.3 V        (NOT 5 V; some boards regulate down internally
//                          but the I2C lines are still 3.3 V logic level)
//      GND -> GND
//      SDA -> GPIO21       (default ESP32 I2C SDA)
//      SCL -> GPIO22       (default ESP32 I2C SCL)
//      SAO -> 3.3 V        (selects I2C address 0x69)
//      CS  -> 3.3 V        (forces I2C mode)
//      INT1, INT2, SCX, SDX, OCS  --  not connected
//
//  Detection logic (matches the user's tested sketch):
//      Free-fall  : |a| < fallThreshG  for at least fallWindowMs
//      Impact     : |a| > impactThreshG (single sample)
//      Cooldown   : alertCooldownMs between ANY two alerts (fall or impact)
//
//  Each fall or impact triggers two side effects:
//      1. logFallEvent() in SDLogger  (appends to /falllog.csv)
//      2. lastEventType / lastEventMs (consumed by main.cpp to set the
//         "event" column of the very next snowlog row)
// ============================================================================
#pragma once

#include <Arduino.h>
#include "Config.h"

class Imu {
public:
    enum class EventType : uint8_t { None = 0, Fall = 1, Impact = 2 };

    struct Event {
        EventType type;
        float     mag;
        float     ax, ay, az;
        uint32_t  durationMs;       // 0 for impact, >0 for fall
        uint64_t  timestampUs;      // device millis() * 1000 at event
        uint32_t  bootMs;           // raw millis() for sequencing
    };

    void  begin();
    void  loop();                          // call frequently (every loop iter)
    void  printStats() const;

    bool  isReady() const                  { return _ready; }

    // Live values
    float ax() const                       { return _ax; }
    float ay() const                       { return _ay; }
    float az() const                       { return _az; }
    float magnitude() const                { return _mag; }

    // Peak since boot
    float peakMag()  const                 { return _peakMag; }
    float peakAx()   const                 { return _peakX; }
    float peakAy()   const                 { return _peakY; }
    float peakAz()   const                 { return _peakZ; }
    uint32_t peakBootMs() const            { return _peakMs; }
    void  resetPeak();

    // Event counters
    uint32_t fallCount()   const           { return _fallCount; }
    uint32_t impactCount() const           { return _impactCount; }

    // Most recent event -- consumed by main.cpp to mark the next CSV row.
    // After the row is written, main.cpp calls clearLastEvent().
    EventType lastEventType()  const       { return _lastEventType; }
    uint32_t  lastEventBootMs() const      { return _lastEventMs; }
    void      clearLastEvent()             { _lastEventType = EventType::None; }
    static const char* eventName(EventType t);

    // Snapshot of last event details (for /api/imu/events live ring)
    const Event* recentEvents(size_t& count) const;

private:
    bool     _ready    = false;
    uint32_t _lastReadMs = 0;
    float    _ax = 0, _ay = 0, _az = 0, _mag = 0;
    float    _peakMag = 0, _peakX = 0, _peakY = 0, _peakZ = 0;
    uint32_t _peakMs  = 0;
    uint32_t _fallCount    = 0;
    uint32_t _impactCount  = 0;

    // Detection state
    bool     _inFreeFall      = false;
    uint32_t _freeFallStartMs = 0;
    uint32_t _lastAlertMs     = 0;

    // Last-event "mailbox" for the snowlog row stamper
    EventType _lastEventType = EventType::None;
    uint32_t  _lastEventMs   = 0;

    // In-memory ring of recent events (newest at high indices)
    Event    _ring[IMU_EVENT_RING_SIZE];
    size_t   _ringHead = 0;
    size_t   _ringFill = 0;

    void  _pushEvent(EventType t, float mag, float ax, float ay, float az,
                     uint32_t durationMs);
};

extern Imu imu;
