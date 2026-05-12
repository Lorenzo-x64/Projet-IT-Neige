// ============================================================================
//  main.cpp  --  Snow Transceiver firmware entrypoint
//
//  Adding a new subsystem (e.g. LoRa, BLE) means creating a new lib/MyThing
//  folder and registering it here.  This file stays small on purpose.
// ============================================================================

#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <esp_sleep.h>

#include "Config.h"
#include "Ultrasonic.h"
#include "SDLogger.h"
#include "AppSettings.h"
#include "SnowEngine.h"
#include "TimeKeeper.h"
#include "Gps.h"
#include "OutlierDetector.h"
#include "Imu.h"
#include "Battery.h"
#include "AhtSensor.h"
#include "LoRaTx.h"
#include "WebUI.h"

// ----------------------------------------------------------------------------
//  Captive portal -- redirects any URL the browser tries to our dashboard.
// ----------------------------------------------------------------------------
static DNSServer dnsServer;

// ----------------------------------------------------------------------------
static void setupWiFiAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(WIFI_AP_IP, WIFI_AP_GATEWAY, WIFI_AP_SUBNET);
    bool ok = WiFi.softAP(WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_CHANNEL);
    if (!ok) {
        DBG("WIFI", "softAP() FAILED");
        return;
    }
    DBG("WIFI", "AP \"%s\" up at %s", WIFI_AP_SSID,
        WiFi.softAPIP().toString().c_str());

    dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
    dnsServer.start(DNS_PORT, "*", WIFI_AP_IP);
}

// ----------------------------------------------------------------------------
static void setupFilesystem() {
    if (!LittleFS.begin(/*formatOnFail=*/false)) {
        DBG("FS", "mount failed -- attempting format");
        if (!LittleFS.begin(true)) {
            DBG("FS", "format also failed; UI will not load");
            return;
        }
    }
    DBG("FS", "LittleFS mounted (total=%u used=%u)",
        (unsigned)LittleFS.totalBytes(), (unsigned)LittleFS.usedBytes());
}

// ----------------------------------------------------------------------------
//  Sample + log one row.  Returns true if a row was actually written.
// ----------------------------------------------------------------------------
static bool sampleAndLog() {
    if (!ultrasonic.isFresh()) return false;
    if (!sdLogger.isReady())   return false;

    float dist  = ultrasonic.distanceCm();
    float depth = snow.snowDepthCm();
    bool  flag  = outlier.evaluate(dist);

    // If a fall/impact happened since the last sample, stamp this row with
    // the event name and clear the IMU mailbox so the next sample is clean.
    Imu::EventType pendingEvent = imu.lastEventType();
    const char* event = Imu::eventName(pendingEvent);
    bool wrote = sdLogger.logRow(dist, depth, flag, event,
                                 gps.latitude(), gps.longitude(),
                                 gps.altitudeM(), gps.speedKmh(),
                                 gps.satellites(), gps.hdop(),
                                 aht.isFresh() ? aht.tempC()    : NAN,
                                 aht.isFresh() ? aht.humidity() : NAN);
    if (!wrote) return false;

    // ----- Mirror the row over LoRa (when enabled + trigger=follow) -----
    if (settings.loraEnabled() &&
        settings.loraTxTrigger() == LORA_TX_TRIGGER_FOLLOW &&
        loraTx.isReady()) {
        loraTx.sendCsvRow(sdLogger.lastRow());
    }

    // ----- Fire alert frames for the event that's tagged on this row ---
    if (settings.loraEnabled() && loraTx.isReady()) {
        if (pendingEvent == Imu::EventType::Fall) {
            // We don't have the original duration here; use 0.  The receiver
            // already has the full row above with timestamp + magnitude in
            // /falllog.csv format if it cares about the detail.
            loraTx.sendAlertFall(imu.magnitude(), 0, timekeeper.iso8601());
        } else if (pendingEvent == Imu::EventType::Impact) {
            loraTx.sendAlertImpact(imu.magnitude(), timekeeper.iso8601());
        }
    }
    if (pendingEvent != Imu::EventType::None) {
        imu.clearLastEvent();
    }
    return wrote;
}

// ----------------------------------------------------------------------------
//  Periodic CSV logging tick (when deep sleep is OFF)
// ----------------------------------------------------------------------------
static uint32_t lastLogMs = 0;
static void logTick() {
    uint32_t intervalMs = settings.intervalS() * 1000UL;
    if (millis() - lastLogMs < intervalMs) return;
    lastLogMs = millis();
    sampleAndLog();
}

