#pragma once
#define HASP_NUM_GPIO_CONFIG 2
#define SERIAL_DIMMER 48
#define SERIAL_DIMMER_L8_HD 49
#define SERIAL_DIMMER_L8_HD_INVERTED 50
struct GpioConfig { int type = 0; };
extern GpioConfig configs[2];
inline GpioConfig gpioGetPinConfig(int i) { return configs[i]; }
inline bool gpioInUse(int p) { return p == 11; }
