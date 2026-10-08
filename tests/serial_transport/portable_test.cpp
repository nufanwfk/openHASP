// Same common extension on a platform with no ESP32, HardwareSerial or RTOS APIs.
#include <cassert>
#include <deque>
#include "../../src/custom/uart_transport.cpp"
#include "hasp_filesystem.h"
#include "../../src/hal/hasp_uart_unsupported.cpp"
std::string configText;
std::vector<std::string> commands;
FS testFS;
void dispatch_text_line(const char* text, uint8_t) { commands.push_back(text); }
#ifdef TEST_UART_EXTERNAL_HAL
namespace hasp_uart { namespace hal {
bool running = false, locked = false;
std::deque<uint8_t> incoming;
std::string outgoing;
bool begin(const Config& c) {
    assert(c.port == 4 && c.rx == 101 && c.tx == 102 && c.baud == 115200);
    running = true; return true;
}
bool active() { return running; }
int read() { if(incoming.empty()) return -1; int c=incoming.front();incoming.pop_front();return c; }
size_t writeCapacity() { return 8; }
size_t write(const uint8_t* p, size_t n) {
    assert(locked && n <= 8);
    // Deliberate short writes: common code must retain the remainder.
    if(n > 2) n=2;
    outgoing.append(reinterpret_cast<const char*>(p),n);return n;
}
bool pinInUse(uint8_t p) { return running && (p == 101 || p == 102); }
void lock() { assert(!locked); locked=true; }
void unlock() { assert(locked); locked=false; }
} }
#endif
int main() {
    configText = "{\"enabled\":true,\"uart\":4,\"rx\":101,\"tx\":102,\"baud\":115200}";
    custom_uart_layout_loaded();
    custom_setup();
#ifdef TEST_UART_EXTERNAL_HAL
    using namespace hasp_uart::hal;
    assert(active() && custom_pin_in_use(101));
    for(char c : std::string("page 2\n")) incoming.push_back(c);
    for(int i=0;i<20;++i) custom_loop();
    assert(commands == std::vector<std::string>{"page 2"});
    assert(outgoing == "\nready 1\n");
    custom_state_subtopic("p1b48", "{\"event\":\"down\"}");
    for(int i=0;i<20;++i) custom_loop();
    assert(outgoing == "\nready 1\nevent p1b48 down\n");
    puts("PASS: common transport on independent HAL, non-ESP32 pins/port and short writes");
#else
    assert(!hasp_uart::hal::active());
    custom_loop();
    custom_state_subtopic("p1b48", "{\"event\":\"down\"}");
    assert(commands.empty());
    puts("PASS: unsupported hardware remains disabled without ESP32 dependencies");
#endif
}
