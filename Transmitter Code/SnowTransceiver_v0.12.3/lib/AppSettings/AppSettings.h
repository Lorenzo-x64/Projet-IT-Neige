// ============================================================================
//  AppSettings.h  --  persistent settings stored in NVS (Preferences)
//
//  All settings here survive reboots.  Anything that should reset on power
//  cycle does NOT belong in this module.
//
//  Stored keys (in namespace NVS_NS_SETTINGS):
//      "tare_cm"      float   reference distance for snow depth
//      "units"        uint8   0 = metric, 1 = imperial
//      "theme"        uint8   0 = dark,   1 = light
//      "interval_s"   uint32  logging interval in seconds (1 .. 172800)
//      "rotation"     uint8   CSV rotation strategy (Rotation enum)
//      "rotate_rows"  uint32  row count threshold for Rows rotation
//      "deep_sleep"   bool    enable RTC-timer deep sleep between samples
//      "fall_g"       float   BMI160 free-fall threshold (g)
//      "impact_g"     float   BMI160 impact threshold (g)
//      "fall_win"     uint32  free-fall sustained-window duration (ms)
//      "alert_cd"     uint32  cooldown between alerts (ms)
// ============================================================================
#pragma once

// IMPORTANT: pull in Config.h here, not in the .cpp.  The default values
// (DEFAULT_FALL_THRESH_G, DEFAULT_ROTATION, ...) are used directly as
// member initialisers below, so the macros must be visible at the point
// the class declaration is parsed -- regardless of which .cpp includes us.
#include <Arduino.h>
#include "Config.h"

enum class Units    : uint8_t { Metric = 0, Imperial = 1 };
enum class Theme    : uint8_t { Dark   = 0, Light    = 1 };
enum class Rotation : uint8_t { None = 0, Daily = 1, Weekly = 2, Monthly = 3, Rows = 4 };

class AppSettings {
public:
    void   begin();                             // load from NVS
    void   resetToDefaults();

    // Tare value (in centimetres, always stored as cm regardless of UI units)
    float  tareCm() const                { return _tareCm; }
    void   setTareCm(float cm);                 // persists immediately

    // Units (display only — internal data is always SI)
    Units  units() const                 { return _units; }
    void   setUnits(Units u);

    // Theme (informational; the browser actually decides rendering)
    Theme  theme() const                 { return _theme; }
    void   setTheme(Theme t);

    // Logging interval, in seconds (1 .. 172800 = 2 days)
    uint32_t intervalS() const           { return _intervalS; }
    void   setIntervalS(uint32_t s);

    // CSV rotation
    Rotation rotation() const            { return _rotation; }
    void   setRotation(Rotation r);
    uint32_t rotateRows() const          { return _rotateRows; }
    void   setRotateRows(uint32_t n);

    // Deep sleep between samples
    bool   deepSleepEnabled() const      { return _deepSleep; }
    void   setDeepSleepEnabled(bool on);

    // IMU thresholds (BMI160 fall/impact detection)
    float    fallThreshG()    const      { return _fallThreshG; }
    void     setFallThreshG(float g);
    float    impactThreshG()  const      { return _impactThreshG; }
    void     setImpactThreshG(float g);
    uint32_t fallWindowMs()   const      { return _fallWindowMs; }
    void     setFallWindowMs(uint32_t ms);
    uint32_t alertCooldownMs() const     { return _alertCooldownMs; }
    void     setAlertCooldownMs(uint32_t ms);

    // LoRa transmitter
    bool     loraEnabled()    const      { return _loraEnabled; }
    void     setLoraEnabled(bool on);
    uint8_t  loraRegion()     const      { return _loraRegion; }
    void     setLoraRegion(uint8_t r);
    uint8_t  loraChannel()    const      { return _loraChannel; }
    void     setLoraChannel(uint8_t ch);
    uint8_t  loraTxPower()    const      { return _loraTxPower; }
    void     setLoraTxPower(uint8_t p);
    bool     loraEncrypt()    const      { return _loraEncrypt; }
    void     setLoraEncrypt(bool on);
    uint16_t loraKey()        const      { return _loraKey; }
    void     setLoraKey(uint16_t k);
    uint8_t  loraTxTrigger()  const      { return _loraTxTrigger; }
    void     setLoraTxTrigger(uint8_t t);
    uint32_t loraTxPeriodS()  const      { return _loraTxPeriodS; }
    void     setLoraTxPeriodS(uint32_t s);

private:
    float    _tareCm     = 0.0f;
    Units    _units      = Units::Metric;
    Theme    _theme      = Theme::Dark;
    uint32_t _intervalS  = 10;
    Rotation _rotation   = Rotation::None;
    uint32_t _rotateRows = 10000;
    bool     _deepSleep  = false;
    float    _fallThreshG     = DEFAULT_FALL_THRESH_G;
    float    _impactThreshG   = DEFAULT_IMPACT_THRESH_G;
    uint32_t _fallWindowMs    = DEFAULT_FALL_WINDOW_MS;
    uint32_t _alertCooldownMs = DEFAULT_ALERT_COOLDOWN_MS;
    bool     _loraEnabled  = false;
    uint8_t  _loraRegion   = LORA_DEFAULT_REGION;
    uint8_t  _loraChannel  = LORA_DEFAULT_CHANNEL_EU;
    uint8_t  _loraTxPower  = LORA_TX_POWER_DEFAULT;
    bool     _loraEncrypt  = LORA_DEFAULT_ENCRYPT;
    uint16_t _loraKey      = LORA_DEFAULT_KEY;
    uint8_t  _loraTxTrigger = LORA_DEFAULT_TX_TRIGGER;
    uint32_t _loraTxPeriodS = LORA_DEFAULT_TX_PERIOD_S;
};

extern AppSettings settings;
