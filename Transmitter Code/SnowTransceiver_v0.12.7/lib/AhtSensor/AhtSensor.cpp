// ============================================================================
//  AhtSensor.cpp  --  AHT10 I2C driver using Wire directly (no library dep)
// ============================================================================
#include "AhtSensor.h"
#include <Wire.h>

AhtSensor aht;

// AHT10 command bytes
static constexpr uint8_t CMD_INIT[3]    = { 0xBE, 0x08, 0x00 };
static constexpr uint8_t CMD_TRIGGER[3] = { 0xAC, 0x33, 0x00 };

// ----------------------------------------------------------------------------
bool AhtSensor::_initChip() {
    // Check if the chip is on the bus at all
    Wire.beginTransmission(AHT10_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        DBG("AHT", "not found at I2C addr 0x%02X", AHT10_I2C_ADDR);
        return false;
    }

    // Send init / calibration command
    Wire.beginTransmission(AHT10_I2C_ADDR);
    Wire.write(CMD_INIT, 3);
    if (Wire.endTransmission() != 0) {
        DBG("AHT", "init command failed");
        return false;
    }
    delay(10);   // datasheet: wait >= 10 ms after init

    DBG("AHT", "AHT10 found at 0x%02X on SDA=GPIO%d SCL=GPIO%d",
        AHT10_I2C_ADDR, IMU_SDA_PIN, IMU_SCL_PIN);
    return true;
}

// ----------------------------------------------------------------------------
bool AhtSensor::_triggerAndRead() {
    // 1. Trigger measurement
    Wire.beginTransmission(AHT10_I2C_ADDR);
    Wire.write(CMD_TRIGGER, 3);
    if (Wire.endTransmission() != 0) return false;

    // 2. Wait for conversion (datasheet: ~75-80 ms for normal mode)
    delay(85);

    // 3. Read 6 bytes
    uint8_t buf[6];
    uint8_t got = Wire.requestFrom((uint8_t)AHT10_I2C_ADDR, (uint8_t)6);
    if (got < 6) return false;
    for (uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();

    // Bit 7 of byte 0 = busy flag; bit 3 = calibrated flag
    if (buf[0] & 0x80) return false;   // still busy

    // 4. Extract 20-bit humidity and temperature fields
    uint32_t hum_raw  = ((uint32_t)buf[1] << 12)
                      | ((uint32_t)buf[2] << 4)
                      | ((buf[3] >> 4) & 0x0F);

    uint32_t temp_raw = ((uint32_t)(buf[3] & 0x0F) << 16)
                      | ((uint32_t)buf[4] << 8)
                      | buf[5];

    // 5. Convert
    float h = (float)hum_raw  / 1048576.0f * 100.0f;
    float t = (float)temp_raw / 1048576.0f * 200.0f - 50.0f;

    // 6. Sanity clamp
    if (h < AHT10_HUM_MIN_PCT || h > AHT10_HUM_MAX_PCT) return false;
    if (t < AHT10_TEMP_MIN_C  || t > AHT10_TEMP_MAX_C)  return false;

    _humPct = h;
    _tempC  = t;
    return true;
}

// ----------------------------------------------------------------------------
void AhtSensor::begin() {
    // Wire.begin() was already called by the IMU module — no need to repeat.
    // If for any reason it was not called, call it now:
    // Wire.begin(IMU_SDA_PIN, IMU_SCL_PIN);

    if (_initChip()) {
        // Take one measurement immediately so the dashboard isn't blank
        if (_triggerAndRead()) {
            _lastGoodMs = millis();
            _ready = true;
            Serial.printf("[ AHT   ] ready  %.1f°C  %.1f%%rH\n",
                          _tempC, _humPct);
        } else {
            DBG("AHT", "init read failed — will retry in loop()");
            _ready = true;   // chip present, just first read failed
        }
    } else {
        _ready = false;
        Serial.println(F("[ AHT   ] !! AHT10 NOT DETECTED !!"));
        Serial.println(F("[ AHT   ] Wiring:  VIN->3.3V  GND->GND"));
        Serial.printf ("[ AHT   ]          SDA->GPIO%d  SCL->GPIO%d\n",
                       IMU_SDA_PIN, IMU_SCL_PIN);
        Serial.println(F("[ AHT   ] I2C address: 0x38"));
        Serial.println(F("[ AHT   ] (shares bus with BMI160 at 0x69, no conflict)"));
    }

    _lastReadMs = millis();
}

// ----------------------------------------------------------------------------
void AhtSensor::loop() {
    if (!_ready) return;
    if (millis() - _lastReadMs < AHT10_READ_INTERVAL_MS) return;
    _lastReadMs = millis();

    if (_triggerAndRead()) {
        _lastGoodMs = millis();
        _errCount = 0;
    } else {
        _errCount++;
        // After 5 consecutive failures, mark as stale but keep trying
        if (_errCount % 10 == 0) {
            DBG("AHT", "consecutive errors: %lu", (unsigned long)_errCount);
        }
    }
}

// ----------------------------------------------------------------------------
void AhtSensor::printStats() const {
    if (!_ready) {
        Serial.println(F("[ AHT   ] NOT DETECTED  (check wiring, addr 0x38)"));
        return;
    }
    if (!isFresh()) {
        Serial.printf("[ AHT   ] STALE  errs=%lu  (last good: %lums ago)\n",
                      (unsigned long)_errCount,
                      (unsigned long)(millis() - _lastGoodMs));
        return;
    }
    Serial.printf("[ AHT   ] ok  temp=%.2f°C  hum=%.1f%%rH  errs=%lu\n",
                  (double)_tempC, (double)_humPct, (unsigned long)_errCount);
}
