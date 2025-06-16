#include <Wire.h>
#include "bq25155.h"

bq25155 charger;

void setup() {
  Serial.begin(115200);

  // Initialize with control pins: CHEN, INT, LPM
  if (!charger.begin(5, 6, 7)) {
    Serial.println("BQ25155 not found!");
    while (1);
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
  if (charger.CHRG_DONE_Flag()) {
    Serial.println("Charge complete");
  }

  delay(1000);
}
