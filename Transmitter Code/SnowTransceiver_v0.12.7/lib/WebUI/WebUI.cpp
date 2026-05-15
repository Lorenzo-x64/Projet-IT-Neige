// ============================================================================
//  WebUI.cpp  --  AsyncWebServer routes
// ============================================================================
#include "WebUI.h"
#include "Config.h"
#include "Ultrasonic.h"
#include "SDLogger.h"
#include "SnowEngine.h"
#include "AppSettings.h"
#include "TimeKeeper.h"
#include "Gps.h"
#include "OutlierDetector.h"
#include "Imu.h"
#include "Battery.h"
#include "AhtSensor.h"
#include "LoRaTx.h"

#include <WiFi.h>
#include <LittleFS.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <ArduinoJson.h>
#include <SD.h>

WebUI webui;

static AsyncWebServer server(WEB_PORT);

// ----------------------------------------------------------------------------
//  Helpers
// ----------------------------------------------------------------------------
static const char* sourceLabel(TimeKeeper::Source s) {
    switch (s) {
        case TimeKeeper::Source::Ntp:    return "ntp";
        case TimeKeeper::Source::Manual: return "manual";
        default:                         return "boot";
    }
}

static void addJson(JsonObject& o, const char* k, float v, int prec = 2) {
    if (isnan(v)) o[k] = nullptr;
    else          o[k] = (double) ((int)(v * powf(10, prec) + (v >= 0 ? 0.5f : -0.5f)) / powf(10, prec));
}

static void sendJson(AsyncWebServerRequest* req, JsonDocument& doc, int code = 200) {
    String out;
    serializeJson(doc, out);
    AsyncWebServerResponse* resp = req->beginResponse(code, "application/json", out);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
}

