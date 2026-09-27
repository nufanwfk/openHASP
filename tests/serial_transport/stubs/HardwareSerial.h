#pragma once
#include "hasplib.h"
#define SERIAL_8N1 0
struct HardwareSerial {
    bool active = false;
    int room = 3, rx = -1, tx = -1;
    uint32_t baud = 0;
    std::deque<uint8_t> input;
    std::string output;
    void setRxBufferSize(size_t) {}
    void begin(uint32_t b, int, int r, int t) { active = true; baud = b; rx = r; tx = t; }
    explicit operator bool() const { return active; }
    int available() { return input.size(); }
    int read() { if(input.empty()) return -1; int c = input.front(); input.pop_front(); return c; }
    int availableForWrite() { return room; }
    size_t write(const uint8_t* p, size_t n) { assert(n <= static_cast<size_t>(room)); output.append((const char*)p,n); return n; }
};
extern HardwareSerial Serial, Serial1, Serial2;
