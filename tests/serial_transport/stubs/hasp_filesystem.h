#pragma once
#include "hasplib.h"
extern std::string configText;
struct File {
    size_t pos = 0;
    explicit operator bool() const { return !configText.empty(); }
    int read() { return pos < configText.size() ? (unsigned char)configText[pos++] : -1; }
    size_t readBytes(char* p, size_t n) { size_t i=0; while(i<n && pos<configText.size()) p[i++]=configText[pos++]; return i; }
    void close() {}
};
struct FS { File open(const char*,const char*) { return File(); } };
extern FS testFS;
#define HASP_FS testFS
