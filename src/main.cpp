// ---------------------------------------------------------------------------
//  T-Beam POCSAG receiver
//
//  Hardware : LilyGo T-Beam v1.1  (ESP32 + SX1278 433 MHz + AXP192 PMU)
//  Function : Listen for POCSAG pager messages on 439.9875 MHz and print
//             every decoded message to the serial port.
//
//  POCSAG is direct-FSK demodulated by RadioLib's PagerClient. On the SX127x
//  the radio is put into continuous (direct) mode where:
//     * DIO1 outputs the recovered bit clock (DCLK)  -> drives the read ISR
//     * DIO2 outputs the demodulated data            -> sampled on each clock
// ---------------------------------------------------------------------------

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <RadioLib.h>

// Select the AXP192 PMU before including XPowersLib so that the unified
// XPowersPMU type is defined for this chip (T-Beam v1.1 uses the AXP192).
#define XPOWERS_CHIP_AXP192
#include <XPowersLib.h>

// ----- T-Beam v1.1 LoRa (SX1278) pin map -----------------------------------
#define LORA_SCK    5
#define LORA_MISO   19
#define LORA_MOSI   27
#define LORA_CS     18
#define LORA_RST    23
#define LORA_DIO0   26   // IRQ
#define LORA_DIO1   33   // direct-mode bit clock (DCLK)
#define LORA_DIO2   32   // direct-mode data        (DATA)

// ----- I2C bus for the AXP192 power-management chip ------------------------
#define I2C_SDA     21
#define I2C_SCL     22

// ----- POCSAG receive settings ---------------------------------------------
// Center frequency of the paging channel we want to listen to.
static const float    POCSAG_FREQ_MHZ = 439.9875f;
// POCSAG baud rate. The most common is 1200; some networks use 512 or 2400.
// If you see no/garbled output, try the other two values.
static const uint16_t POCSAG_BAUD     = 1200;
// Some transmitters invert the FSK polarity. If messages decode as garbage
// (consistently), flip this to true.
static const bool     POCSAG_INVERT   = false;

// SX1278 module wiring: Module(cs, irq = DIO0, rst, gpio = DIO1)
SX1278 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);
PagerClient pager(&radio);

// AXP192 power-management unit (auto-detected by XPowersLib)
XPowersPMU PMU;

// Halt with a blinking-free busy loop after a fatal init error.
static void halt(const char *msg) {
  Serial.println(msg);
  Serial.flush();
  while (true) {
    delay(1000);
  }
}

// Bring up the AXP192 and power the LoRa radio rail (LDO2 on the T-Beam).
static bool setupPower() {
  if (!PMU.begin(Wire, AXP192_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
    return false;
  }
  // LDO2 -> SX1278 VDD.  (LDO3 -> GPS, left off; DCDC1/3 -> ESP32, untouched.)
  PMU.setLDO2Voltage(3300);
  PMU.enableLDO2();
  // Give the radio's regulator a moment to settle before SPI access.
  delay(50);
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("=== T-Beam POCSAG receiver ==="));

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!setupPower()) {
    halt("AXP192 not found over I2C - is this a v1.1 board? Halting.");
  }
  Serial.println(F("AXP192 OK, LoRa rail (LDO2) enabled."));

  // Bring up the SPI bus the radio lives on.
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);

  Serial.print(F("Init SX1278 (FSK) ... "));
  int state = radio.beginFSK();
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("failed (code %d)\n", state);
    halt("Radio init failed. Check wiring/power. Halting.");
  }
  Serial.println(F("OK"));

  Serial.print(F("Init POCSAG decoder ... "));
  state = pager.begin(POCSAG_FREQ_MHZ, POCSAG_BAUD, POCSAG_INVERT);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("failed (code %d)\n", state);
    halt("Pager init failed. Halting.");
  }
  Serial.println(F("OK"));

  Serial.print(F("Start receive ... "));
  // addr = 0, mask = 0  -> match every capcode, i.e. dump all traffic.
  state = pager.startReceive(LORA_DIO2, (uint32_t)0, (uint32_t)0);
  if (state != RADIOLIB_ERR_NONE) {
    Serial.printf("failed (code %d)\n", state);
    halt("startReceive failed. Halting.");
  }
  Serial.printf("listening on %.4f MHz @ %u baud%s\n",
                POCSAG_FREQ_MHZ, POCSAG_BAUD,
                POCSAG_INVERT ? " (inverted)" : "");
  Serial.println(F("------------------------------------------"));
}

void loop() {
  // Messages are decoded in the background ISR; poll for completed frames.
  if (pager.available() >= 2) {
    String msg;
    uint32_t addr = 0;

    // readData(str, len, addr): len = 0 reads all available bytes.
    int state = pager.readData(msg, 0, &addr);
    if (state == RADIOLIB_ERR_NONE) {
      Serial.printf("[POCSAG] capcode=%lu  msg=", (unsigned long)addr);
      Serial.println(msg);
    } else {
      Serial.printf("[POCSAG] decode error (code %d)\n", state);
    }
  }
}
