#include <Wire.h>
#include "bq25155.h"

// CHEN pin
#define BQ_CHEN     2
// Charge Enable.
// Drive CE low or leave disconnected to enable charging when VIN is valid.
// Drive CE high to disable charge when VIN is present.
// CE is pulled low internally with 900-kΩ resistor.
// CE has no effect when VIN is not present.

// INT pin
#define BQ_INT  5
// Interruption pin.
// INT is an open-drain output that signals fault interrupts. 
// When a fault occurs, a 128-µs pulse is sent out as an interrupt for the host.
// INT is enabled/disabled using the MASK_INT bit in the control register.

// LPM pin
#define BQ_LPM      20
// Low Power Mode Enable.
// Drive this pin:
// -LOW to set the device in low power mode when powered by the battery.
// -HIGH to allow I2C communication when VIN is not present
// This pin must be driven high to allow I2C communication when VIN is not present.
// LP is pulled low internally with 900-kΩ resistor.
// This pin has no effect when VIN is present

BQ25155 charger;

void setup() {
  Serial.begin(115200);

  // Initialize with control pins: CHEN, INT, LPM
  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM)) {
    Serial.println("bq25155 not found!");
    while (1);
  } else {
	Serial.print(charger.getDeviceIDString());
	Serial.println(" found!");
  }

  // Configure charging profile
  charger.initCHG(
    4200,     // Target charge voltage in mV
    true,     // Enable fast charging
    100000,   // Charge current in uA
    25000,    // Precharge current in uA
    150,      // Input current limit in mA
    30        // Safety timer in tenths of an hour (e.g., 15 = 1.5h, 30 = 3h)
  );

  Serial.println("Charger configured.");
}

void loop(){
	delay(10000);
}