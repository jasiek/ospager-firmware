// AXP192 power management for the T-Beam v1.1.
#pragma once

// Initialise the AXP192 and enable the LoRa radio rail (LDO2).
// Wire.begin() must have been called first. Returns false if the PMU is absent.
bool powerInit();
