// ============================================================================
//  AppSettings.cpp  --  Preferences (NVS) implementation
// ============================================================================
#include "AppSettings.h"
#include "Config.h"
#include <Preferences.h>

AppSettings settings;

static Preferences prefs;

// ----------------------------------------------------------------------------
void AppSettings::begin() {
    if (!prefs.begin(NVS_NS_SETTINGS, /*readOnly=*/false)) {
        DBG("SET", "ERROR: could not open NVS namespace '%s'", NVS_NS_SETTINGS);
        return;
    }
    _tareCm     = prefs.getFloat ("tare_cm",    0.0f);
    _units      = (Units)    prefs.getUChar("units",     (uint8_t)Units::Metric);
    _theme      = (Theme)    prefs.getUChar("theme",     (uint8_t)Theme::Dark);
    _intervalS  =            prefs.getULong("interval_s", DEFAULT_SAMPLE_INTERVAL_S);
    _rotation   = (Rotation) prefs.getUChar("rotation",   DEFAULT_ROTATION);
    _rotateRows =            prefs.getULong("rotate_rows", DEFAULT_ROTATE_ROWS);
    _deepSleep  =            prefs.getBool ("deep_sleep",  DEFAULT_DEEP_SLEEP_ENABLED);
    _tempCorrection =        prefs.getBool ("temp_corr",   false);
    _fallThreshG     =       prefs.getFloat("fall_g",     DEFAULT_FALL_THRESH_G);
    _impactThreshG   =       prefs.getFloat("impact_g",   DEFAULT_IMPACT_THRESH_G);
    _fallWindowMs    =       prefs.getULong("fall_win",   DEFAULT_FALL_WINDOW_MS);
    _alertCooldownMs =       prefs.getULong("alert_cd",   DEFAULT_ALERT_COOLDOWN_MS);

    _loraEnabled    =        prefs.getBool ("lora_on",    false);
    _loraRegion     =        prefs.getUChar("lora_reg",   LORA_DEFAULT_REGION);
    _loraChannel    =        prefs.getUChar("lora_ch",    LORA_DEFAULT_CHANNEL_EU);
    _loraTxPower    =        prefs.getUChar("lora_pwr",   LORA_TX_POWER_DEFAULT);
    _loraEncrypt    =        prefs.getBool ("lora_enc",   LORA_DEFAULT_ENCRYPT);
    _loraKey        =        prefs.getUShort("lora_key",  LORA_DEFAULT_KEY);
    _loraTxTrigger  =        prefs.getUChar("lora_trg",   LORA_DEFAULT_TX_TRIGGER);
    _loraTxPeriodS  =        prefs.getULong("lora_per",   LORA_DEFAULT_TX_PERIOD_S);

    // Clamp threshold values in case stale NVS state holds garbage
    if (_fallThreshG     < MIN_FALL_THRESH_G   || _fallThreshG     > MAX_FALL_THRESH_G)     _fallThreshG     = DEFAULT_FALL_THRESH_G;
    if (_impactThreshG   < MIN_IMPACT_THRESH_G || _impactThreshG   > MAX_IMPACT_THRESH_G)   _impactThreshG   = DEFAULT_IMPACT_THRESH_G;
    if (_fallWindowMs    < MIN_FALL_WINDOW_MS  || _fallWindowMs    > MAX_FALL_WINDOW_MS)    _fallWindowMs    = DEFAULT_FALL_WINDOW_MS;
    if (_alertCooldownMs < MIN_ALERT_COOLDOWN_MS || _alertCooldownMs > MAX_ALERT_COOLDOWN_MS) _alertCooldownMs = DEFAULT_ALERT_COOLDOWN_MS;
    if (_loraChannel > LORA_CHANNEL_MAX) _loraChannel =
        (_loraRegion == LORA_REGION_US) ? LORA_DEFAULT_CHANNEL_US : LORA_DEFAULT_CHANNEL_EU;
    if (_loraTxPower > 3) _loraTxPower = LORA_TX_POWER_DEFAULT;
    if (_loraTxTrigger > 1) _loraTxTrigger = LORA_DEFAULT_TX_TRIGGER;
    if (_loraTxPeriodS < 5)         _loraTxPeriodS = 5;
    if (_loraTxPeriodS > 86400)     _loraTxPeriodS = 86400;

    if (_intervalS < MIN_SAMPLE_INTERVAL_S || _intervalS > MAX_SAMPLE_INTERVAL_S) {
        _intervalS = DEFAULT_SAMPLE_INTERVAL_S;
    }
    if (_rotateRows < 100) _rotateRows = DEFAULT_ROTATE_ROWS;
    prefs.end();

    DBG("SET",
        "Loaded: tare=%.1fcm units=%d theme=%d interval=%lus rot=%d rows=%lu sleep=%d",
        _tareCm, (int)_units, (int)_theme,
        (unsigned long)_intervalS, (int)_rotation,
        (unsigned long)_rotateRows, (int)_deepSleep);
}

