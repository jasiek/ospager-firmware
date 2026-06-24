# ospager-firmware

POCSAG-synchronized clock for the **LilyGo T-Beam v1.1** (ESP32 + SX1278
433 MHz + AXP192). It listens on **439.9875 MHz**, decodes POCSAG messages with
[RadioLib](https://github.com/jgromes/RadioLib), syncs the ESP32 internal RTC
from the broadcast time messages, and shows a live `HH:MM:SS` clock (second
precision) on an SSD1306 OLED. Every decoded message is also printed to serial.

## Hardware

LilyGo T-Beam **v1.1**, 433 MHz variant (SX1278), plus an **SSD1306 128×64
OLED** on the shared I²C bus (address `0x3C`). The radio is already on-board; no
external wiring beyond the OLED. The firmware enables the LoRa power rail
(AXP192 `LDO2`) at boot, so it assumes a real v1.1 (AXP192). A v1.2 uses an
AXP2101 and would need a different PMU init. If no OLED is found, it runs as a
serial-only clock.

| Signal      | ESP32 GPIO |
|-------------|------------|
| LoRa SCK    | 5          |
| LoRa MISO   | 19         |
| LoRa MOSI   | 27         |
| LoRa CS     | 18         |
| LoRa RST    | 23         |
| LoRa DIO0   | 26 (IRQ)   |
| LoRa DIO1   | 33 (DCLK)  |
| LoRa DIO2   | 32 (DATA)  |
| I²C SDA     | 21 (AXP192 + OLED) |
| I²C SCL     | 22 (AXP192 + OLED) |

## Build, flash, monitor

```sh
pio run                 # build
pio run -t upload       # flash (auto-detects the USB port)
pio device monitor      # serial @ 115200
```

Expected output:

```
=== T-Beam POCSAG clock ===
AXP192 OK, LoRa rail (LDO2) enabled.
SSD1306 OLED OK
Init SX1278 (FSK) ... OK
Init POCSAG decoder ... OK
Start receive ... listening on 439.9875 MHz @ 1200 baud
------------------------------------------
[POCSAG] capcode=216  msg=YYYYMMDDHHMMSS260624213600
[RTC] synced from capcode 216 (UTC) -> local 2026-06-24 23:36:00
```

## Time sync & RTC

The RTC is the ESP32's internal system clock. It is stored in **UTC** and
displayed in **local time** via a POSIX timezone string (`DISPLAY_TZ`,
default `CET-1CEST,M3.5.0,M10.5.0/3` — Central European with EU DST). Between
syncs the RTC free-runs on the crystal, so the clock stays second-precise even
though broadcasts arrive only every few minutes.

This network sends several time broadcasts, all sharing the `YYYYMMDDHHMMSS`
label but differing by capcode and timezone, so they are told apart by capcode:

| Capcode | Format                          | Timezone | Used for sync?            |
|---------|---------------------------------|----------|---------------------------|
| 224     | `YYYYMMDDHHMMSS` + YYMMDDHHMMSS | local    | yes (rare)                |
| 216     | `YYYYMMDDHHMMSS` + YYMMDDHHMMSS | UTC      | yes (frequent, ~4 min)    |
| 2504    | `HHMMSS   DDMMYY`               | UTC      | no (216 already covers it)|
| 208     | `XTIME=HHMMDDMMYY`              | local    | no (minute precision only)|

Syncing is driven mainly by the frequent UTC sender (216); the rarer local
sender (224) is accepted too. Senders without seconds (208) are ignored so the
displayed seconds stay accurate.

## Tuning

Constants near the top of [`src/main.cpp`](src/main.cpp):

- **`POCSAG_FREQ_MHZ`** — receive frequency (default `439.9875`).
- **`POCSAG_BAUD`** — `512`, `1200`, or `2400`. POCSAG does not announce its
  rate; if you see nothing or garbage, try the others.
- **`POCSAG_INVERT`** — flip to `true` for transmitters with inverted FSK.
- **`DISPLAY_TZ`** — POSIX timezone for the displayed local time.
- **`SYNC_CAPCODE_LOCAL` / `SYNC_CAPCODE_UTC`** — capcodes trusted as time
  sources (network-specific; change if your paging network differs).

The receiver matches **all** capcodes (`startReceive(LORA_DIO2, 0, 0)`), so
every message is logged to serial; only the sync capcodes set the clock.

## How it works

POCSAG is plain 2-FSK. RadioLib's `PagerClient` puts the SX1278 into continuous
(direct) mode: `DIO1` carries the recovered bit clock (driving a read ISR) and
`DIO2` carries the demodulated data, sampled on each clock edge and fed through
the POCSAG batch/codeword decoder. Decoded time messages set the RTC; the main
loop redraws the OLED once per second.
