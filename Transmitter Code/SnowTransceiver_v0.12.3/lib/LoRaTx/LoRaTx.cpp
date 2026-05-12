// ============================================================================
//  LoRaTx.cpp  --  Ebyte E220 transmitter driver
//
//  No external library: we talk to the E220 directly via HardwareSerial
//  (UART1).  The xreef library works fine but adds a layer we don't need
//  -- the protocol below is small enough that a custom driver is cleaner.
// ============================================================================
#include "LoRaTx.h"
#include "Config.h"
#include "AppSettings.h"
#include "Battery.h"
#include "TimeKeeper.h"
#include <HardwareSerial.h>

LoRaTx loraTx;

static HardwareSerial loraSerial(1);   // UART1 (UART0=USB, UART2=ultrasonic)

// ----------------------------------------------------------------------------
//  CRC-16/CCITT-FALSE: init 0xFFFF, poly 0x1021, no xor-out, no reflect.
//  Standard for LoRa-link layers, plenty good for a 248-byte frame.
// ----------------------------------------------------------------------------
uint16_t LoRaTx::_crc16(const uint8_t* data, size_t n) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < n; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                                 : (uint16_t)(crc << 1);
        }
    }
    return crc;
}

// ----------------------------------------------------------------------------
//  Mode pin control:
//      Mode 0  (M1=0, M0=0)  Normal transparent transmission
//      Mode 1  (M1=0, M0=1)  WOR sender
//      Mode 2  (M1=1, M0=0)  WOR receiver
//      Mode 3  (M1=1, M0=1)  Configuration (must be in mode 3 to write
//                            registers; UART runs at 9600 8N1 in this mode
//                            regardless of normal-mode baud)
// ----------------------------------------------------------------------------
bool LoRaTx::_setMode(uint8_t mode) {
    digitalWrite(LORA_M1_PIN, (mode >> 1) & 1);
    digitalWrite(LORA_M0_PIN, (mode >> 0) & 1);
    delay(15);                          // T_mode (datasheet: ~5-15ms)
    return _waitAux(200);
}

// ----------------------------------------------------------------------------
//  Wait for AUX to go HIGH (= module ready, buffer drained).
//  Returns false on timeout.
// ----------------------------------------------------------------------------
bool LoRaTx::_waitAux(uint32_t timeoutMs) {
    uint32_t t0 = millis();
    while (digitalRead(LORA_AUX_PIN) == LOW) {
        if (millis() - t0 > timeoutMs) return false;
        delay(2);
    }
    // Datasheet: wait at least 2ms after rising edge before next op
    delay(3);
    return true;
}

