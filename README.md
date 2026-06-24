# ospager-firmware

POCSAG pager receiver for the **LilyGo T-Beam v1.1** (ESP32 + SX1278 433 MHz +
AXP192). It listens on **439.9875 MHz**, decodes POCSAG messages with
[RadioLib](https://github.com/jgromes/RadioLib), and prints every message to the
serial port.

## Hardware

LilyGo T-Beam **v1.1**, 433 MHz variant (SX1278). No external wiring needed —
the radio is already on-board. The firmware enables the LoRa power rail (AXP192
`LDO2`) at boot, so make sure the board really is a v1.1 (AXP192). The newer
v1.2 uses an AXP2101 and would need a different PMU init.

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
| AXP192 SDA  | 21         |
| AXP192 SCL  | 22         |

## Build, flash, monitor

```sh
pio run                 # build
pio run -t upload       # flash (auto-detects the USB port)
pio device monitor      # serial @ 115200
```

Expected output:

```
=== T-Beam POCSAG receiver ===
AXP192 OK, LoRa rail (LDO2) enabled.
Init SX1278 (FSK) ... OK
Init POCSAG decoder ... OK
Start receive ... listening on 439.9875 MHz @ 1200 baud
------------------------------------------
[POCSAG] capcode=1234567  msg=HELLO WORLD
```

## Tuning

All knobs are constants near the top of [`src/main.cpp`](src/main.cpp):

- **`POCSAG_FREQ_MHZ`** — receive frequency (default `439.9875`).
- **`POCSAG_BAUD`** — `512`, `1200`, or `2400`. POCSAG does not announce its
  baud rate, so if you see nothing or only garbage, try the other two values.
- **`POCSAG_INVERT`** — flip to `true` if messages decode as consistent
  garbage; some transmitters invert the FSK polarity.

The receiver matches **all** capcodes (address `0`, mask `0`). To filter to a
specific pager, change the `pager.startReceive(LORA_DIO2, addr, mask)` call.

## How it works

POCSAG is plain 2-FSK. RadioLib's `PagerClient` puts the SX1278 into continuous
(direct) mode: `DIO1` carries the recovered bit clock (driving a read ISR) and
`DIO2` carries the demodulated data, which is sampled on each clock edge and
fed through the POCSAG batch/codeword decoder.