// ----------------------------------------------------------------------------
void AppSettings::resetToDefaults() {
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.clear();
        prefs.end();
    }
    _tareCm     = 0.0f;
    _units      = Units::Metric;
    _theme      = Theme::Dark;
    _intervalS  = DEFAULT_SAMPLE_INTERVAL_S;
    _rotation   = (Rotation)DEFAULT_ROTATION;
    _rotateRows = DEFAULT_ROTATE_ROWS;
    _deepSleep  = DEFAULT_DEEP_SLEEP_ENABLED;
    _tempCorrection = false;
    _fallThreshG     = DEFAULT_FALL_THRESH_G;
    _impactThreshG   = DEFAULT_IMPACT_THRESH_G;
    _fallWindowMs    = DEFAULT_FALL_WINDOW_MS;
    _alertCooldownMs = DEFAULT_ALERT_COOLDOWN_MS;
    _loraEnabled    = false;
    _loraRegion     = LORA_DEFAULT_REGION;
    _loraChannel    = LORA_DEFAULT_CHANNEL_EU;
    _loraTxPower    = LORA_TX_POWER_DEFAULT;
    _loraEncrypt    = LORA_DEFAULT_ENCRYPT;
    _loraKey        = LORA_DEFAULT_KEY;
    _loraTxTrigger  = LORA_DEFAULT_TX_TRIGGER;
    _loraTxPeriodS  = LORA_DEFAULT_TX_PERIOD_S;
}

// ----------------------------------------------------------------------------
void AppSettings::setTareCm(float cm) {
    if (cm < 0) cm = 0;
    _tareCm = cm;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putFloat("tare_cm", cm);
        prefs.end();
    }
    DBG("SET", "Tare saved: %.1f cm", cm);
}

// ----------------------------------------------------------------------------
void AppSettings::setUnits(Units u) {
    _units = u;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUChar("units", (uint8_t)u);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setTheme(Theme t) {
    _theme = t;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUChar("theme", (uint8_t)t);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setIntervalS(uint32_t s) {
    if (s < MIN_SAMPLE_INTERVAL_S) s = MIN_SAMPLE_INTERVAL_S;
    if (s > MAX_SAMPLE_INTERVAL_S) s = MAX_SAMPLE_INTERVAL_S;
    _intervalS = s;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putULong("interval_s", s);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setRotation(Rotation r) {
    _rotation = r;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUChar("rotation", (uint8_t)r);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setRotateRows(uint32_t n) {
    if (n < 100) n = 100;
    _rotateRows = n;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putULong("rotate_rows", n);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setDeepSleepEnabled(bool on) {
    _deepSleep = on;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putBool("deep_sleep", on);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setTempCorrectionEnabled(bool on) {
    _tempCorrection = on;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putBool("temp_corr", on);
        prefs.end();
    }
    DBG("SET", "Temperature correction: %s", on ? "ON" : "OFF");
}

// ----------------------------------------------------------------------------
void AppSettings::setFallThreshG(float g) {
    if (g < MIN_FALL_THRESH_G) g = MIN_FALL_THRESH_G;
    if (g > MAX_FALL_THRESH_G) g = MAX_FALL_THRESH_G;
    _fallThreshG = g;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putFloat("fall_g", g);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setImpactThreshG(float g) {
    if (g < MIN_IMPACT_THRESH_G) g = MIN_IMPACT_THRESH_G;
    if (g > MAX_IMPACT_THRESH_G) g = MAX_IMPACT_THRESH_G;
    _impactThreshG = g;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putFloat("impact_g", g);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setFallWindowMs(uint32_t ms) {
    if (ms < MIN_FALL_WINDOW_MS) ms = MIN_FALL_WINDOW_MS;
    if (ms > MAX_FALL_WINDOW_MS) ms = MAX_FALL_WINDOW_MS;
    _fallWindowMs = ms;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putULong("fall_win", ms);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
void AppSettings::setAlertCooldownMs(uint32_t ms) {
    if (ms < MIN_ALERT_COOLDOWN_MS) ms = MIN_ALERT_COOLDOWN_MS;
    if (ms > MAX_ALERT_COOLDOWN_MS) ms = MAX_ALERT_COOLDOWN_MS;
    _alertCooldownMs = ms;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putULong("alert_cd", ms);
        prefs.end();
    }
}

// ----------------------------------------------------------------------------
//  LoRa settings -- each setter clamps + persists to NVS atomically.
// ----------------------------------------------------------------------------
void AppSettings::setLoraEnabled(bool on) {
    _loraEnabled = on;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putBool("lora_on", on);
        prefs.end();
    }
}
void AppSettings::setLoraRegion(uint8_t r) {
    if (r > 1) r = LORA_DEFAULT_REGION;
    _loraRegion = r;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUChar("lora_reg", r);
        prefs.end();
    }
}
void AppSettings::setLoraChannel(uint8_t ch) {
    if (ch > LORA_CHANNEL_MAX) ch = LORA_CHANNEL_MAX;
    _loraChannel = ch;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUChar("lora_ch", ch);
        prefs.end();
    }
}
void AppSettings::setLoraTxPower(uint8_t p) {
    if (p > 3) p = LORA_TX_POWER_DEFAULT;
    _loraTxPower = p;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUChar("lora_pwr", p);
        prefs.end();
    }
}
void AppSettings::setLoraEncrypt(bool on) {
    _loraEncrypt = on;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putBool("lora_enc", on);
        prefs.end();
    }
}
void AppSettings::setLoraKey(uint16_t k) {
    _loraKey = k;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUShort("lora_key", k);
        prefs.end();
    }
}
void AppSettings::setLoraTxTrigger(uint8_t t) {
    if (t > 1) t = LORA_DEFAULT_TX_TRIGGER;
    _loraTxTrigger = t;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putUChar("lora_trg", t);
        prefs.end();
    }
}
void AppSettings::setLoraTxPeriodS(uint32_t s) {
    if (s < 5)     s = 5;
    if (s > 86400) s = 86400;
    _loraTxPeriodS = s;
    if (prefs.begin(NVS_NS_SETTINGS, false)) {
        prefs.putULong("lora_per", s);
        prefs.end();
    }
}
