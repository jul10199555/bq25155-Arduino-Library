#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin

bq25155 charger;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // Initialize with control pins: CHEN, INT, LPM
  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  Serial.print(charger.getDeviceIDString());
  Serial.println(" found!");

  // Configure charging profile
  // Safety timer request is in hours; the device supports 3, 6, 12 hours (0 disables).
  bool ok = charger.initCHG(
    4200,     // Target charge voltage in mV
    true,     // Enable fast charging
    100000,   // Charge current in uA
    25000,    // Precharge current in uA
    150,      // Input current limit in mA
    3         // Safety timer request in hours
  );
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  if (!ok) {
    Serial.println("Charger configuration failed!");
    while (1) { delay(1000); }
  }

  Serial.println("Charger configured.");
}

void loop() {
  delay(10000);
}
