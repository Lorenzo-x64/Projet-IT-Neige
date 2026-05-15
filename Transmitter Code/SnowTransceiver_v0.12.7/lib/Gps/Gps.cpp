// ============================================================================
//  Gps.cpp  --  implementation
//
//  GPS uses EspSoftwareSerial (not a hardware UART) because all 3 ESP32
//  hardware UARTs are spoken for: UART0=USB, UART1=LoRa, UART2=Ultrasonic
//  (the primary measurement sensor and not negotiable). This is acceptable
//  here because:
//    - 9600 baud -> ~104us per bit, longer than typical WiFi IRQ latency
//    - TinyGPSPlus validates every NMEA sentence with a *XX checksum and
//      silently drops corrupted ones
//    - The receiver sends multiple sentences per second, so a dropped
//      sentence just means we use the next one ~1 s later
//    - Position changes slowly; sub-second freshness is not required
// ============================================================================
#include "Gps.h"
#include "Config.h"
#include "TimeKeeper.h"
#include <SoftwareSerial.h>
#include <TinyGPS++.h>
#include <time.h>

Gps gps;

static SoftwareSerial gpsSerial;
static TinyGPSPlus    parser;

// ----------------------------------------------------------------------------
void Gps::begin() {
    gpsSerial.begin(GPS_BAUD, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    // Larger buffer helps absorb bursts during WiFi activity:
    gpsSerial.enableIntTx(false);  // we only listen, never transmit configs
    DBG("GPS", "SoftwareSerial RX=GPIO%d TX=GPIO%d @ %d baud  (TinyGPSPlus v%s)",
        GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD, TinyGPSPlus::libraryVersion());
}

// ----------------------------------------------------------------------------
void Gps::loop() {
    while (gpsSerial.available()) {
        uint8_t b = gpsSerial.read();
        _chars++;
        if (parser.encode(b)) {
            // Sentence completed -- pull whatever fields are valid.
            if (parser.location.isValid()) {
                _lat       = parser.location.lat();
                _lon       = parser.location.lng();
                _hasFix    = true;
                _lastFixMs = millis();
                _sentencesWithFix++;
            }
            if (parser.altitude.isValid())   _altM      = parser.altitude.meters();
            if (parser.speed.isValid())      _speedKmh  = parser.speed.kmph();
            if (parser.course.isValid())     _courseDeg = parser.course.deg();
            if (parser.satellites.isValid()) _sats      = parser.satellites.value();
            if (parser.hdop.isValid())       _hdop      = parser.hdop.hdop();

            // Push UTC -> TimeKeeper exactly once per session as soon as we
            // get a valid date+time.  Using a manual UTC->epoch calculation
            // is more portable than mktime/timegm (mktime treats input as
            // local time; TZ may or may not be set on ESP32 newlib).
            if (!_clockPushed && parser.date.isValid() && parser.time.isValid()) {
                int Y  = parser.date.year();
                int M  = parser.date.month();
                int D  = parser.date.day();
                int hh = parser.time.hour();
                int mm = parser.time.minute();
                int ss = parser.time.second();

                // Days since 1970-01-01 (Howard Hinnant algorithm)
                Y -= (M <= 2) ? 1 : 0;
                int era    = (Y >= 0 ? Y : Y - 399) / 400;
                unsigned yoe = (unsigned)(Y - era * 400);
                unsigned doy = (153 * (M + (M > 2 ? -3 : 9)) + 2) / 5 + D - 1;
                unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
                long days = (long)era * 146097L + (long)doe - 719468L;
                int64_t ep = (int64_t)days * 86400LL +
                             (int64_t)hh * 3600LL +
                             (int64_t)mm * 60LL +
                             (int64_t)ss;
                if (ep > 1700000000LL) {     // sanity: must be after ~2023-11
                    timekeeper.setEpoch((uint64_t)ep);
                    _clockPushed = true;
                    DBG("GPS", "system clock set from GPS: %s",
                        timekeeper.iso8601().c_str());
                }
            }
        }
    }
    _failedChecksum = parser.failedChecksum();
}

// ----------------------------------------------------------------------------
bool Gps::isFresh(uint32_t maxAgeMs) const {
    if (_lastFixMs == 0) return false;
    return (millis() - _lastFixMs) < maxAgeMs;
}

// ----------------------------------------------------------------------------
void Gps::printStats() const {
    if (!_hasFix) {
        Serial.printf("[ GPS   ] %-5s  no fix yet     chars=%lu  sats=%lu  fc=%lu\n",
                      "WAIT",
                      (unsigned long)_chars,
                      (unsigned long)_sats,
                      (unsigned long)_failedChecksum);
        return;
    }
    Serial.printf("[ GPS   ] %-5s  %.6f, %.6f  alt=%.1fm  sats=%lu  HDOP=%.1f  age=%lums\n",
                  isFresh() ? "ok" : "STALE",
                  _lat, _lon, _altM,
                  (unsigned long)_sats,
                  _hdop,
                  (unsigned long)(millis() - _lastFixMs));
}