// ----------------------------------------------------------------------------
//  /api/status
// ----------------------------------------------------------------------------
static void handleStatus(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["fw_name"]        = FW_NAME;
    doc["fw_version"]     = FW_VERSION;
    doc["uptime_s"]       = millis() / 1000;
    doc["free_heap"]      = ESP.getFreeHeap();
    doc["ap_clients"]     = WiFi.softAPgetStationNum();

    JsonObject sensor     = doc["sensor"].to<JsonObject>();
    sensor["fresh"]       = ultrasonic.isFresh();
    addJson(sensor, "distance_cm", ultrasonic.distanceCm(), 1);
    sensor["last_ms_ago"] = ultrasonic.lastReadMs() == 0 ? -1
                              : (long)(millis() - ultrasonic.lastReadMs());
    sensor["frames"]      = ultrasonic.totalFrames();
    sensor["checksum_err"]= ultrasonic.checksumErrors();
    sensor["out_of_range"]= ultrasonic.outOfRangeReads();

    JsonObject snowObj    = doc["snow"].to<JsonObject>();
    addJson(snowObj, "depth_cm", snow.snowDepthCm(), 1);
    addJson(snowObj, "tare_cm",  snow.tareCm(), 1);
    snowObj["outlier"]    = outlier.lastFlagged();
    snowObj["outliers"]   = outlier.totalFlagged();

    JsonObject gpsObj     = doc["gps"].to<JsonObject>();
    gpsObj["fix"]         = gps.hasFix();
    gpsObj["fresh"]       = gps.isFresh();
    addJson(gpsObj, "lat",       gps.latitude(),  6);
    addJson(gpsObj, "lon",       gps.longitude(), 6);
    addJson(gpsObj, "alt_m",     gps.altitudeM(), 1);
    addJson(gpsObj, "speed_kmh", gps.speedKmh(),  1);
    addJson(gpsObj, "course",    gps.courseDeg(), 1);
    addJson(gpsObj, "hdop",      gps.hdop(),      1);
    gpsObj["sats"]        = gps.satellites();
    gpsObj["chars"]       = gps.charsProcessed();
    gpsObj["fix_sentences"]   = gps.sentencesWithFix();
    gpsObj["failed_checksum"] = gps.failedChecksum();

    JsonObject sd         = doc["sd"].to<JsonObject>();
    sd["ready"]           = sdLogger.isReady();
    sd["rows"]            = sdLogger.rowCount();
    sd["total_bytes"]     = (double)sdLogger.totalBytes();
    sd["used_bytes"]      = (double)sdLogger.usedBytes();
    sd["free_bytes"]      = (double)sdLogger.freeBytes();
    sd["nearly_full"]     = sdLogger.isNearlyFull();
    sd["active_file"]     = sdLogger.activeFile();

    JsonObject im         = doc["imu"].to<JsonObject>();
    im["ready"]           = imu.isReady();
    addJson(im, "ax",       imu.ax(),  3);
    addJson(im, "ay",       imu.ay(),  3);
    addJson(im, "az",       imu.az(),  3);
    addJson(im, "mag",      imu.magnitude(), 3);
    addJson(im, "peak_mag", imu.peakMag(),   3);
    addJson(im, "peak_ax",  imu.peakAx(),    3);
    addJson(im, "peak_ay",  imu.peakAy(),    3);
    addJson(im, "peak_az",  imu.peakAz(),    3);
    im["peak_age_ms"]     = imu.peakBootMs() ? (long)(millis() - imu.peakBootMs()) : -1;
    im["fall_count"]      = imu.fallCount();
    im["impact_count"]    = imu.impactCount();
    im["last_event"]      = Imu::eventName(imu.lastEventType());

    JsonObject bat        = doc["battery"].to<JsonObject>();
    bat["ready"]          = battery.ready();
    addJson(bat, "voltage", battery.voltage(),  2);
    bat["percent"]        = (int)(battery.percent() + 0.5f);
    bat["on_usb"]         = battery.onUsb();
    bat["low"]            = battery.isLow();
    addJson(bat, "rate_pct_per_hr", battery.dischargeRatePctPerHour(), 2);
    addJson(bat, "hours_remaining", battery.hoursRemaining(), 2);
    bat["history_min"]    = battery.historyMinutesAvailable();

    JsonObject env        = doc["env"].to<JsonObject>();
    env["ready"]          = aht.ready();
    env["fresh"]          = aht.isFresh();
    addJson(env, "temp_c",    aht.isFresh() ? aht.tempC()    : NAN, 2);
    addJson(env, "humidity",  aht.isFresh() ? aht.humidity() : NAN, 1);
    addJson(env, "temp_f",    aht.isFresh() ? aht.tempF()    : NAN, 1);

    JsonObject t          = doc["time"].to<JsonObject>();
    t["iso"]              = timekeeper.iso8601();
    t["source"]           = sourceLabel(timekeeper.source());

    JsonObject lo         = doc["lora"].to<JsonObject>();
    {
        const char* st = "?";
        switch (loraTx.state()) {
            case LoRaTx::State::Disabled:     st = "disabled";     break;
            case LoRaTx::State::InitFailed:   st = "init_failed";  break;
            case LoRaTx::State::Idle:         st = "idle";         break;
            case LoRaTx::State::Configuring:  st = "configuring";  break;
            case LoRaTx::State::Transmitting: st = "transmitting"; break;
        }
        lo["state"]            = st;
        lo["enabled"]          = settings.loraEnabled();
        lo["region"]           = (int)settings.loraRegion();
        lo["channel"]          = (int)settings.loraChannel();
        lo["freq_mhz"]         = LoRaTx::channelToMhz(settings.loraChannel());
        lo["tx_power_idx"]     = (int)settings.loraTxPower();
        lo["encrypt"]          = settings.loraEncrypt();
        lo["trigger"]          = (int)settings.loraTxTrigger();
        lo["tx_period_s"]      = settings.loraTxPeriodS();
        lo["tx_count"]         = loraTx.txCount();
        lo["tx_failures"]      = loraTx.txFailures();
        lo["tx_bytes"]         = loraTx.txBytes();
        lo["last_tx_age_ms"]   = loraTx.lastTxAgeMs() == 0xFFFFFFFFUL
                                    ? -1L : (long)loraTx.lastTxAgeMs();
    }

    JsonObject s          = doc["settings"].to<JsonObject>();
    s["units"]            = (int)settings.units();
    s["theme"]            = (int)settings.theme();
    s["interval_s"]       = settings.intervalS();
    s["rotation"]         = (int)settings.rotation();
    s["rotate_rows"]      = settings.rotateRows();
    s["deep_sleep"]       = settings.deepSleepEnabled();
    s["temp_correction"]  = settings.tempCorrectionEnabled();
    s["fall_g"]           = settings.fallThreshG();
    s["impact_g"]         = settings.impactThreshG();
    s["fall_window_ms"]   = settings.fallWindowMs();
    s["alert_cooldown_ms"]= settings.alertCooldownMs();

    sendJson(req, doc);
}

