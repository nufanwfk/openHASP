# Generic UART transport extension

Based on upstream **0.7.0-rc13**, commit `042fe057`, the release GitHub marked
latest when checked on 2026-09-27 UTC. This extension is optional firmware
code, with an ESP32 hardware implementation, not a dynamically installed plugin. It uses openHASP's existing custom
setup, loop, state, pin-reservation and sensor hooks. The only functional core
change is a guarded layout-load notification in `src/hasp/hasp_page.cpp`.

No NanoELS actions, Nextion emulation, widget mappings, or display layouts are
included here. The matching H5 implementation lives in
[nufanwfk/nanoels, codex/h5-openhasp-adapter](https://github.com/nufanwfk/nanoels/tree/codex/h5-openhasp-adapter).
Use its **version 2 token event contract**; the earlier H5 JSON-event build is
incompatible. MQTT retains its original payloads and processing.

## Build and configuration

Add these flags to your existing board environment or `override.build_flags`
in `platformio_override.ini`:

```ini
-D HASP_USE_CUSTOM=1
-D HASP_USE_UART_TRANSPORT=1
```

Both flags must be set. Leave both unset for stock behavior. This implementation
occupies `my_custom.h` and the custom callbacks; merge callbacks explicitly if
you already have a different custom extension. A separate LVGL task requires
openHASP's ESP MQTT/GUI mutex support; unsupported combinations fail compilation.

Upload `/serial.json` using the normal panel file editor, then restart. This
separate file is not part of openHASP's main `config.json` and is not rewritten
by normal configuration saves. There is no new web settings form.

Safe initial contents:

```json
{"enabled":false,"uart":1,"rx":-1,"tx":-1,"baud":115200}
```

| Key | Meaning |
| --- | --- |
| `enabled` | Boolean; absent file or false disables the transport |
| `uart` | Available hardware UART number, 1 or 2; UART0 is excluded |
| `rx` | Panel receive GPIO, connected to the host's TX |
| `tx` | Panel transmit GPIO, connected to the host's RX |
| `baud` | Integer 1200–921600; use 115200 for H5 |

All enabled settings must be valid; invalid configuration fails closed and logs
the reason on the normal debug output. Settings apply only at boot. Framing is
fixed at 8N1, non-inverted, no flow control. Logs and console echoes stay on the
normal console, never the transport UART. There is no transport auto-detection.

The startup checks reject unavailable/input-only TX pins, known system/display/
touch/backlight pins, configured GPIO functions, default console pins and active
S3 USB CDC pins. A configured serial dimmer reserves UART1; a compiled Tasmota
client reserves UART2. Do not add another UART consumer after boot. Custom
hardware, microphone, SD and expansion circuitry still require a board-level
pin review; software checks cannot establish electrical availability.

**DIS02050A is not a verified target in this change.** The release's existing
`elecrow-s3-8048c050_4MB` environment targets the older WZ8048C050. Do not flash
that environment onto CrowPanel Advance based on the similar name, or copy its
pin choices. Establish the Advance board's display/touch configuration and free
UART pins separately. This extension does not provide a board port.

## Porting to other hardware

The separation follows openHASP's existing `src/hal` and per-platform `src/dev`
patterns and its `ESP32` compile guards. Upstream does not supply a general UART
transport abstraction, so this fork adds a small one without changing BaseDevice.

| Layer | Files | Responsibilities |
| --- | --- | --- |
| Framing | `src/custom/uart_framing.h` | Bounded lines, output queue, partial writes, readiness marker; standard C++ only |
| Message content | `src/custom/uart_messages.h` | Stock state JSON to event/page tokens; existing ArduinoJson dependency only |
| openHASP integration | `src/custom/uart_transport.cpp` | Custom hooks, configuration file, main-loop command dispatch |
| Hardware contract | `src/hal/hasp_uart.h` | UART configuration, byte I/O, pin ownership, locking |
| ESP32 implementation | `src/hal/esp32/hasp_uart_esp32.cpp` | HardwareSerial, pin/UART checks and FreeRTOS mutex |

To port it, implement the functions declared in `hasp_uart.h` and define
`HASP_UART_EXTERNAL_HAL` to replace the built-in implementation. The interface
uses only standard integer/size types, not ESP32, Arduino, LVGL or JSON types.
`read()` returns -1 when no byte is available. Writes must be nonblocking and
may return short counts; the common queue retains unsent bytes. Supply a mutex
when hooks can run concurrently, or no-op locking on a single-threaded target.
Callbacks are never called from an ISR. Keep GUI locking in the platform's
normal openHASP loop, not in the UART HAL.

Port numbers and pin validity are hardware decisions: the common code does not
assume ESP32 ranges. The ESP32 implementation excludes UART0, but a future port
can choose its own safe port policy. The test HAL deliberately uses UART4 and
pins 101/102. Targets lacking an implementation compile a disabled fallback;
they do not accidentally enable unvalidated pins.

The current configuration loader uses openHASP's `HASP_FS` when SPIFFS/LittleFS
is enabled. Platforms using another storage model also need to supply configuration
loading in the integration layer; message encoding and parsing need no changes.

## Wire format

Host → panel uses ordinary openHASP commands, one per LF-terminated line:

```text
jsonl {"page":1,"id":21,"text":"0.125"}
page 2
```

Up to 1022 bytes of command text are accepted, plus optional CR and final LF.
NUL/control bytes, embedded CR and oversized lines are discarded through LF.
Empty lines are ignored. The receiver never executes a partial/overflow suffix.
It reads at most 256 bytes per main-loop iteration and dispatches complete
commands with `dispatch_text_line` in the existing GUI execution context.

Panel → host uses simple tokens separated by one ASCII space:

```text
event p1b48 down
event p1b48 release
page 2
ready 1
```

Object event names `down`, `up`, `release`, `lost`, `long`, `hold` and `changed`
are extracted from the stock state payload using openHASP's existing ArduinoJson
library. Only the top-level `event` string is used. Tags, values and metadata
are omitted. H5 acts on down/up/release/lost and ignores other event names.
Numeric page states are forwarded. Other states remain MQTT-only. No broker,
MQTT topic prefix or MQTT packet framing is carried on the UART. Keep page
names unset when using H5 so object topics remain numeric.

The fixed output queue holds eight frames of up to 512 bytes before LF. The
main loop drains at most 128 bytes at a time, limited by UART write capacity;
callbacks never wait for transmission at wire speed. Queue operations use a
mutex, so callbacks cannot interleave partial frames. A full queue is cleared
and replaced with a readiness marker, cancelling a peer's held touch and
requesting a refresh. The event that caused overflow is discarded. This avoids
silently retaining a held key after losing its release, but does not guarantee
event delivery. Repeated overload can still prevent operation.

`serial.dropped` is added to the ordinary sensor JSON when enabled. It counts
rejected output frames/overflow incidents, not every frame cleared by recovery.
It is diagnostic data, not an acknowledgement or link-health measure.

## Startup, reload and recovery

State forwarding is suppressed until a layout file has been read and the first
main-loop iteration after startup runs. The first output is `\nready 1\n`,
including a leading LF to terminate a stale partial input at the host. The normal
file-layout reload path also replaces pending output with that marker before
forwarding later events. This says the loader has returned; it does not validate
application-specific object IDs or certify every JSONL record. Upstream's
parser still reports layout errors through normal logging.

Uploading a layout file alone does not load it. Use the normal panel reload
operation or restart. For layouts streamed as individual JSONL commands, send
this stock custom command after the complete layout is installed:

```text
custom/serialready 1
```

It can also be used to resend readiness when the host starts after the panel.
There is no periodic ready spam, heartbeat, acknowledgement, or link-loss
watchdog. A wire/power failure can still lose a held touch's release. Hardware
validation and a separate decision on link-loss handling remain necessary.

## Tests and validation

From the repository root, using the existing ArduinoJson 6 dependency:

```sh
python3 tests/serial_transport/run_tests.py --arduinojson /path/to/ArduinoJson
```

Optionally test actual panel output against the H5 receiver and all 60 mappings:

```sh
python3 tests/serial_transport/run_tests.py \
  --arduinojson /path/to/ArduinoJson --h5 /path/to/nanoels/h5
```

`CXX` and `CXXFLAGS` may override the host compiler/flags. Host tests compile the
actual extension with fake hardware and real ArduinoJson, with MQTT both enabled
and disabled in the test configuration. They cover configuration validation,
pin conflicts, framing/limits, nonblocking partial output, overflow recovery,
startup ordering, filtered events, reload and H5 compatibility. The production
MQTT dispatcher is unchanged; fake-hardware tests do not exercise a real broker.

Host and address/undefined-behavior sanitizer tests passed. The common extension and ESP32 HAL sources
also compiled to ESP32-S3 objects against the installed Arduino-ESP32 framework
and real UART/FreeRTOS headers, with openHASP interfaces stubbed. That is not a
complete firmware build. The full release build was attempted but blocked during
PlatformIO dependency download by the execution environment's network policy.
A separate fake-HAL test also verifies operation without ESP32 APIs, and the
unsupported-target fallback is tested. No panel firmware has been flashed and hardware testing remains pending.
