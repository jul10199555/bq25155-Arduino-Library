#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin

bq25155 charger;
static constexpr BatteryChemistry BQ_CHEM = LI_HV_4V35;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM, BQ_CHEM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  // Intentionally aggressive requests to demonstrate internal clamps:
  // - VBAT will be clamped by selected chemistry (4.35 V max here)
  // - ICHG will be clamped to remain below ILIM
  // - IPRECHG will be clamped to <=40% of effective ICHG
  const bool ok = charger.initCHG(
    4600,    // Requested charge voltage in mV (clamped by chemistry)
    true,    // Fast-charge range
    500000,  // Requested charge current in uA
    200000,  // Requested pre-charge current in uA
    150,     // Input current limit in mA
    6        // Safety timer request in hours
  );
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  if (!ok) {
    Serial.println("Failed to initialize charger profile");
    while (1) { delay(1000); }
  }

  Serial.println("Applied profile:");
  Serial.print("  VBAT (mV): ");
  Serial.println(charger.getChargeVoltage());
  Serial.print("  ICHG (uA): ");
  Serial.println(charger.getChargeCurrent());
  Serial.print("  IPRECHG (uA): ");
  Serial.println(charger.getPrechargeCurrent());
  Serial.print("  ILIM code: ");
  Serial.println(charger.getILIM());
}

void loop() {
  bool chargeDisabled = false;
  if (!charger.enforceSafetyFaultPolicy(&chargeDisabled)) {
    Serial.println("Safety policy check failed");
  } else if (chargeDisabled) {
    Serial.println("Safety policy disabled charging after a severe fault");
  }

  delay(2000);
}