// ----------------------------------------------------------------------------
//  /api/settings  (GET + POST)
// ----------------------------------------------------------------------------
static void handleGetSettings(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["units"]             = (int)settings.units();
    doc["theme"]             = (int)settings.theme();
    doc["interval_s"]        = settings.intervalS();
    doc["rotation"]          = (int)settings.rotation();
    doc["rotate_rows"]       = settings.rotateRows();
    doc["deep_sleep"]        = settings.deepSleepEnabled();
    doc["temp_correction"]   = settings.tempCorrectionEnabled();
    doc["fall_g"]            = settings.fallThreshG();
    doc["impact_g"]          = settings.impactThreshG();
    doc["fall_window_ms"]    = settings.fallWindowMs();
    doc["alert_cooldown_ms"] = settings.alertCooldownMs();
    doc["lora_on"]           = settings.loraEnabled();
    doc["lora_region"]       = (int)settings.loraRegion();
    doc["lora_channel"]      = (int)settings.loraChannel();
    doc["lora_tx_power"]     = (int)settings.loraTxPower();
    doc["lora_encrypt"]      = settings.loraEncrypt();
    doc["lora_key"]          = settings.loraKey();
    doc["lora_trigger"]      = (int)settings.loraTxTrigger();
    doc["lora_tx_period_s"]  = settings.loraTxPeriodS();
    sendJson(req, doc);
}

static void handlePostSettings(AsyncWebServerRequest* req, JsonVariant& json) {
    JsonObject obj = json.as<JsonObject>();
    if (!obj["units"].isNull())             settings.setUnits   ((Units)   obj["units"].as<int>());
    if (!obj["theme"].isNull())             settings.setTheme   ((Theme)   obj["theme"].as<int>());
    if (!obj["interval_s"].isNull())        settings.setIntervalS         (obj["interval_s"].as<uint32_t>());
    if (!obj["rotation"].isNull())          settings.setRotation((Rotation)obj["rotation"].as<int>());
    if (!obj["rotate_rows"].isNull())       settings.setRotateRows        (obj["rotate_rows"].as<uint32_t>());
    if (!obj["deep_sleep"].isNull())        settings.setDeepSleepEnabled  (obj["deep_sleep"].as<bool>());
    if (!obj["temp_correction"].isNull())   settings.setTempCorrectionEnabled(obj["temp_correction"].as<bool>());
    if (!obj["fall_g"].isNull())            settings.setFallThreshG       (obj["fall_g"].as<float>());
    if (!obj["impact_g"].isNull())          settings.setImpactThreshG     (obj["impact_g"].as<float>());
    if (!obj["fall_window_ms"].isNull())    settings.setFallWindowMs      (obj["fall_window_ms"].as<uint32_t>());
    if (!obj["alert_cooldown_ms"].isNull()) settings.setAlertCooldownMs   (obj["alert_cooldown_ms"].as<uint32_t>());

    // LoRa: track whether any RF-affecting field changed so we know to
    // re-push the config to the module.  Toggling lora_on also requires
    // a re-config (to bring it up or shut it down).
    bool loraDirty = false;
    if (!obj["lora_on"].isNull())        { settings.setLoraEnabled  (obj["lora_on"].as<bool>());          loraDirty = true; }
    if (!obj["lora_region"].isNull())    { settings.setLoraRegion   (obj["lora_region"].as<int>());      loraDirty = true; }
    if (!obj["lora_channel"].isNull())   { settings.setLoraChannel  (obj["lora_channel"].as<int>());     loraDirty = true; }
    if (!obj["lora_tx_power"].isNull())  { settings.setLoraTxPower  (obj["lora_tx_power"].as<int>());    loraDirty = true; }
    if (!obj["lora_encrypt"].isNull())   { settings.setLoraEncrypt  (obj["lora_encrypt"].as<bool>());    loraDirty = true; }
    if (!obj["lora_key"].isNull())       { settings.setLoraKey      (obj["lora_key"].as<uint16_t>());    loraDirty = true; }
    if (!obj["lora_trigger"].isNull())     settings.setLoraTxTrigger(obj["lora_trigger"].as<int>());
    if (!obj["lora_tx_period_s"].isNull()) settings.setLoraTxPeriodS(obj["lora_tx_period_s"].as<uint32_t>());

    if (loraDirty) {
        loraTx.applySettings();
    }

    JsonDocument out;
    out["ok"]                = true;
    out["units"]             = (int)settings.units();
    out["theme"]             = (int)settings.theme();
    out["interval_s"]        = settings.intervalS();
    out["rotation"]          = (int)settings.rotation();
    out["rotate_rows"]       = settings.rotateRows();
    out["deep_sleep"]        = settings.deepSleepEnabled();
    out["temp_correction"]   = settings.tempCorrectionEnabled();
    out["fall_g"]            = settings.fallThreshG();
    out["impact_g"]          = settings.impactThreshG();
    out["fall_window_ms"]    = settings.fallWindowMs();
    out["alert_cooldown_ms"] = settings.alertCooldownMs();
    out["lora_on"]           = settings.loraEnabled();
    out["lora_region"]       = (int)settings.loraRegion();
    out["lora_channel"]      = (int)settings.loraChannel();
    out["lora_tx_power"]     = (int)settings.loraTxPower();
    out["lora_encrypt"]      = settings.loraEncrypt();
    out["lora_key"]          = settings.loraKey();
    out["lora_trigger"]      = (int)settings.loraTxTrigger();
    out["lora_tx_period_s"]  = settings.loraTxPeriodS();
    sendJson(req, out);
}

