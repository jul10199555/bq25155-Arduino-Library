#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin
static constexpr BatteryChemistry BQ_CHEM = LI_ION_4V2;

bq25155 charger;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  // Initialize with control pins: CHEN, INT, LPM
  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM, BQ_CHEM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  Serial.print(charger.getDeviceIDString());
  Serial.println(" found!");

  // Configure charging profile
  // Safety timer request is in hours; the device supports 3, 6, 12 hours (0 disables).
  ChargeProfile profile;
  profile.chargeVoltage_mV = 4400; // Will clamp to chemistry max
  profile.enableFastCharge = true;
  profile.chargeCurrent_uA = 220000;   // May clamp to ILIM-safe value
  profile.prechargeCurrent_uA = 120000; // Will clamp to <=40% of ICHG
  profile.inputCurrentLimit = ILIMLevel::ILIM_150mA;
  profile.safetyTimer = SafetyTimerLimit::HOURS_3;
  profile.use2xSafetyTimer = false;

  bool ok = charger.applyChargeProfile(profile);
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  if (!ok) {
    Serial.println("Charger configuration failed!");
    while (1) { delay(1000); }
  }

  Serial.println("Charger configured.");
  Serial.print("Applied VBAT target (mV): ");
  Serial.println(charger.getChargeVoltage());
  Serial.print("Applied ICHG (uA): ");
  Serial.println(charger.getChargeCurrent());
  Serial.print("Applied IPRECHG (uA): ");
  Serial.println(charger.getPrechargeCurrent());
}

void loop() {
  delay(10000);
}
