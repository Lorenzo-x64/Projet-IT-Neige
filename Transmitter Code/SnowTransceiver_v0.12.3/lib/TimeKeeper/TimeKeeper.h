// ============================================================================
//  TimeKeeper.h  --  ISO 8601 timestamp generation with cascading fallbacks
//
//  Source priority (best -> worst):
//      1. NTP-synchronised system clock                  (Source::Ntp)
//      2. User-set time pushed from the dashboard        (Source::Manual)
//      3. millis()-based relative timestamp from boot    (Source::Boot)
//
//  Whenever any caller (web client, dashboard JS) sends us a real wall-clock
//  time, we accept it and upgrade ourselves to "Manual".  If at any point
//  the ESP32 manages to sync NTP we upgrade to "Ntp".
//
//  All exported timestamps use ISO 8601 with a Z (UTC) suffix when possible,
//  falling back to a "PT..." duration form (relative to boot) when no real
//  clock is available -- which is still sortable as text.
// ============================================================================
#pragma once

#include <Arduino.h>

class TimeKeeper {
public:
    enum class Source : uint8_t { Boot = 0, Manual = 1, Ntp = 2 };

    void   begin();

    // Accept a UTC time as POSIX seconds.  Used by the dashboard "Set time"
    // button and could also be called from a future GPS module.
    void   setEpoch(uint64_t epochSeconds);

    // Try once to sync via NTP (requires station-mode internet -- not used
    // by default in this firmware but kept for future expansion).
    void   tryNtp();

    // Produce an ISO 8601 timestamp suitable for CSV logging.
    //   - "2026-05-04T12:34:56Z"        (NTP or manual)
    //   - "PT0H0M12S" or "BOOT+12.345s" (no clock yet)
    String iso8601() const;

    // Best-guess "just date" (YYYY-MM-DD) and "just time" (HH:MM:SS) accessors
    String dateString() const;
    String timeString() const;

    Source source() const { return _source; }
    bool   hasRealClock() const { return _source != Source::Boot; }

private:
    Source _source = Source::Boot;
};

extern TimeKeeper timekeeper;