// ----------------------------------------------------------------------------
//  Tare endpoints
// ----------------------------------------------------------------------------
static void handleTareNow(AsyncWebServerRequest* req) {
    float v = snow.tareNow();
    JsonDocument out;
    if (isnan(v)) {
        out["ok"]    = false;
        out["error"] = "no fresh sensor reading";
        sendJson(req, out, 409);
        return;
    }
    out["ok"]      = true;
    out["tare_cm"] = v;
    sendJson(req, out);
}

static void handleTareManual(AsyncWebServerRequest* req, JsonVariant& json) {
    JsonObject obj = json.as<JsonObject>();
    if (obj["value"].isNull()) {
        JsonDocument out;
        out["ok"]    = false;
        out["error"] = "missing 'value' (cm)";
        sendJson(req, out, 400);
        return;
    }
    float v = obj["value"].as<float>();
    if (v < 0) v = 0;
    snow.setManualTare(v);
    JsonDocument out;
    out["ok"]      = true;
    out["tare_cm"] = v;
    sendJson(req, out);
}

static void handleTareReset(AsyncWebServerRequest* req) {
    snow.resetTare();
    JsonDocument out;
    out["ok"]      = true;
    out["tare_cm"] = 0.0f;
    sendJson(req, out);
}

// ----------------------------------------------------------------------------
//  /api/time
// ----------------------------------------------------------------------------
static void handlePostTime(AsyncWebServerRequest* req, JsonVariant& json) {
    JsonObject obj = json.as<JsonObject>();
    JsonDocument out;
    if (obj["epoch"].isNull()) {
        out["ok"]    = false;
        out["error"] = "missing 'epoch' (UTC seconds)";
        sendJson(req, out, 400);
        return;
    }
    uint64_t ep = (uint64_t)obj["epoch"].as<double>();
    timekeeper.setEpoch(ep);
    out["ok"]     = true;
    out["iso"]    = timekeeper.iso8601();
    out["source"] = sourceLabel(timekeeper.source());
    sendJson(req, out);
}

// ----------------------------------------------------------------------------
//  /api/sd/*
// ----------------------------------------------------------------------------
static void handleSdInfo(AsyncWebServerRequest* req) {
    JsonDocument doc;
    doc["ready"]       = sdLogger.isReady();
    doc["rows"]        = sdLogger.rowCount();
    doc["total_bytes"] = (double)sdLogger.totalBytes();
    doc["used_bytes"]  = (double)sdLogger.usedBytes();
    doc["free_bytes"]  = (double)sdLogger.freeBytes();
    doc["nearly_full"] = sdLogger.isNearlyFull();
    doc["active_file"] = sdLogger.activeFile();
    sendJson(req, doc);
}

static void handleSdList(AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonArray arr = doc["files"].to<JsonArray>();
    auto files = sdLogger.listCsvFiles();
    for (auto& f : files) {
        JsonObject o = arr.add<JsonObject>();
        o["name"] = f.name;
        o["size"] = (uint32_t)f.size;
    }
    sendJson(req, doc);
}

