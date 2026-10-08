#pragma once
struct Device { bool is_system_pin(int p) { return p >= 26 && p <= 37; } };
extern Device haspDevice;
