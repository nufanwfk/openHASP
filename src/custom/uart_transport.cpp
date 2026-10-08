// MIT License. openHASP custom hooks; byte I/O is supplied by hal/hasp_uart.h.
#include "hasplib.h"
#if defined(HASP_USE_UART_TRANSPORT) && HASP_USE_UART_TRANSPORT > 0
#if !defined(HASP_USE_CUSTOM) || HASP_USE_CUSTOM != 1
#error "UART transport requires HASP_USE_CUSTOM=1"
#endif
#include "uart_messages.h"
#include "hal/hasp_uart.h"
#include "hasp_debug.h"
#if HASP_USE_SPIFFS > 0 || HASP_USE_LITTLEFS > 0
#include "hasp_filesystem.h"
#endif
namespace {
hasp_uart::Receiver receiver;
hasp_uart::Outbox outbox;
bool layoutLoaded = false;
bool started = false;
}

void custom_setup() {
#if HASP_USE_SPIFFS > 0 || HASP_USE_LITTLEFS > 0
    File file = HASP_FS.open("/serial.json", "r");
    if(!file) return; // Missing configuration means disabled.
    StaticJsonDocument<512> config;
    auto error = deserializeJson(config, file);
    file.close();
    if(error || !config["enabled"].is<bool>()) {
        LOG_ERROR(TAG_CONF, F("Invalid serial.json; UART transport disabled"));
        return;
    }
    if(!config["enabled"].as<bool>()) return;
    if(!config["uart"].is<int>() || !config["rx"].is<int>() || !config["tx"].is<int>() ||
       !config["baud"].is<uint32_t>()) {
        LOG_ERROR(TAG_CONF, F("serial.json requires uart, rx, tx and baud integers"));
        return;
    }
    hasp_uart::hal::Config settings;
    settings.port = config["uart"];
    settings.rx = config["rx"];
    settings.tx = config["tx"];
    settings.baud = config["baud"];
    hasp_uart::hal::begin(settings);
#endif
}

void custom_uart_layout_loaded() {
    hasp_uart::hal::lock(); // Safe before begin: boot-time layout loading is single-threaded.
    layoutLoaded = true;
    if(started) outbox.ready();
    hasp_uart::hal::unlock();
}

void custom_loop() {
    using namespace hasp_uart;
    if(!hal::active()) return;
    hal::lock();
    bool loaded = layoutLoaded;
    if(loaded && !started) {
        outbox.ready();
        started = true;
    }
    hal::unlock();
    if(!loaded) return;
    // This hook runs in openHASP's main/GUI context. The HAL never dispatches.
    for(unsigned budget = 0; budget < 256; ++budget) {
        int value = hal::read();
        if(value < 0) break;
        if(receiver.feed(static_cast<uint8_t>(value))) dispatch_text_line(receiver.line(), TAG_MSGR);
    }
    hal::lock();
    size_t room = hal::writeCapacity();
    size_t n = outbox.size();
    if(n > room) n = room;
    if(n > 128) n = 128;
    if(n) outbox.consume(hal::write(reinterpret_cast<const uint8_t*>(outbox.data()), n));
    hal::unlock();
}

void custom_state_subtopic(const char* subtopic, const char* payload) {
    if(!hasp_uart::hal::active()) return;
    hasp_uart::hal::lock();
    if(started) hasp_uart::forwardState(outbox, subtopic, payload);
    hasp_uart::hal::unlock();
}

bool custom_pin_in_use(uint8_t pin) { return hasp_uart::hal::pinInUse(pin); }
void custom_get_sensors(JsonDocument& doc) {
    if(!hasp_uart::hal::active()) return;
    hasp_uart::hal::lock();
    uint32_t dropped = outbox.dropped;
    hasp_uart::hal::unlock();
    doc["serial"]["dropped"] = dropped;
}
void custom_every_second() {}
void custom_every_5seconds() {}
void custom_topic_payload(const char* topic, const char* payload, uint8_t source) {
    (void)source;
    if(!strcmp(topic, "serialready") && !strcmp(payload, "1")) {
        hasp_uart::hal::lock();
        if(layoutLoaded && started) outbox.ready();
        hasp_uart::hal::unlock();
    }
}
#endif