static void handleSdDownload(AsyncWebServerRequest* req) {
    if (!req->hasParam("file")) {
        req->send(400, "text/plain", "missing 'file' parameter");
        return;
    }
    String path = req->getParam("file")->value();
    if (!path.startsWith("/")) path = "/" + path;
    if (!sdLogger.isReady() || !SD.exists(path.c_str())) {
        req->send(404, "text/plain", "file not found");
        return;
    }
    AsyncWebServerResponse* resp =
        req->beginResponse(SD, path, "text/csv", /*download=*/true);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
}

static void handleSdPreview(AsyncWebServerRequest* req) {
    String path  = req->hasParam("file")
                     ? req->getParam("file")->value()
                     : sdLogger.activeFile();
    size_t lines = req->hasParam("lines")
                     ? (size_t) req->getParam("lines")->value().toInt()
                     : 50;
    if (!path.startsWith("/")) path = "/" + path;
    String body = sdLogger.previewFile(path.c_str(), lines);
    AsyncWebServerResponse* resp = req->beginResponse(200, "text/plain", body);
    resp->addHeader("Cache-Control", "no-store");
    req->send(resp);
}

static void handleSdClear(AsyncWebServerRequest* req) {
    bool ok = sdLogger.clearLog();
    JsonDocument out;
    out["ok"]   = ok;
    out["rows"] = sdLogger.rowCount();
    sendJson(req, out, ok ? 200 : 500);
}

// ----------------------------------------------------------------------------
//  /api/imu/events  -- recent fall/impact events ring buffer (newest first)
// ----------------------------------------------------------------------------
static void handleImuEvents(AsyncWebServerRequest* req) {
    size_t count = 0;
    const Imu::Event* ring = imu.recentEvents(count);

    JsonDocument doc;
    JsonArray arr = doc["events"].to<JsonArray>();

    // The ring is filled with the oldest entry at index 0 (when not yet full)
    // or in chronological order rotating around _ringHead (when full).
    // We iterate by bootMs to surface newest first regardless of layout.
    // Simple O(n^2) sort is fine for IMU_EVENT_RING_SIZE = 32.
    int order[IMU_EVENT_RING_SIZE];
    for (size_t i = 0; i < count; i++) order[i] = (int)i;
    for (size_t i = 0; i < count; i++) {
        for (size_t j = i + 1; j < count; j++) {
            if (ring[order[j]].bootMs > ring[order[i]].bootMs) {
                int tmp = order[i]; order[i] = order[j]; order[j] = tmp;
            }
        }
    }

    uint32_t now = millis();
    for (size_t i = 0; i < count; i++) {
        const Imu::Event& e = ring[order[i]];
        JsonObject o = arr.add<JsonObject>();
        o["type"]      = Imu::eventName(e.type);
        o["mag"]       = (double)e.mag;
        o["ax"]        = (double)e.ax;
        o["ay"]        = (double)e.ay;
        o["az"]        = (double)e.az;
        o["duration_ms"] = (uint32_t)e.durationMs;
        o["age_ms"]    = (long)(now - e.bootMs);
    }
    doc["count"]       = (uint32_t)count;
    doc["fall_total"]  = imu.fallCount();
    doc["impact_total"]= imu.impactCount();
    sendJson(req, doc);
}

// ----------------------------------------------------------------------------
//  /api/imu/reset_peak  -- zero the peak-magnitude tracker
// ----------------------------------------------------------------------------
static void handleImuResetPeak(AsyncWebServerRequest* req) {
    imu.resetPeak();
    JsonDocument out;
    out["ok"] = true;
    sendJson(req, out);
}

// ----------------------------------------------------------------------------
//  /tiles/{z}/{x}/{y}.png  -- offline map tiles served from SD card
//  Layout on SD: /tiles/{z}/{x}/{y}.png  (standard slippy-map directory)
//  Returns 404 if the tile is missing -- the JS map gracefully falls back
//  to the vector graticule overlay in that case.
// ----------------------------------------------------------------------------
static void handleTile(AsyncWebServerRequest* req) {
    if (!sdLogger.isReady()) { req->send(404, "text/plain", "no sd"); return; }
    if (!req->hasParam("z") || !req->hasParam("x") || !req->hasParam("y")) {
        req->send(400, "text/plain", "missing z/x/y"); return;
    }
    String z = req->getParam("z")->value();
    String x = req->getParam("x")->value();
    String y = req->getParam("y")->value();
    // Sanitise: only digits allowed
    for (auto& s : { &z, &x, &y }) {
        for (size_t i = 0; i < s->length(); i++) {
            if (!isDigit((*s)[i])) {
                req->send(400, "text/plain", "bad coords"); return;
            }
        }
    }
    String path = "/tiles/" + z + "/" + x + "/" + y + ".png";
    if (!SD.exists(path.c_str())) {
        req->send(404, "text/plain", "tile not on SD");
        return;
    }
    AsyncWebServerResponse* resp = req->beginResponse(SD, path, "image/png");
    resp->addHeader("Cache-Control", "max-age=86400");
    req->send(resp);
}

