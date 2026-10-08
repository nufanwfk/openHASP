#pragma once
#include <cassert>
using SemaphoreHandle_t = bool*;
#define portMAX_DELAY 0
inline SemaphoreHandle_t xSemaphoreCreateMutex() { return new bool(false); }
inline void xSemaphoreTake(bool* p, int) { assert(!*p); *p = true; }
inline void xSemaphoreGive(bool* p) { assert(*p); *p = false; }
inline void vSemaphoreDelete(bool* p) { delete p; }
