// ============================================================================
//  WebUI.h  --  AsyncWebServer + REST API for the dashboard
//
//  Static UI files (index.html, style.css, app.js) live in LittleFS and are
//  served from the root path.  All dynamic data goes through /api/*.
//
//  Endpoints:
//      GET  /                        index.html (LittleFS)
//      GET  /css/style.css           static
//      GET  /js/app.js               static
//
//      GET  /api/status              live distance, depth, sensor health, ...
//      GET  /api/ultrasonic/live     minimal high-frequency feed (~5 Hz UI)
//      GET  /api/settings            current settings JSON
//      POST /api/settings            { units, theme, interval_s }
//      POST /api/tare                trigger tare from live reading
//      POST /api/tare/manual         { value: 240.5 }
//      POST /api/tare/reset          set tare to 0
//      POST /api/time                { epoch: <seconds_utc> }
//      GET  /api/sd/info             total / used / free / nearly_full
//      GET  /api/sd/list             [{ name, size }]
//      GET  /api/sd/download?file=   raw download
//      GET  /api/sd/preview?file=&lines=
//      POST /api/sd/clear            wipe active log
// ============================================================================
#pragma once

#include <Arduino.h>

class WebUI {
public:
    void begin();
};

extern WebUI webui;