// ----------------------------------------------------------------------------
//  /api/ultrasonic/live  -- minimal payload for high-frequency UI polling
//
//  Front page polls this at ~5 Hz to make the live distance/depth readout
//  feel responsive. Keeping the JSON tiny (< 200 bytes) keeps AsyncTCP
//  happy and lets us hit 5 Hz comfortably even with multiple clients.
//  Use /api/status for everything else (1 Hz is plenty).
// ----------------------------------------------------------------------------
static void handleUltrasonicLive(AsyncWebServerRequest* req) {
    JsonDocument doc;
    JsonObject   root = doc.to<JsonObject>();
    root["fresh"]   = ultrasonic.isFresh();
    addJson(root, "distance_cm", ultrasonic.distanceCm(), 1);
    addJson(root, "depth_cm",    snow.snowDepthCm(),      1);
    root["outlier"] = outlier.lastFlagged();
    root["last_ms_ago"] = ultrasonic.lastReadMs() == 0 ? -1
                            : (long)(millis() - ultrasonic.lastReadMs());
    root["frames"]  = ultrasonic.totalFrames();
    root["t_ms"]    = millis();

    String body;
    serializeJson(doc, body);
    auto* res = req->beginResponse(200, "application/json", body);
    res->addHeader("Cache-Control", "no-store");
    req->send(res);
}

// ----------------------------------------------------------------------------
//  begin()
// ----------------------------------------------------------------------------
void WebUI::begin() {
    // ---- API: GET (must come BEFORE serveStatic) --------------------------
    server.on("/api/status",     HTTP_GET, handleStatus);
    server.on("/api/ultrasonic/live", HTTP_GET, handleUltrasonicLive);
    server.on("/api/settings",   HTTP_GET, handleGetSettings);
    server.on("/api/sd/info",    HTTP_GET, handleSdInfo);
    server.on("/api/sd/list",    HTTP_GET, handleSdList);
    server.on("/api/sd/download",HTTP_GET, handleSdDownload);
    server.on("/api/sd/preview", HTTP_GET, handleSdPreview);
    server.on("/api/imu/events", HTTP_GET, handleImuEvents);
    server.on("/tiles",          HTTP_GET, handleTile);     // /tiles?z=&x=&y=

    // ---- API: POST (no body) ----------------------------------------------
    // NOTE: register more-specific paths FIRST. Some ESPAsyncWebServer
    // builds match handlers in registration order, so if "/api/tare" were
    // listed before "/api/tare/reset" the reset endpoint could shadow as
    // a plain tare-now call.
    server.on("/api/tare/reset",      HTTP_POST, handleTareReset);
    server.on("/api/tare",            HTTP_POST, handleTareNow);
    server.on("/api/sd/clear",        HTTP_POST, handleSdClear);
    server.on("/api/imu/reset_peak",  HTTP_POST, handleImuResetPeak);

    // ---- API: POST (JSON body) -------------------------------------------
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/settings",    handlePostSettings));
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/tare/manual", handleTareManual));
    server.addHandler(new AsyncCallbackJsonWebHandler("/api/time",        handlePostTime));

    // ---- Static UI (LAST -- matches every remaining URL) -----------------
    server.serveStatic("/", LittleFS, "/")
          .setDefaultFile("index.html")
          .setCacheControl("max-age=600");

    // ---- 404 ---------------------------------------------------------------
    server.onNotFound([](AsyncWebServerRequest* req) {
        if (req->url().startsWith("/api/")) {
            req->send(404, "application/json", "{\"error\":\"not found\"}");
        } else {
            req->redirect("/");
        }
    });

    server.begin();
    DBG("WEB", "HTTP server up on port %d", WEB_PORT);
}
