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

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  // Configure a safe charging profile.
  charger.initCHG(
    4200,    // Charge voltage in mV
    true,    // Enable fast charge
    50000,   // Charge current in uA
    10000,   // Pre-charge current in uA
    150,     // Input limit in mA
    3        // Safety timer request in hours
  );
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  // Enable LS/LDO block and set LDO voltage.
  charger.EnableLSLDO();
  charger.setLDOVoltage(1800);
  charger.EnableLDO();

  Serial.println("LDO enabled at 1.8V");
}

void loop() {
  // Toggle between LDO and Load Switch every 5 seconds.
  if (charger.isLDOEnabled()) {
    charger.EnableLS();
    Serial.println("Switched to Load Switch mode");
  } else {
    charger.EnableLDO();
    Serial.println("Switched to LDO mode");
  }

  Serial.print("LS/LDO enabled: ");
  Serial.println(charger.isLSLDOEnabled() ? "yes" : "no");
  Serial.print("LDO voltage: ");
  Serial.print(charger.getLDOVoltage());
  Serial.println(" mV");

  Serial.println("--------------------");
  delay(5000);
}
