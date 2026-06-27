// AXP192 power management for the T-Beam v1.1.
#pragma once

// Initialise the AXP192 and enable the LoRa radio rail (LDO2).
// Wire.begin() must have been called first. Returns false if the PMU is absent.
bool powerInit();

// A snapshot of the AXP192 current ADCs, in milliamps.
struct PowerReadings {
  float dischargeMa;  // battery -> board: total draw when running on battery
  float chargeMa;     // board -> battery: current into the cell while charging
  float vbusMa;       // USB/VBUS input current (feeds the board and charger)
};

// Read the current ADCs. Valid after powerInit(); returns zeros if the PMU is
// absent. On USB power the battery discharge reads ~0 and vbusMa is the supply.
PowerReadings powerRead();

// Power the board off via the AXP192, dropping every rail. Only the power
// button or a charger insert brings it back. Does not return on real hardware.
void powerOff();
