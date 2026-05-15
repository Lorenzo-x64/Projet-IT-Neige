// ============================================================================
//  Ultrasonic.h  --  DFRobot SEN0313 / A01NYUB driver
//
//  The sensor outputs a 4-byte UART frame at 9600 8N1:
//      [0xFF] [DATA_H] [DATA_L] [SUM]   where SUM = (0xFF + H + L) & 0xFF
//  Distance (mm) = (DATA_H << 8) | DATA_L
//
//  This module is built on top of HardwareSerial (UART2) with the RX pin
//  remapped via the ESP32 GPIO matrix.  HardwareSerial has a real FIFO and
//  is unaffected by WiFi/AsyncTCP interrupt timing -- the reason we moved
//  away from SoftwareSerial, which silently drops bytes when the web server
//  is active.
// ============================================================================
#pragma once

#include <Arduino.h>

class Ultrasonic {
public:
    void  begin();                         // call once in setup()
    void  loop();                          // call as often as possible
    void  printStats() const;              // dump one line to Serial

    bool  isFresh(uint32_t maxAgeMs = 1500) const;
    // distanceCm() returns the temperature-corrected distance when the
    // appropriate AppSettings flag is set AND the AHT temperature reading
    // is fresh; otherwise it returns the raw sensor reading. The raw value
    // is available separately via rawDistanceCm() for diagnostics.
    float distanceCm() const;
    float rawDistanceCm() const { return _distCm; }
    uint32_t lastReadMs() const { return _lastReadMs; }

    // Statistics (handy for the dashboard)
    uint32_t totalFrames()     const { return _totalFrames; }
    uint32_t checksumErrors()  const { return _checksumErrors; }
    uint32_t outOfRangeReads() const { return _outOfRange; }

private:
    float    _distCm           = -1.0f;
    uint32_t _lastReadMs       = 0;
    uint32_t _totalFrames      = 0;
    uint32_t _checksumErrors   = 0;
    uint32_t _outOfRange       = 0;

    // 4-byte frame parser state
    uint8_t  _buf[4]   = {0};
    uint8_t  _idx      = 0;
};

extern Ultrasonic ultrasonic;
