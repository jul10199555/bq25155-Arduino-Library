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
    4200,     // Target charge voltage in mV
    true,     // Enable fast charging
    100000,   // Charge current in uA
    25000,    // Precharge current in uA
    150,      // Input current limit in mA
    30         // Safety timer (in tenths of an hour (e.g., 15 = 1.5h, 30 = 3h))
  );

  Serial.println("Charger configured.");
}

void loop(){
	delay(5000);
}