// ============================================================================
//  SnowEngine.cpp
// ============================================================================
#include "SnowEngine.h"
#include "Ultrasonic.h"
#include "AppSettings.h"
#include "Config.h"
#include <math.h>

SnowEngine snow;

void SnowEngine::begin() {
    DBG("SNOW", "Engine ready (initial tare = %.1f cm)", settings.tareCm());
}

float SnowEngine::snowDepthCm() const {
    if (!ultrasonic.isFresh()) return -1.0f;
    float d = settings.tareCm() - ultrasonic.distanceCm();
    if (d < 0) d = 0;
    return d;
}

float SnowEngine::tareNow() {
    if (!ultrasonic.isFresh()) {
        DBG("SNOW", "Tare aborted: no fresh sensor reading");
        return NAN;
    }
    float v = ultrasonic.distanceCm();
    settings.setTareCm(v);
    DBG("SNOW", "Tare captured: %.1f cm", v);
    return v;
}

void SnowEngine::setManualTare(float cm) {
    settings.setTareCm(cm);
    DBG("SNOW", "Tare manually set: %.1f cm", cm);
}

void SnowEngine::resetTare() {
    settings.setTareCm(0.0f);
    DBG("SNOW", "Tare reset to 0");
}

float SnowEngine::tareCm() const {
    return settings.tareCm();
}