// ----------------------------------------------------------------------------
//  Apply current AppSettings to the module's registers.
//
//  Register layout (datasheet §7.2):
//      00H  ADDH      -- broadcast 0xFF
//      01H  ADDL      -- broadcast 0xFF
//      02H  REG0      -- bits 7..5 UART baud, bits 4..3 parity, bits 2..0 air rate
//      03H  REG1      -- bits 7..6 sub-packet, bit 5 RSSI noise, bits 1..0 TX power
//      04H  REG2      -- channel
//      05H  REG3      -- bit 7 RSSI byte, bit 6 fixed-tx, bit 4 LBT, bits 2..0 WOR cycle
//      06H  CRYPT_H   -- key high byte
//      07H  CRYPT_L   -- key low byte
//
//  We use a single C0 command to write all 8 bytes at once.
// ----------------------------------------------------------------------------
bool LoRaTx::_writeConfig() {
    if (!_setMode(3)) {
        DBG("LORA", "could not enter configuration mode (AUX timeout)");
        return false;
    }

    // Configuration mode runs at 9600 8N1 regardless of the normal-mode
    // UART baud setting -- we always rebuild the UART link at 9600 here.
    loraSerial.flush();
    loraSerial.end();
    delay(10);
    loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);
    delay(20);

    uint8_t cmd[3 + 8] = {
        0xC0,                                   // "set, save, no echo"
        0x00,                                   // start address
        0x08,                                   // length (8 registers)
        // payload: 8 bytes for registers 00..07
        LORA_DEFAULT_ADDR_HI,                   // ADDH
        LORA_DEFAULT_ADDR_LO,                   // ADDL
        // REG0: 9600 8N1 (UART) | air rate from settings
        // bits 7..5 = 011 (9600), bits 4..3 = 00 (8N1), bits 2..0 = air rate
        (uint8_t)(0b01100000 | (LORA_AIR_RATE_DEFAULT & 0x07)),
        // REG1: 200 byte sub-packet, RSSI noise off, TX power
        (uint8_t)(0b00000000 | (settings.loraTxPower() & 0x03)),
        // REG2: channel
        settings.loraChannel(),
        // REG3: RSSI byte off, transparent (broadcast), LBT off, WOR 2s
        0b00000000,
        // CRYPT_H / CRYPT_L
        settings.loraEncrypt() ? (uint8_t)((settings.loraKey() >> 8) & 0xFF) : 0x00,
        settings.loraEncrypt() ? (uint8_t)( settings.loraKey()       & 0xFF) : 0x00,
    };

    // Drain any leftover RX bytes from a previous session
    while (loraSerial.available()) (void)loraSerial.read();

    loraSerial.write(cmd, sizeof(cmd));
    loraSerial.flush();

    // Read the response.  Module replies with C1 + same payload (~11 bytes).
    uint8_t resp[16];
    size_t got = 0;
    uint32_t t0 = millis();
    while (millis() - t0 < 1000 && got < sizeof(resp)) {
        if (loraSerial.available()) {
            resp[got++] = (uint8_t)loraSerial.read();
            if (got >= 3 && resp[0] == 0xC1 && got >= 3 + resp[2]) break;
        }
    }

    if (got >= 3 && resp[0] == 0xC1) {
        DBG("LORA", "config OK  ch=%u  freq=%.3fMHz  pwr=%u  enc=%s  key=0x%04X",
            (unsigned)settings.loraChannel(),
            (double)channelToMhz(settings.loraChannel()),
            (unsigned)settings.loraTxPower(),
            settings.loraEncrypt() ? "ON" : "off",
            settings.loraKey());
    } else {
        DBG("LORA", "config FAILED -- no/bad response from module (%u bytes)",
            (unsigned)got);
    }

    // Return to normal mode 0
    _setMode(0);
    return (got >= 3 && resp[0] == 0xC1);
}

// ----------------------------------------------------------------------------
void LoRaTx::begin() {
    pinMode(LORA_M0_PIN, OUTPUT);
    pinMode(LORA_M1_PIN, OUTPUT);
    pinMode(LORA_AUX_PIN, INPUT_PULLUP);

    // Always start in mode 0 (sane default; if user disables LoRa later
    // we just stop sending, the module stays idle).
    digitalWrite(LORA_M0_PIN, LOW);
    digitalWrite(LORA_M1_PIN, LOW);

    loraSerial.begin(9600, SERIAL_8N1, LORA_RX_PIN, LORA_TX_PIN);

    if (!settings.loraEnabled()) {
        _state = State::Disabled;
        DBG("LORA", "transmitter disabled (toggle off in settings)");
        return;
    }

    DBG("LORA", "init: M0=GPIO%d M1=GPIO%d AUX=GPIO%d RX=GPIO%d TX=GPIO%d",
        LORA_M0_PIN, LORA_M1_PIN, LORA_AUX_PIN, LORA_RX_PIN, LORA_TX_PIN);

    if (_writeConfig()) {
        _state = State::Idle;
    } else {
        _state = State::InitFailed;
        Serial.println();
        Serial.println(F("[ LORA  ] ============================================"));
        Serial.println(F("[ LORA  ]   !! E220 NOT RESPONDING !!"));
        Serial.println(F("[ LORA  ] --------------------------------------------"));
        Serial.println(F("[ LORA  ] Wiring checklist:"));
        Serial.printf ( "[ LORA  ]   - VCC -> 3.3V or 5V (verify breakout LDO)\n");
        Serial.printf ( "[ LORA  ]   - GND -> GND\n");
        Serial.printf ( "[ LORA  ]   - M0  -> GPIO%d\n", LORA_M0_PIN);
        Serial.printf ( "[ LORA  ]   - M1  -> GPIO%d\n", LORA_M1_PIN);
        Serial.printf ( "[ LORA  ]   - AUX -> GPIO%d  (4k7 pull-up to 3.3V)\n", LORA_AUX_PIN);
        Serial.printf ( "[ LORA  ]   - TXD -> GPIO%d  (E220 TX -> ESP32 RX)\n", LORA_RX_PIN);
        Serial.printf ( "[ LORA  ]   - RXD -> GPIO%d  (E220 RX <- ESP32 TX)\n", LORA_TX_PIN);
        Serial.println(F("[ LORA  ]   - Antenna connected (do NOT power on without one!)"));
        Serial.println(F("[ LORA  ] ============================================"));
    }
}

