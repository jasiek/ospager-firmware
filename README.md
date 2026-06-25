# ospager-firmware

Open-hardware POCSAG **pager** firmware, currently targeting the **LilyGo
T-Beam v1.1** (ESP32 + SX1278 433 MHz + AXP192). It receives POCSAG on
**439.9875 MHz**, keeps an RTC synced from the broadcast time messages, and
presents a small text UI that renders identically to a **serial terminal** and
to a **graphic display** (an SSD1306 OLED today; a larger LCD later).

The UI is laid out on a fixed character grid (`UI_COLS`×`UI_ROWS`, 21×8 for a
128×64 OLED) so a single set of screens drives every output. The serial terminal
is the primary UI for development; the OLED renders the same screens to prove
the abstraction on real hardware.

## Hardware

LilyGo T-Beam **v1.1**, 433 MHz variant (SX1278), with an optional **SSD1306
128×64 OLED** on the shared I²C bus (`0x3C`). The radio is on-board; the LoRa
power rail (AXP192 `LDO2`) is enabled at boot, so a real v1.1 (AXP192) is
assumed. With no OLED present it runs as a serial-only terminal UI.

| Signal | GPIO | | Signal | GPIO |
|--------|------|-|--------|------|
| LoRa SCK | 5 | | LoRa DIO0 | 26 (IRQ) |
| LoRa MISO | 19 | | LoRa DIO1 | 33 (DCLK) |
| LoRa MOSI | 27 | | LoRa DIO2 | 32 (DATA) |
| LoRa CS | 18 | | I²C SDA | 21 |
| LoRa RST | 23 | | I²C SCL | 22 |

## Build, flash, view

```sh
pio run                 # build
pio run -t upload       # flash
```

The UI is a full-screen ANSI/VT100 terminal app. View it with any terminal at
115200 baud — `tio`, `picocom`, or `screen` give the cleanest full-screen
render; `pio device monitor` also works (don't enable `monitor_filters`, they
corrupt cursor positioning).

```sh
tio /dev/cu.usbserial-XXXX        # or: pio device monitor
```

### Controls

| Key | Action |
|-----|--------|
| ↑/↓ or k/j | move selection |
| Enter or → | select / open |
| Esc or q | back |
| d | delete the selected message |
| Ctrl-L | redraw (use if a reconnected terminal shows a partial screen) |

Home shows the clock, date and sync/traffic status. **Any key** opens the main
**Menu** (Messages, Diagnostics). **Messages** is the scrollable list; Enter on a
message opens **Detail** (full text); q steps back. In the list (or in Detail)
press **d** to delete the current message; the remaining messages keep their
order and the selection lands on the next one. **Diagnostics → Power** shows the
live AXP192 currents (battery discharge, battery charge, and VBUS input).

The static frame is drawn once at connect; most terminals reset the board on
connect (you'll see it repaint), but if you attach without a reset and the
screen looks partial, press **Ctrl-L**.

## Architecture

A headless core feeds a UI layer through a character-grid `FrameBuffer`. Screens
render once into the grid; each attached `Surface` draws it. Nothing in the core
or screens talks to a specific device, so adding a display (or a web UI, or
tests) is a new `Surface`, and a new view is a new `Screen`.

```
include/ + src/
  board.h            pin map + UI grid size (the per-board file)
  power.*            AXP192 bring-up (LoRa rail) + current readings (powerRead)
  time_sync.*        Clock: RTC kept in UTC, shown via DISPLAY_TZ (DST-aware)
  message_store.*    ring buffer of recent messages (revision counter)
  pocsag_service.*   owns radio+pager; decodes, syncs clock, stores messages
  ui/surface.h       Cell / FrameBuffer / Surface interface
  ui/serial_surface.*  ANSI backend (border frame + per-cell diffing)
  ui/oled_surface.*    SSD1306 backend (cell -> 6x8 char, redraw on change)
  ui/input.*         InputEvent + serial-key decoder (arrows/enter/esc/jkdq)
  ui/ui.*            Screen base + UiManager (screen stack, fan-out to surfaces)
  ui/screens.*       HomeScreen / MessagesScreen / DetailScreen
  main.cpp           composition root: wires core + UI, runs the loop
```

The same `InputEvent` set (`Up/Down/Select/Back/...`) will later be produced by
GPIO buttons, so screens never see raw keys.

## Time sync & RTC

The RTC is the ESP32 system clock, stored in **UTC** and displayed in local time
via `DISPLAY_TZ` (default `CET-1CEST,M3.5.0,M10.5.0/3`). It free-runs on the
crystal between syncs, so the clock stays second-precise.

This network sends several time broadcasts sharing the `YYYYMMDDHHMMSS` label
but differing by capcode/timezone, so they are told apart by capcode:

| Capcode | Format | Zone | Sync source? |
|---------|--------|------|--------------|
| 224 | `YYYYMMDDHHMMSS`+YYMMDDHHMMSS | local | yes (rare) |
| 216 | `YYYYMMDDHHMMSS`+YYMMDDHHMMSS | UTC | yes (frequent) |
| 2504 | `HHMMSS DDMMYY` | UTC | no (216 covers it) |
| 208 | `XTIME=HHMMDDMMYY` | local | no (minute precision only) |

`SYNC_CAPCODE_LOCAL` / `SYNC_CAPCODE_UTC` in `pocsag_service.cpp` select the
trusted senders; change them if your paging network differs.

## Tuning

- `board.h` — pin map, `POCSAG_FREQ_MHZ`, `POCSAG_BAUD` (512/1200/2400),
  `POCSAG_INVERT`, and the `UI_COLS`/`UI_ROWS` grid size.
- `time_sync.cpp` — `DISPLAY_TZ`.
- `pocsag_service.cpp` — the sync capcodes.

## How it works

POCSAG is plain 2-FSK. RadioLib's `PagerClient` runs the SX1278 in continuous
(direct) mode: `DIO1` is the recovered bit clock (driving a read ISR) and `DIO2`
the demodulated data. Decoded time messages set the RTC; the loop repaints the
grid once per second (and on input or new traffic), and each surface transmits
only what changed.
