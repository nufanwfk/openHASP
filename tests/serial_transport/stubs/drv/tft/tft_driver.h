#pragma once
struct TFT { bool is_driver_pin(int p) { return p == 10; } };
extern TFT haspTft;