// ----------------------------------------------------------------------------
bool LoRaTx::applySettings() {
    if (!settings.loraEnabled()) {
        _state = State::Disabled;
        DBG("LORA", "disabled by settings update");
        return true;
    }
    DBG("LORA", "applying settings...");
    _state = State::Configuring;
    bool ok = _writeConfig();
    _state = ok ? State::Idle : State::InitFailed;
    return ok;
}

// ----------------------------------------------------------------------------
//  Build and transmit one frame. Always serialised (one in-flight at a time);
//  callers should be OK with a few-ms blocking delay -- it's faster than a
//  TCP round-trip and we expect to be sending only once per sample.
// ----------------------------------------------------------------------------
bool LoRaTx::_sendFrame(FrameType t, const uint8_t* payload, size_t len) {
    if (_state != State::Idle) return false;
    if (len > 240) len = 240;

    uint8_t frame[6 + 240 + 2];
    frame[0] = 0xA5;                              // MAGIC
    frame[1] = LORA_PROTOCOL_VERSION;
    frame[2] = (uint8_t)t;
    frame[3] = (uint8_t)len;
    frame[4] = (uint8_t)(_seq & 0xFF);
    frame[5] = (uint8_t)((_seq >> 8) & 0xFF);
    if (len > 0 && payload != nullptr) memcpy(&frame[6], payload, len);
    uint16_t crc = _crc16(frame, 6 + len);
    frame[6 + len + 0] = (uint8_t)(crc & 0xFF);
    frame[6 + len + 1] = (uint8_t)((crc >> 8) & 0xFF);

    size_t total = 6 + len + 2;
    _state = State::Transmitting;

    // Wait for AUX high before pushing bytes (module must be ready)
    if (!_waitAux(500)) {
        _txFailures++;
        _state = State::Idle;
        DBG("LORA", "TX abort: AUX never went high");
        return false;
    }

    loraSerial.write(frame, total);
    loraSerial.flush();

    // Wait for module to finish radiating. AUX pulse can be quite long
    // for big payloads at 2.4kbps air rate -- ~250ms for 80 bytes.  Allow
    // up to 5 seconds to be safe.
    bool sent = _waitAux(5000);
    _state = State::Idle;

    if (sent) {
        _seq++;
        _txCount++;
        _txBytes += total;
        _lastTxMs = millis();
        Serial.printf("[ LORA  ] TX type=0x%02X seq=%u len=%u (%u bytes total)\n",
                      (unsigned)t, (unsigned)((_seq - 1) & 0xFFFF),
                      (unsigned)len, (unsigned)total);
        return true;
    } else {
        _txFailures++;
        DBG("LORA", "TX timeout (AUX did not go high after %u bytes)",
            (unsigned)total);
        return false;
    }
}

