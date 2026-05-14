// ============================================================================
//  LoRaTx.h  --  Snow Transceiver -> remote receiver, over Ebyte E220
//
//  This module is TRANSMIT ONLY.  The receiver runs separate firmware (in
//  a different project) and uses the same E220 module paired with another
//  ESP32 to decode the frames described below.
//
// ----------------------------------------------------------------------------
//  PROTOCOL  (version 1, see LORA_PROTOCOL_VERSION in Config.h)
// ----------------------------------------------------------------------------
//  Each LoRa transmission is a single self-contained frame. The E220's
//  internal packet engine handles the radio framing/CRC; we add our own
//  application-layer envelope on top so the receiver can:
//      - dispatch by frame type (CSV row vs. alert vs. heartbeat)
//      - drop corrupted/garbled bytes (CRC16 over payload)
//      - reject frames sent with a different protocol version
//      - decrypt the payload with the matching shared key
//
//  Wire format (bytes, in transmission order):
//
//      Offset  Size   Field           Description
//      ------  ----   -----           -----------
//      0       1      MAGIC           0xA5  -- start-of-frame sync byte
//      1       1      VERSION         1     -- LORA_PROTOCOL_VERSION
//      2       1      TYPE            see FrameType enum below
//      3       1      LEN             payload length in bytes (0..240)
//      4       2      SEQ             little-endian uint16 sequence number
//      6       LEN    PAYLOAD         frame-type-specific bytes
//      6+LEN   2      CRC16           little-endian, CRC-16/CCITT-FALSE
//                                     computed over bytes 0 .. 6+LEN-1
//
//  Maximum frame size: 6 + 240 + 2 = 248 bytes.  This stays under the
//  E220's 200-byte single-packet limit only when LEN <= 192; for longer
//  payloads (rare, only the full CSV header line) we let the E220's
//  built-in sub-packetisation handle it.
//
//  ENCRYPTION:
//      The E220 has a built-in 16-bit register (CRYPT_H/CRYPT_L) that
//      XOR-obfuscates the air bytes.  Both transmitter and receiver MUST
//      configure the same key, otherwise the receiver sees garbage bytes
//      and CRC16 will reject everything.  This is obscurity-grade, not
//      real cryptography, but it stops a stranger with a stock E220 from
//      seeing your data.
//
// ----------------------------------------------------------------------------
//  FRAME TYPES
// ----------------------------------------------------------------------------
//
//  TYPE = 0x01  CSV_ROW
//      ASCII payload, identical to one row of /snowlog.csv (no trailing
//      \r\n).  Receiver can directly append "\r\n" + the bytes to a local
//      copy of the CSV.  Schema:
//          timestamp,distance_cm,depth_cm,outlier,event,
//          lat,lon,alt_m,speed_kmh,sats,hdop
//
//  TYPE = 0x02  ALERT_FALL
//      ASCII payload "FALL,<mag_g>,<dur_ms>,<iso8601>"
//      e.g. "FALL,0.183,210,2026-05-04T12:35:25Z"
//
//  TYPE = 0x03  ALERT_IMPACT
//      ASCII payload "IMPACT,<mag_g>,<iso8601>"
//      e.g. "IMPACT,4.823,2026-05-04T12:35:26Z"
//
//  TYPE = 0x04  ALERT_SD_FULL
//      ASCII payload "SDFULL,<used_pct>,<iso8601>"
//      Fired once when SD usage crosses 90%, then re-armed when it drops
//      back below 85% (built-in hysteresis).
//
//  TYPE = 0x05  ALERT_SENSOR_LOST
//      ASCII payload "LOST,<which>,<iso8601>"
//      where <which> is one of: ULTRASONIC, GPS, IMU, BATTERY
//      Fired when a sensor that was previously fresh stops producing
//      data for > 30 seconds (configurable via SENSOR_STALE_MS).
//
//  TYPE = 0x06  HEARTBEAT
//      ASCII payload "HB,<uptime_s>,<rssi>,<heap>,<bat_pct>"
//      Sent every 60s when LoRa is enabled regardless of other traffic,
//      so the receiver can tell "transmitter is alive but quiet" from
//      "transmitter has fallen off the network".
// ============================================================================
#pragma once

#include <Arduino.h>
#include "Config.h"

class LoRaTx {
public:
    enum class FrameType : uint8_t {
        CsvRow         = 0x01,
        AlertFall      = 0x02,
        AlertImpact    = 0x03,
        AlertSdFull    = 0x04,
        AlertSensorLost= 0x05,
        Heartbeat      = 0x06,
    };

    enum class State : uint8_t {
        Disabled = 0,         // settings.loraEnabled() == false
        InitFailed,           // module not responding on UART
        Idle,                 // ready to send
        Configuring,          // applying new settings (mode 3)
        Transmitting,         // AUX low, frame in flight
    };

    void  begin();
    void  loop();
    void  printStats() const;

    State state() const                   { return _state; }
    bool  isReady() const                 { return _state == State::Idle; }
    uint32_t txCount() const              { return _txCount; }
    uint32_t txBytes() const              { return _txBytes; }
    uint32_t txFailures() const           { return _txFailures; }
    uint32_t lastTxAgeMs() const          { return _lastTxMs ? (millis() - _lastTxMs) : 0xFFFFFFFFUL; }

    // Send a CSV row (one line, no trailing CRLF).  Returns true if queued.
    bool  sendCsvRow(const String& csvLine);

    // Send an alert.  Buffered behind any in-flight CSV row but always
    // takes priority over future CSV rows in the queue.
    bool  sendAlertFall   (float mag, uint32_t durationMs, const String& iso);
    bool  sendAlertImpact (float mag, const String& iso);
    bool  sendAlertSdFull (float usedPct, const String& iso);
    bool  sendAlertSensorLost(const char* which, const String& iso);

    // Re-apply settings to the module (call from WebUI when the user
    // changes channel/key/region/power).  Returns false on UART error.
    bool  applySettings();

    // Compute centre frequency (MHz) for a given channel.
    static float channelToMhz(uint8_t ch) {
        return LORA_FREQ_BASE_MHZ + (float)ch;
    }

private:
    State    _state         = State::Disabled;
    uint16_t _seq           = 0;
    uint32_t _txCount       = 0;
    uint32_t _txBytes       = 0;
    uint32_t _txFailures    = 0;
    uint32_t _lastTxMs      = 0;
    uint32_t _lastHeartbeatMs = 0;

    // Internal helpers
    bool _setMode(uint8_t mode);            // 0..3 per datasheet
    bool _waitAux(uint32_t timeoutMs);
    bool _writeConfig();
    bool _sendFrame(FrameType t, const uint8_t* payload, size_t len);
    void _heartbeatTick();

    // CRC-16/CCITT-FALSE (init 0xFFFF, poly 0x1021)
    static uint16_t _crc16(const uint8_t* data, size_t n);
};

extern LoRaTx loraTx;
