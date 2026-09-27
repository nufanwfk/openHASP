// MIT License. Independently implemented from Elecrow's published board examples.
#include "hasplib.h"
#if defined(ESP32) && defined(HASP_CROWPANEL_ADVANCE_STC)
#include "crowpanel_advance.h"
#include "Wire.h"
#include "hasp_debug.h"
namespace {
bool initialized = false;
bool controllerPresent = false;
bool touchReady = false;
bool probe(uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0;
}
bool command(uint8_t value) {
    Wire.beginTransmission(0x30);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}
}
bool crowpanelAdvanceBegin() {
    if(initialized) return touchReady;
    initialized = true;
    Wire.begin(15, 16, 400000);
    Wire.setTimeOut(50);
    delay(50);
    // Fail closed on a different board/revision, rather than pulse an unknown pin.
    controllerPresent = probe(0x30);
    if(!controllerPresent) {
        LOG_ERROR(TAG_GUI, F("CrowPanel STC at 0x30 not found; verify PCB revision"));
        return false;
    }
    bool touch = probe(0x5D);
    for(unsigned attempt = 0; !touch && attempt < 5; ++attempt) {
        if(!command(250)) break; // Vendor touch activation command, not brightness.
        pinMode(1, OUTPUT);
        digitalWrite(1, LOW);
        delay(120);
        pinMode(1, INPUT);
        delay(100);
        touch = probe(0x5D);
    }
    if(!touch) LOG_ERROR(TAG_GUI, F("CrowPanel GT911 at 0x5D not found after bounded recovery"));
    command(0); // Initial full brightness; openHASP applies saved brightness later.
    touchReady = touch;
    return touchReady;
}
void crowpanelAdvanceBacklight(uint8_t level, bool power) {
    if(!initialized || !controllerPresent) return;
    // Vendor: 0 = maximum light, 245 = off. Never emit special 246..255 commands.
    uint8_t value = power ? 245u - (uint32_t(level) * 245u / 255u) : 245u;
    if(!command(value)) LOG_ERROR(TAG_GUI, F("CrowPanel backlight I2C write failed"));
}
bool crowpanelAdvancePinInUse(uint8_t pin) {
    // RGB bus, shared I2C and touch recovery line. Prevent reconfiguration as GPIO/UART.
    const uint8_t pins[] = {1,3,7,9,10,11,12,13,14,15,16,17,18,21,38,39,40,41,42,45,46,47,48};
    for(uint8_t used : pins) if(pin == used) return true;
    return false;
}
#endif
