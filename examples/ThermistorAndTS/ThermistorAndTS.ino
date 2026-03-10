#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin
static constexpr BatteryChemistry BQ_CHEM = LI_ION_4V2;
static constexpr bool BQ_USE_PG_LED = true;
static constexpr bool BQ_LED_ON_WHEN_DONE = false;

bq25155 charger;

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM, BQ_CHEM, BQ_USE_PG_LED)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  // Apply a baseline charging profile before enabling TS/JEITA behavior.
  ChargeProfile profile;
  profile.chargeVoltage_mV = 4200;
  profile.enableFastCharge = true;
  profile.chargeCurrent_uA = 100000;
  profile.prechargeCurrent_uA = 20000;
  profile.inputCurrentLimit = ILIMLevel::ILIM_150mA;
  profile.safetyTimer = SafetyTimerLimit::HOURS_3;
  profile.use2xSafetyTimer = false;
  profile.ledOnWhenChargeDone = BQ_LED_ON_WHEN_DONE;
  if (!charger.applyChargeProfile(profile)) {
    Serial.println("Failed to apply charge profile");
    while (1) { delay(1000); }
  }

  // Enable TS function and JEITA-style control.
  charger.EnableTS();
  charger.CustomTSControl();
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  // Configure TS thresholds (0-1200 mV range).
  charger.setTSVAL(1000, TSThresholdRegister::COLD);
  charger.setTSVAL(900,  TSThresholdRegister::COOL);
  charger.setTSVAL(600,  TSThresholdRegister::WARM);
  charger.setTSVAL(500,  TSThresholdRegister::HOT);

  // Reduce charge voltage by 100 mV in WARM region.
  charger.setTSVCHG(100);

  // Reduce charge current to 0.5x in COOL/WARM regions.
  charger.setTSICHG(500); // 500 = 0.500x

  Serial.println("TS thresholds configured.");
}

void loop() {
  charger.enforceSafetyFaultPolicy();

  Serial.print("TS COLD threshold: ");
  Serial.print(charger.getTSVAL(TSThresholdRegister::COLD));
  Serial.println(" mV");
  Serial.print("TS COOL threshold: ");
  Serial.print(charger.getTSVAL(TSThresholdRegister::COOL));
  Serial.println(" mV");
  Serial.print("TS WARM threshold: ");
  Serial.print(charger.getTSVAL(TSThresholdRegister::WARM));
  Serial.println(" mV");
  Serial.print("TS HOT threshold: ");
  Serial.print(charger.getTSVAL(TSThresholdRegister::HOT));
  Serial.println(" mV");

  Serial.println("--------------------");
  delay(5000);
}
