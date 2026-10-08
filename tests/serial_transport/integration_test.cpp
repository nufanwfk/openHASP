#include <cassert>
#include "../../src/hal/esp32/hasp_uart_esp32.cpp"
#include "../../src/custom/uart_transport.cpp"
#include "hasp_filesystem.h"
using namespace hasp_uart::hal;
std::string configText;
std::vector<std::string> commands;
HardwareSerial Serial, Serial1, Serial2;
FS testFS;
Device haspDevice;
TFT haspTft;
GpioConfig configs[2];
void dispatch_text_line(const char* line, uint8_t) { commands.push_back(line); }
void tick(unsigned n=100) { while(n--) custom_loop(); }
void configure(const std::string& cfg) { configText=cfg; custom_setup(); }
int main() {
    configure(""); assert(!uart);
    configure("{broken"); assert(!uart);
    configure("{\"enabled\":false}"); assert(!uart);
    configure("{\"enabled\":true}"); assert(!uart);
    for(int pin : {-1,2,8,9,10,11,19,20,27,43,44,49}) {
        configure("{\"enabled\":true,\"uart\":1,\"rx\":"+std::to_string(pin)+",\"tx\":18,\"baud\":115200}");
        assert(!uart);
    }
    for(int port : {0,3,-1}) {
        configure("{\"enabled\":true,\"uart\":"+std::to_string(port)+",\"rx\":17,\"tx\":18,\"baud\":115200}");
        assert(!uart);
    }
    configs[0].type = SERIAL_DIMMER;
    const char* valid = "{\"enabled\":true,\"uart\":1,\"rx\":17,\"tx\":18,\"baud\":115200}";
    configure(valid); assert(!uart);
    configs[0].type = 0;
    custom_uart_layout_loaded(); // normal boot order: layout before custom_setup
    configure(valid); assert(uart == &Serial1);
    assert(Serial1.baud == 115200 && Serial1.rx == 17 && Serial1.tx == 18);
    assert(custom_pin_in_use(17) && custom_pin_in_use(18) && !custom_pin_in_use(16));
    custom_state_subtopic("page", "2"); // startup events suppressed
    tick(); assert(Serial1.output == "\nready 1\n");
    Serial1.output.clear();
    for(char c : std::string("page 2\njsonl {\"page\":1}\r\n")) Serial1.input.push_back(c);
    tick(); assert(commands == (std::vector<std::string>{"page 2","jsonl {\"page\":1}"}));
    custom_state_subtopic("p1b48", "{\"event\":\"down\"}");
    custom_state_subtopic("p1b48", "{\"event\":\"release\"}");
    tick(); assert(Serial1.output == "event p1b48 down\nevent p1b48 release\n");
    Serial1.output.clear();
    custom_state_subtopic("p1b49", "{\"tag\":{\"event\":\"up\"}}");
    custom_state_subtopic("p1b49", "{\"event\":true}");
    custom_state_subtopic("p1b49", "{\"event\":\"invalid\"}");
    custom_state_subtopic("statusupdate", "{\"uptime\":10}");
    custom_state_subtopic("page", "2junk");
    custom_state_subtopic("page", "256");
    custom_state_subtopic("page", "main");
    custom_state_subtopic("p1b49", "{\"tag\":{\"some\":\"metadata\"},\"event\":\"lost\"}");
    tick(); assert(Serial1.output == "event p1b49 lost\n");
    Serial1.output.clear();
    Serial1.room = 0;
    custom_state_subtopic("page", "2"); tick(); assert(Serial1.output.empty());
    custom_uart_layout_loaded();
    custom_state_subtopic("page", "1");
    Serial1.room = 7; tick(); assert(Serial1.output == "\nready 1\npage 1\n");
    Serial1.output.clear();
    custom_topic_payload("serialready", "1", 0); tick(); assert(Serial1.output == "\nready 1\n");
    StaticJsonDocument<256> doc;
    custom_get_sensors(doc); assert(doc["serial"]["dropped"].as<int>() == 0);
    vSemaphoreDelete(txMutex);
    puts("PASS: actual UART extension, configuration, pin conflicts, startup, events, reload and dispatch");
}
