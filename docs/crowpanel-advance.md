# Experimental CrowPanel Advance 5-inch DIS02050A profile

**Not hardware-validated; no complete firmware build yet. Do not treat this as
an established supported-board release. Confirm the physical PCB revision first.**

No matching Advance/DIS02050A profile was found in release 0.7.0-rc13 or inspected
upstream master `7bc826e8`. The older Elecrow WZ8048C050 environment has different
RGB/touch/backlight wiring and a 4 MB target. It is not a substitute.

## Evidence and scope

This draft targets the vendor's **V1.2_and_V1.3** Arduino example: ESP32-S3,
16 MB flash / 8 MB OPI PSRAM assumption (verify module marking), 800 × 480 RGB,
GT911 touch and an STC controller at I2C address 0x30. Earlier revisions are
outside its scope. No vendor source is copied into the repository; the small
helper independently implements the published initialization transactions.

Pinned vendor sources (retrieved 2026-09-27):

- [RGB/touch setup](https://github.com/Elecrow-RD/CrowPanel-Advance-5-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480/blob/19b0f58cdb046493238f1dbf2e04215cf8d6bdec/example/V1.2_and_V1.3/Arduino/lesson-03/BigInch_LVGL/LovyanGFX_Driver.h)
- [Board initialization](https://github.com/Elecrow-RD/CrowPanel-Advance-5-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480/blob/19b0f58cdb046493238f1dbf2e04215cf8d6bdec/example/V1.2_and_V1.3/Arduino/lesson-03/BigInch_LVGL/BigInch_LVGL.ino)
- [V1.3 schematic](https://github.com/Elecrow-RD/CrowPanel-Advance-5-HMI-ESP32-S3-AI-Powered-IPS-Touch-Screen-800x480/blob/19b0f58cdb046493238f1dbf2e04215cf8d6bdec/Eagle_SCH%26PCB/Version%201.3/ESP32%20Display%205.0%20inch%20V1.3.pdf)

| Interface | Draft configuration |
| --- | --- |
| RGB B0..B4 | 21,47,48,45,38 |
| RGB G0..G5 | 9,10,11,12,13,14 |
| RGB R0..R4 | 7,17,18,3,46 |
| DE / VSYNC / HSYNC / PCLK | 42 / 41 / 40 / 39 |
| Pixel clock / H and V timing | 16 MHz; front 8, pulse 4, back 8 |
| Shared I2C | SDA15, SCL16, 400 kHz |
| GT911 | 0x5D, polled; no interrupt/reset pin assigned in driver |
| STC board controller | 0x30; brightness 0=full, 245=off |
| Touch recovery | command 250, pulse GPIO1 low then input; at most 5 attempts |
| Serial console | UART0/USB-UART bridge; native USB CDC disabled |
| Expansion serial | UART1 pins selected in serial.json after routing verification |

## Why more than an INI file is needed

`user_setups/esp32s3/crowpanel-advance.ini` selects the RGB wiring, touch address,
memory and display parameters. `src/hal/boards/crowpanel_advance.*` isolates the
board controller from the generic serial HAL and message parser. Small guarded
hooks initialize it before the display, route backlight changes through I2C and
reserve the RGB/I2C/recovery pins against GPIO/UART reuse. The RGB driver branch
also honors the profile's 0x5D address instead of its existing hardcoded 0x14.
Other boards retain their current behavior when the macro is absent.

Missing STC stops the helper before it pulses GPIO1. It does not prove a board is
compatible merely because an I2C device responds. Touch failure is logged and
bounded; it does not hang startup. Brightness never emits the STC's special
246..255 commands. The helper uses the existing shared Wire bus; coexistence
with LovyanGFX's I2C access still needs stress testing on the panel.

GPIO19/20 are potential expansion UART pins, also associated with native USB and
other routed peripherals. Confirm the revision's schematic, header labels and
DIP routing. This document intentionally does not prescribe a switch position
or final RX/TX assignment before the physical board is identified.

## Reproduce configuration checks

Use the normal local `platformio_override.ini` convention:

```ini
[override]
build_flags =
    -D HASP_USE_CUSTOM=1
    -D HASP_USE_UART_TRANSPORT=1
extra_default_envs =
    crowpanel-advance-5-v12-v13_16MB
[platformio]
extra_configs =
    user_setups/esp32s3/crowpanel-advance.ini
```

The UART flags are optional for board support itself. `pio project config`
resolves this environment; `pio run -e crowpanel-advance-5-v12-v13_16MB` is the
remaining full-build gate, not a build claimed to have passed.

## Validation status

- PlatformIO configuration resolves the environment and 16 MB partition table.
- `python3 tests/crowpanel_advance/run_tests.py` passes against the actual helper:
  all 256 brightness levels, off, absent controller, bounded missing-touch
  recovery, repeated begin and representative reserved/unreserved pins.
- Helper compiles to an ESP32-S3 object with the installed Arduino SDK; openHASP
  application/logging interfaces were stubbed. This is not a firmware link test.
- Existing generic UART host tests pass, including the real H5 parser contract.
- A prior full panel build was blocked by a toolchain download rejected by this
  execution environment. Full firmware build, existing-board regression build,
  physical display, touch, brightness, pin routing and serial tests remain open.
- No panel has been flashed. Follow the NanoELS hardware test plan after the
  owner identifies the PCB and a complete firmware build becomes available.
