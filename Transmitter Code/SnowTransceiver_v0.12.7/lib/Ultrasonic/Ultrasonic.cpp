// ============================================================================
//  Ultrasonic.cpp  --  implementation
//
//  PRIMARY SENSOR — uses HardwareSerial (UART2) with a real RX FIFO.  The
//  device-side ULPM filter on the A01NYUB requires receive timing precise
//  enough that bit-banging would risk dropped frames during WiFi/AsyncTCP
//  bursts. UART2 has a hardware FIFO and is unaffected by interrupt load.
//
//  UART map (see platformio.ini):
//    UART0 -> USB monitor   UART1 -> LoRa   UART2 -> THIS (ultrasonic)
//    GPS lives on EspSoftwareSerial because it tolerates dropped sentences
//    (TinyGPSPlus checksums every NMEA line and the next one comes 1 s later).
// ============================================================================
#include "Ultrasonic.h"
#include "Config.h"
#include "AppSettings.h"
#include "AhtSensor.h"
#include <HardwareSerial.h>
#include <math.h>

Ultrasonic ultrasonic;

// UART2 with the RX pin remapped to whatever Config.h declares.  TX is
// disabled (-1) since we only listen to the sensor.
static HardwareSerial usSerial(2);

// ----------------------------------------------------------------------------
void Ultrasonic::begin() {
    usSerial.begin(US_BAUD, SERIAL_8N1, US_RX_PIN, /*tx=*/-1);
    DBG("US", "UART2 RX=GPIO%d (TX unused) @ %d baud", US_RX_PIN, US_BAUD);
}

// ----------------------------------------------------------------------------
void Ultrasonic::loop() {
    while (usSerial.available()) {
        uint8_t b = usSerial.read();

        // Wait for the sync byte (0xFF) before accepting any data.
        if (_idx == 0 && b != 0xFF) continue;

        _buf[_idx++] = b;

        if (_idx == 4) {
            _idx = 0;
            _totalFrames++;

            uint8_t ck = (_buf[0] + _buf[1] + _buf[2]) & 0xFF;
            if (ck != _buf[3]) {
                _checksumErrors++;
                continue;
            }

            // Convert mm -> cm
            float dist = ((_buf[1] << 8) | _buf[2]) / 10.0f;

            if (dist >= US_DIST_MIN_CM && dist <= US_DIST_MAX_CM) {
                _distCm     = dist;
                _lastReadMs = millis();
            } else {
                _outOfRange++;
            }
        }
    }
}

// ----------------------------------------------------------------------------
bool Ultrasonic::isFresh(uint32_t maxAgeMs) const {
    if (_lastReadMs == 0) return false;
    return (millis() - _lastReadMs) < maxAgeMs;
}

// ----------------------------------------------------------------------------
//  distanceCm() -- raw or temperature-compensated depending on settings.
//
//  Speed of sound in air:  v(T) = 331.3 + 0.606 * T   (m/s, T in deg C)
//  The A01NYUB reports its echo-time measurement as a distance computed
//  internally assuming a 20 deg C calibration (v_ref = 343.42 m/s). The
//  real distance is therefore  d_actual = d_reported * v(T) / v_ref.
//
//  At 0  deg C: factor = 0.965  ( ~3.5% shorter than nominal )
//  At -20 deg C: factor = 0.930  ( ~7.0% shorter -- 21 cm of error on 300 cm )
//  Above ~20 deg C the correction increases distance slightly.
// ----------------------------------------------------------------------------
float Ultrasonic::distanceCm() const {
    // No reading yet -- return the sentinel as-is so callers can detect it.
    if (_distCm < 0) return _distCm;

    // Pass-through when the feature is off or there's no fresh temperature
    // to base the compensation on (cold-start, AHT disconnected, etc.).
    if (!settings.tempCorrectionEnabled()) return _distCm;
    if (!aht.isFresh())                    return _distCm;

    const float tC = aht.tempC();
    if (isnan(tC)) return _distCm;

    constexpr float V_REF = 343.42f;            // 331.3 + 0.606 * 20
    const float vT   = 331.3f + 0.606f * tC;
    const float corr = _distCm * (vT / V_REF);
    return corr;
}

// ----------------------------------------------------------------------------
void Ultrasonic::printStats() const {
    char age[16];
    if (_lastReadMs == 0) {
        snprintf(age, sizeof(age), "never");
    } else {
        snprintf(age, sizeof(age), "%lums", (unsigned long)(millis() - _lastReadMs));
    }
    if (_distCm < 0) {
        Serial.printf("[ US    ] %-5s  no data        frames=%lu  cks=%lu  oor=%lu  age=%s\n",
                      "WAIT",
                      (unsigned long)_totalFrames,
                      (unsigned long)_checksumErrors,
                      (unsigned long)_outOfRange,
                      age);
    } else {
        Serial.printf("[ US    ] %-5s  %6.1f cm     frames=%lu  cks=%lu  oor=%lu  age=%s\n",
                      isFresh() ? "ok" : "STALE",
                      _distCm,
                      (unsigned long)_totalFrames,
                      (unsigned long)_checksumErrors,
                      (unsigned long)_outOfRange,
                      age);
    }
}