// ----------------------------------------------------------------------------
//  Deep sleep tick.  Active only when:
//      - settings.deepSleepEnabled() == true
//      - settings.intervalS() >= DEEP_SLEEP_MIN_INTERVAL_S
//
//  Strategy: stay awake DEEP_SLEEP_AWAKE_WINDOW_MS after each sample so the
//  user can connect to the dashboard, then sleep until the next sample.
// ----------------------------------------------------------------------------
static uint32_t bootMs    = 0;     // millis() at top of loop after boot/wake
static bool     sampledThisCycle = false;

static void deepSleepTick() {
    if (!settings.deepSleepEnabled())                   return;
    if (settings.intervalS() < DEEP_SLEEP_MIN_INTERVAL_S) return;
    if (!sdLogger.isReady())                            return;

    // Take exactly one sample at the start of the awake window, then idle
    // until the awake window expires.
    if (!sampledThisCycle) {
        if (ultrasonic.isFresh()) {
            sampleAndLog();
            sampledThisCycle = true;
        }
        return;     // sample not ready yet, keep waiting
    }

    if (millis() - bootMs < DEEP_SLEEP_AWAKE_WINDOW_MS) return;

    // Sleep for the rest of the interval (subtract the awake window).
    uint64_t sleepUs =
        (uint64_t)(settings.intervalS() * 1000UL - DEEP_SLEEP_AWAKE_WINDOW_MS) * 1000ULL;
    DBG("PWR", "deep sleeping for %.1f s ...",
        sleepUs / 1000000.0);
    Serial.flush();
    esp_sleep_enable_timer_wakeup(sleepUs);
    esp_deep_sleep_start();
    // (Never returns -- ESP32 reboots on wake)
}

// ----------------------------------------------------------------------------
//  Periodic console diagnostics (visible via `pio device monitor`)
// ----------------------------------------------------------------------------
static uint32_t lastDiagMs = 0;

static String fmtUptime(uint32_t s) {
    uint32_t d = s / 86400; s -= d * 86400;
    uint32_t h = s / 3600;  s -= h * 3600;
    uint32_t m = s / 60;    s -= m * 60;
    char buf[32];
    snprintf(buf, sizeof(buf), "%lud %luh %lum %lus",
             (unsigned long)d, (unsigned long)h, (unsigned long)m, (unsigned long)s);
    return String(buf);
}

// ============================================================================
//  Alert + LoRa-tick logic
// ============================================================================
//
// Sensor-disconnect: any sensor that was previously fresh and stops
//                    producing data for > SENSOR_STALE_MS triggers a
//                    one-shot LOST alert.  Each sensor has a re-arm
//                    flag so we don't spam alerts on flapping links.
//
// SD-full: when sdLogger.usagePercent() rising-edges past 90% we fire
//          one SDFULL alert.  Re-armed when usage drops back below 85%.
//
// Manual-period TX: when LoRa trigger == MANUAL, send the latest
//                   cached CSV row every loraTxPeriodS() seconds
//                   (regardless of sample interval).
// ============================================================================
#define SENSOR_STALE_MS    30000UL    // 30 seconds without a fresh reading
#define SD_FULL_REARM_PCT  85.0f      // hysteresis for the SD alert

struct SensorWatch {
    const char* name;
    bool        wasFresh;       // previous loop's freshness
    bool        alertFired;     // suppress repeated alerts
    uint32_t    lastFreshMs;    // for tracking stale duration
};

static SensorWatch sensorWatches[] = {
    { "ULTRASONIC", false, false, 0 },
    { "GPS",        false, false, 0 },
    { "IMU",        false, false, 0 },
    { "BATTERY",    false, false, 0 },
};

static bool _isSensorFresh(int idx) {
    switch (idx) {
        case 0: return ultrasonic.isFresh();
        case 1: return gps.hasFix();
        case 2: return imu.isReady();
        case 3: return battery.ready();
    }
    return false;
}

static void sensorWatchTick() {
    if (!settings.loraEnabled() || !loraTx.isReady()) return;

    for (size_t i = 0; i < sizeof(sensorWatches) / sizeof(sensorWatches[0]); i++) {
        SensorWatch& w = sensorWatches[i];
        bool fresh = _isSensorFresh(i);

        if (fresh) {
            w.wasFresh    = true;
            w.lastFreshMs = millis();
            // Re-arm the alert: if the sensor comes back, allow a future
            // alert when it goes stale again.
            w.alertFired = false;
        } else if (w.wasFresh && !w.alertFired) {
            // Was fresh, now isn't.  Only fire if it's been stale long
            // enough to be a real disconnect (not just a one-poll blip).
            if (millis() - w.lastFreshMs > SENSOR_STALE_MS) {
                if (loraTx.sendAlertSensorLost(w.name, timekeeper.iso8601())) {
                    w.alertFired = true;
                    Serial.printf("[ ALERT ] sensor lost: %s (alert sent)\n", w.name);
                }
            }
        }
    }
}

