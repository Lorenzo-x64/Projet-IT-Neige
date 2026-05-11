// ============================================================================
//  TimeKeeper.cpp
// ============================================================================
#include "TimeKeeper.h"
#include "Config.h"
#include <time.h>
#include <sys/time.h>

TimeKeeper timekeeper;

// ----------------------------------------------------------------------------
void TimeKeeper::begin() {
    // No real clock yet.  We will keep returning boot-relative timestamps
    // until somebody sets the time (manual or NTP).
    _source = Source::Boot;
    DBG("TIME", "Boot-relative mode (no real clock yet)");
}

// ----------------------------------------------------------------------------
void TimeKeeper::setEpoch(uint64_t epochSeconds) {
    // If we are already at NTP quality, only accept upgrades, not downgrades.
    if (_source == Source::Ntp) return;

    struct timeval tv;
    tv.tv_sec  = (time_t)epochSeconds;
    tv.tv_usec = 0;
    settimeofday(&tv, nullptr);
    _source = Source::Manual;
    DBG("TIME", "Manual time set: epoch=%llu", (unsigned long long)epochSeconds);
}

// ----------------------------------------------------------------------------
void TimeKeeper::tryNtp() {
    // Placeholder: the device runs in AP-only mode by default, so NTP is
    // not reachable.  This hook is here so a future station-mode build can
    // call configTzTime("UTC0", "pool.ntp.org") and upgrade _source.
}

// ----------------------------------------------------------------------------
String TimeKeeper::iso8601() const {
    if (_source == Source::Boot) {
        // No real clock -- emit a boot-relative, sortable token.
        uint32_t s   = millis() / 1000;
        uint32_t ms  = millis() % 1000;
        char buf[32];
        snprintf(buf, sizeof(buf), "BOOT+%lu.%03lus",
                 (unsigned long)s, (unsigned long)ms);
        return String(buf);
    }
    time_t now = time(nullptr);
    struct tm t;
    gmtime_r(&now, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &t);
    return String(buf);
}

// ----------------------------------------------------------------------------
String TimeKeeper::dateString() const {
    if (_source == Source::Boot) return String("1970-01-01");
    time_t now = time(nullptr);
    struct tm t;
    gmtime_r(&now, &t);
    char buf[16];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &t);
    return String(buf);
}

// ----------------------------------------------------------------------------
String TimeKeeper::timeString() const {
    if (_source == Source::Boot) {
        uint32_t s = millis() / 1000;
        uint32_t hh = (s / 3600) % 24;
        uint32_t mm = (s /   60) % 60;
        uint32_t ss =  s         % 60;
        char buf[16];
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu",
                 (unsigned long)hh, (unsigned long)mm, (unsigned long)ss);
        return String(buf);
    }
    time_t now = time(nullptr);
    struct tm t;
    gmtime_r(&now, &t);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &t);
    return String(buf);
}
