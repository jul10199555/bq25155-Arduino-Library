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
    Serial.println("BQ25155 not found!");
    while (1);
  } else {
	Serial.print(charger.getDeviceIDString());
	Serial.println(" found!");
  }
  
  // Configure charging profile
  charger.initCHG(
    4200,    // Charge voltage in mV
    true,    // Enable fast charge
    150000,  // Charge current in µA
    30000,   // Pre-charge current in µA
    150,     // Input limit in mA
    60        // Safety timer in tenths of an hour (e.g., 15 = 1.5h, 30 = 3h)
  );
}

void loop() {

  if (charger.is_CHARGE_DONE()) {
    Serial.println("Charge complete");
  }else{
    if (charger.is_VIN_PGOOD()) {
      Serial.println("INPUT Voltage Good!");
    }
    
    Serial.print("Input Voltage: ");
    Serial.println(charger.readVIN(1));
    Serial.print("Input Current: ");
    Serial.println(charger.readIIN(2));
    
    Serial.print("Battery Voltage: ");
    Serial.println(charger.readVBAT(1));
    Serial.print("Charging Current: ");
    Serial.println(charger.readICHG(2));
  }
  
  Serial.println("--------------------");
  
  delay(2000);
}