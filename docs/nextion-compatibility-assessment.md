# Panel-side Nextion compatibility assessment

Status: design/research only, 2026-09-27. **This library belongs in openHASP**, above
the shared serial hardware interface. It would let NanoELS run its existing
Nextion backend against an openHASP panel. No emulator is implemented here.

## Recommendation

Start with a clearly documented NanoELS runtime subset, independently implemented
in portable C++. Keep the existing generic openHASP UART protocol available.
Choose the protocol explicitly at startup; do not auto-detect formats on a
machine-control connection. A reusable library should know commands and logical
components, while a NanoELS mapping file supplies the actual page/object IDs.

| Layer | Responsibility |
| --- | --- |
| UART HAL | Bytes, pin/baud configuration, locking; current ESP32 implementation |
| Protocol selection | One active serial codec: openHASP lines or Nextion binary framing |
| Portable Nextion codec | Bounded parsing, three-0xFF terminator, touch packet encoding |
| Component mapping | Logical page/name/ID to openHASP page/object; layout-owned data |
| openHASP adapter | Apply properties/page changes; translate touch lifecycle |

Share the byte HAL, not the current newline receiver: Nextion frames are binary
terminated. No ESP32 headers should enter the codec. Touch state and queued writes
must preserve release ordering, partial-write behavior and page transitions.

## Smallest useful contract, from audited H5 source

| NanoELS behavior | Proposed panel handling |
| --- | --- |
| `<name>.txt="..."` | Parse supported assignment, map 18 names, set text |
| `page 0` / `page 1` | Map to openHASP 1 / 2 |
| `play 0,0,0` | Optional audio hook; explicitly unsupported until audio is validated |
| Three 0xFF command terminators | Streaming bounded parser, resynchronize malformed frames |
| Legacy 0xDF degree glyph | Convert to UTF-8 degree character |
| Touch notification | Emit 0x65,page,id,pressed,0xFF,0xFF,0xFF |
| Limit aliases/local BACK | Use current mapping; BACK is panel-local |

Source: [H5 adapter and retained Nextion backend](https://github.com/nufanwfk/nanoels/tree/codex/h5-openhasp-adapter/h5),
particularly `display_protocol.h`, `display_mapping.h`, `setText`,
`setScreenPage`, and `readScreenEvent` in `h5.ino`. Before implementation,
capture traffic from the exact stock H5 firmware that will be supported and
compare it with the source audit. The official Nextion instruction-set endpoint
returned HTTP 403 during research; it was not treated as verified full coverage.

The H5 TFT updater is a separate protocol (`connect`, baud negotiation,
`whmi-wri`, binary upload). Do not falsely acknowledge it. An openHASP panel
cannot load a Nextion `.tft` image. The existing H5 Nextion backend does not
have the openHASP adapter's `ready 1` full-redraw mechanism: after a panel reset,
unchanged cached fields may remain stale. This needs a documented limitation
or an agreed controller-side recovery mechanism; panel emulation alone does
not automatically recover data that H5 never resends.

## Boundaries and effort

A NanoELS subset is feasible without implementing the Nextion scripting language.
A planning allowance is several engineering days for a portable prototype plus
one or two weeks of integration/physical regression work, depending on audio,
reset behavior and malformed-input coverage. This is an estimate, not a schedule.

Broader compatibility adds variables, expressions, event scripts, timers,
component properties, global/page state, `get`/`sendme`/`bkcmd`, return codes,
baud/sleep behavior, font/media formats and uploader/editor compatibility.
Running arbitrary HMI/TFT projects is a fundamentally larger undertaking and
should not be promised under a claim of full Nextion emulation. Expand with
explicit capability versions and real application fixtures, not an unbounded
claim of drop-in compatibility.

## Test plan before publishing a compatibility claim

- Golden wire captures for every NanoELS command and mapped button.
- Fragment every command/terminator across reads; partial writes, overflow,
  malformed quotes, unknown names/pages, long text and degree encoding.
- Press → page change → release, duplicate/stale release, drag-off and reboot.
- Observe Nextion backend caches and panel reset behavior with real H5.
- Confirm unsupported commands have documented behavior and cannot start upload.
- Build native codec tests without Arduino, then test at least one real panel.

## Licensing and legal questions

An independent implementation of a functional serial contract has a different
risk profile from copying vendor firmware, documentation, fonts or artwork.
US [17 USC 102(b)](https://www.law.cornell.edu/uscode/text/17/102) excludes methods
of operation from copyright protection, but does not give blanket clearance for
all API-related expression. [Google v. Oracle (2021)](https://www.supremecourt.gov/opinions/20pdf/18-956_d18f.pdf)
was a particular fair-use holding, not a universal permission to copy APIs.
[17 USC 1201(f)](https://www.law.cornell.edu/uscode/text/17/1201) contains a
conditional interoperability exception; it is not general permission to bypass
access controls. No such bypass is needed for the proposed wire-level subset.

Keep independent source/provenance records, retain MIT notices for any NanoELS
or openHASP code reused, and avoid distributing Nextion firmware, editor binaries,
proprietary `.tft` assets or vendor fonts. Use accurate compatibility wording
without vendor logos or implied affiliation. Nextion editor/device terms,
trademark rights, patents and non-US law have not been cleared in this research.
This is a technical risk assessment, not legal clearance; obtain qualified legal
review before a public compatibility product or broad branding commitment.

## Decision for the owner

Proceed only if the narrowly defined runtime subset provides enough community
value despite uploader, audio and restart limitations. A generic portable codec
plus application mappings is the recommended first deliverable. Keep it separate
from the UART transport and board-support upstream proposals.
