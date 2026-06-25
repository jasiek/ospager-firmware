// Board support: pin map and hardware constants for the LilyGo T-Beam v1.1.
// Kept separate so other open-hardware variants can swap this one file.
#pragma once

#include <stdint.h>

// ----- LoRa SX1278 (433 MHz) -----------------------------------------------
// Prefixed PIN_* to avoid colliding with the LORA_* macros the T-Beam Arduino
// variant already defines in pins_arduino.h.
constexpr int PIN_LORA_SCK  = 5;
constexpr int PIN_LORA_MISO = 19;
constexpr int PIN_LORA_MOSI = 27;
constexpr int PIN_LORA_CS   = 18;
constexpr int PIN_LORA_RST  = 23;
constexpr int PIN_LORA_DIO0 = 26;   // IRQ
constexpr int PIN_LORA_DIO1 = 33;   // direct-mode bit clock (DCLK)
constexpr int PIN_LORA_DIO2 = 32;   // direct-mode data        (DATA)

// ----- I2C bus (AXP192 PMU + SSD1306 OLED) ---------------------------------
constexpr int     PIN_I2C_SDA = 21;
constexpr int     PIN_I2C_SCL = 22;
constexpr uint8_t OLED_ADDR   = 0x3C;
constexpr int     OLED_W      = 128;
constexpr int     OLED_H      = 64;

// ----- POCSAG receive defaults ---------------------------------------------
constexpr float    POCSAG_FREQ_MHZ = 439.9875f;
constexpr uint16_t POCSAG_BAUD     = 1200;    // 512 / 1200 / 2400
constexpr bool     POCSAG_INVERT   = false;

// ----- Canonical UI grid ----------------------------------------------------
// The whole UI is laid out on a fixed character grid so the same screens render
// identically to the serial terminal and to a graphic display. 21x8 matches a
// 128x64 OLED at the 6x8 font; bump these for a larger future LCD.
constexpr uint8_t UI_COLS = 21;
constexpr uint8_t UI_ROWS = 8;
