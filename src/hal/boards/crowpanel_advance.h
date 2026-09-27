// MIT License. Experimental DIS02050A V1.2/V1.3 board-control interface.
#pragma once
#include <stdint.h>
bool crowpanelAdvanceBegin();
void crowpanelAdvanceBacklight(uint8_t level, bool power);
bool crowpanelAdvancePinInUse(uint8_t pin);
