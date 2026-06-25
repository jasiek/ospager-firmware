#include "power.h"

#include <Arduino.h>
#include <Wire.h>

#include "board.h"

// Select the AXP192 PMU before including XPowersLib so XPowersPMU is defined.
#define XPOWERS_CHIP_AXP192
#include <XPowersLib.h>

static XPowersPMU PMU;

bool powerInit() {
  if (!PMU.begin(Wire, AXP192_SLAVE_ADDRESS, PIN_I2C_SDA, PIN_I2C_SCL)) {
    return false;
  }
  // LDO2 -> SX1278 VDD.  (LDO3 -> GPS, left off; DCDC1/3 -> ESP32, untouched.)
  PMU.setLDO2Voltage(3300);
  PMU.enableLDO2();
  delay(50);   // let the radio regulator settle before SPI access
  return true;
}