static void sdFullTick() {
    static bool armed = true;          // true = next crossing will fire
    if (!settings.loraEnabled() || !loraTx.isReady()) return;
    if (!sdLogger.isReady())            return;

    uint64_t total = sdLogger.totalBytes();
    if (total == 0) return;
    float pct = (float)sdLogger.usedBytes() * 100.0f / (float)total;

    if (armed && pct >= SD_FULL_THRESHOLD_PCT) {
        if (loraTx.sendAlertSdFull(pct, timekeeper.iso8601())) {
            armed = false;
            Serial.printf("[ ALERT ] SD %.1f%% used (alert sent)\n", pct);
        }
    } else if (!armed && pct < SD_FULL_REARM_PCT) {
        armed = true;
        Serial.printf("[ ALERT ] SD usage back down to %.1f%% (alert re-armed)\n", pct);
    }
}

static void manualTxTick() {
    static uint32_t lastManualTxMs = 0;
    if (!settings.loraEnabled() || !loraTx.isReady())          return;
    if (settings.loraTxTrigger() != LORA_TX_TRIGGER_MANUAL)    return;
    if (sdLogger.lastRow().length() == 0)                      return;

    uint32_t periodMs = (uint32_t)settings.loraTxPeriodS() * 1000UL;
    if (lastManualTxMs != 0 && (millis() - lastManualTxMs) < periodMs) return;

    if (loraTx.sendCsvRow(sdLogger.lastRow())) {
        lastManualTxMs = millis();
    }
}

static void diagTick() {


    if (millis() - lastDiagMs < 5000) return;
    lastDiagMs = millis();

    Serial.println();
    Serial.printf("=== status @ uptime %s · heap %.1fKB · clients %u ===\n",
                  fmtUptime(millis() / 1000).c_str(),
                  ESP.getFreeHeap() / 1024.0f,
                  WiFi.softAPgetStationNum());
    ultrasonic.printStats();
    Serial.printf("[ SNOW  ] depth=%.1fcm  tare=%.1fcm  interval=%lus  outliers=%lu\n",
                  snow.snowDepthCm(), snow.tareCm(),
                  (unsigned long)settings.intervalS(),
                  (unsigned long)outlier.totalFlagged());
    gps.printStats();
    imu.printStats();
    aht.printStats();
    battery.printStats();
    sdLogger.printStats();
    loraTx.printStats();
    Serial.printf("[ TIME  ] %s  (source=%s)\n",
                  timekeeper.iso8601().c_str(),
                  timekeeper.source() == TimeKeeper::Source::Ntp    ? "ntp"    :
                  timekeeper.source() == TimeKeeper::Source::Manual ? "manual" :
                                                                      "boot");
    if (settings.deepSleepEnabled() && settings.intervalS() >= DEEP_SLEEP_MIN_INTERVAL_S) {
        Serial.println(F("[ PWR   ] deep-sleep mode ENABLED"));
    }
    Serial.println(F("==="));
}

// ============================================================================
//  setup()
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(150);
    Serial.println();
    Serial.println(F("============================================================"));
    Serial.printf ("  %s  v%s\n", FW_NAME, FW_VERSION);
    Serial.printf ("  Build: %s\n", FW_BUILD_DATE);

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println(F("  Wake cause: deep-sleep TIMER"));
    } else {
        Serial.println(F("  Wake cause: power-on / reset"));
    }
    Serial.println(F("============================================================"));

    setupFilesystem();
    settings.begin();
    timekeeper.begin();
    snow.begin();
    outlier.begin();
    ultrasonic.begin();
    gps.begin();
    imu.begin();
    aht.begin();
    battery.begin();
    sdLogger.begin();
    loraTx.begin();
    setupWiFiAP();
    webui.begin();

    bootMs = millis();
    sampledThisCycle = false;

    Serial.println(F("[ READY ] Connect to the Wi-Fi AP and open http://192.168.4.1"));
}

// ============================================================================
//  loop()
// ============================================================================
void loop() {
    dnsServer.processNextRequest();
    ultrasonic.loop();
    gps.loop();
    imu.loop();
    aht.loop();
    battery.loop();
    sdLogger.loop();
    loraTx.loop();

    if (settings.deepSleepEnabled() &&
        settings.intervalS() >= DEEP_SLEEP_MIN_INTERVAL_S) {
        deepSleepTick();
    } else {
        logTick();
    }

    sensorWatchTick();
    sdFullTick();
    manualTxTick();
    diagTick();

    delay(2);   // yield to AsyncTCP/WiFi tasks
}