// ----------------------------------------------------------------------------
//  Public send helpers -- all build a small ASCII payload then call
//  _sendFrame with the appropriate FrameType.
// ----------------------------------------------------------------------------
bool LoRaTx::sendCsvRow(const String& csvLine) {
    if (_state != State::Idle) return false;
    if (csvLine.length() > 240) {
        // Should never happen with our 11-column schema, but be safe.
        DBG("LORA", "TX skipped: CSV row too long (%u bytes)", csvLine.length());
        return false;
    }
    return _sendFrame(FrameType::CsvRow,
                      (const uint8_t*)csvLine.c_str(),
                      csvLine.length());
}

bool LoRaTx::sendAlertFall(float mag, uint32_t durationMs, const String& iso) {
    char buf[80];
    int n = snprintf(buf, sizeof(buf), "FALL,%.3f,%lu,%s",
                     (double)mag, (unsigned long)durationMs, iso.c_str());
    if (n < 0) return false;
    return _sendFrame(FrameType::AlertFall, (uint8_t*)buf, (size_t)n);
}

bool LoRaTx::sendAlertImpact(float mag, const String& iso) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "IMPACT,%.3f,%s",
                     (double)mag, iso.c_str());
    if (n < 0) return false;
    return _sendFrame(FrameType::AlertImpact, (uint8_t*)buf, (size_t)n);
}

bool LoRaTx::sendAlertSdFull(float usedPct, const String& iso) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "SDFULL,%.1f,%s",
                     (double)usedPct, iso.c_str());
    if (n < 0) return false;
    return _sendFrame(FrameType::AlertSdFull, (uint8_t*)buf, (size_t)n);
}

bool LoRaTx::sendAlertSensorLost(const char* which, const String& iso) {
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "LOST,%s,%s",
                     which ? which : "?", iso.c_str());
    if (n < 0) return false;
    return _sendFrame(FrameType::AlertSensorLost, (uint8_t*)buf, (size_t)n);
}

// ----------------------------------------------------------------------------
//  Heartbeat -- emitted every 60 seconds when idle, so the receiver can
//  distinguish "transmitter alive but no new data" from "transmitter
//  fell off the network".
// ----------------------------------------------------------------------------
void LoRaTx::_heartbeatTick() {
    if (_state != State::Idle) return;
    if (millis() - _lastHeartbeatMs < 60000UL) return;
    _lastHeartbeatMs = millis();

    char buf[80];
    int n = snprintf(buf, sizeof(buf), "HB,%lu,0,%lu,%d",
                     (unsigned long)(millis() / 1000),
                     (unsigned long)ESP.getFreeHeap(),
                     (int)(battery.percent() + 0.5f));
    if (n < 0) return;
    _sendFrame(FrameType::Heartbeat, (uint8_t*)buf, (size_t)n);
}

// ----------------------------------------------------------------------------
void LoRaTx::loop() {
    if (_state == State::Disabled || _state == State::InitFailed) return;
    _heartbeatTick();
}

// ----------------------------------------------------------------------------
void LoRaTx::printStats() const {
    const char* st = "?";
    switch (_state) {
        case State::Disabled:     st = "OFF";    break;
        case State::InitFailed:   st = "INIT!!"; break;
        case State::Idle:         st = "ok";     break;
        case State::Configuring:  st = "config"; break;
        case State::Transmitting: st = "TX";     break;
    }
    if (_state == State::Disabled) {
        Serial.println(F("[ LORA  ] OFF (toggle off in settings)"));
        return;
    }
    Serial.printf("[ LORA  ] %-6s ch=%u (%.3fMHz)  pwr=%u  enc=%s  "
                  "tx=%lu fail=%lu bytes=%lu\n",
                  st,
                  (unsigned)settings.loraChannel(),
                  (double)channelToMhz(settings.loraChannel()),
                  (unsigned)settings.loraTxPower(),
                  settings.loraEncrypt() ? "on" : "off",
                  (unsigned long)_txCount,
                  (unsigned long)_txFailures,
                  (unsigned long)_txBytes);
}
