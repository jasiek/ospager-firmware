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

  // Turn on the ADCs behind powerRead(). The battery/VBUS "voltage" measures
  // also gate the matching current ADC, so both are needed for the currents.
  PMU.enableBattDetection();
  PMU.enableBattVoltageMeasure();
  PMU.enableVbusVoltageMeasure();

  delay(50);   // let the radio regulator settle before SPI access
  return true;
}

PowerReadings powerRead() {
  PowerReadings r;
  r.dischargeMa = PMU.getBattDischargeCurrent();
  r.chargeMa    = PMU.getBatteryChargeCurrent();
  r.vbusMa      = PMU.getVbusCurrent();
  return r;
}
