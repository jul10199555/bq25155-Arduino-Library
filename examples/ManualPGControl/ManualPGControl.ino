#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin
static constexpr BatteryChemistry BQ_CHEM = LI_ION_4V2;

// Manual PG mode (begin(..., false)).
static constexpr bool BQ_USE_PG_LED = false;

// Side-by-side manual LED policies (pick one):
// true  -> LED on only when charge is done or safety timer fault is latched.
// false -> LED on while charging is active, off when done/fault latched.
static constexpr bool BQ_MANUAL_LED_ON_WHEN_DONE = true;

bq25155 charger;
static bool chargeDoneLatched = false;
static bool prevChargeEnabled = false;

static bool applyManualDoneActive(bool chargeEnabled, bool doneLatched) {
  if (!chargeEnabled) {
    return charger.DisablePG(); // High-Z while charge is disabled.
  }
  return doneLatched ? charger.EnablePG() : charger.DisablePG();
}

static bool applyManualChargeActive(bool chargeEnabled, bool doneLatched) {
  if (!chargeEnabled) {
    return charger.DisablePG(); // High-Z while charge is disabled.
  }
  return doneLatched ? charger.DisablePG() : charger.EnablePG();
}

static bool updateManualPGIndicator() {
  const bool chargeEnabled = charger.isChargeEnabled();

  // New charging cycle clears the done latch.
  if (chargeEnabled && !prevChargeEnabled) {
    chargeDoneLatched = false;
  }
  prevChargeEnabled = chargeEnabled;

  // Flags are clear-on-read; keep manual latch in sketch scope.
  charger.readAllFLAGS();
  if (charger.CHRG_DONE_Flag() || charger.SAFETY_TMR_Flag()) {
    chargeDoneLatched = true;
  }

  if (BQ_MANUAL_LED_ON_WHEN_DONE) {
    return applyManualDoneActive(chargeEnabled, chargeDoneLatched);
  }
  return applyManualChargeActive(chargeEnabled, chargeDoneLatched);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM, BQ_CHEM, BQ_USE_PG_LED)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  // In manual mode, sketch owns PG configuration and state transitions.
  if (!charger.setPGasGPOD()) {
    Serial.println("Failed to set PG as GPOD");
    while (1) { delay(1000); }
  }
  if (!charger.DisablePG()) {
    Serial.println("Failed to initialize PG output");
    while (1) { delay(1000); }
  }

  ChargeProfile profile;
  profile.chargeVoltage_mV = 4200;
  profile.enableFastCharge = true;
  profile.chargeCurrent_uA = 100000;
  profile.prechargeCurrent_uA = 20000;
  profile.inputCurrentLimit = ILIMLevel::ILIM_150mA;
  profile.safetyTimer = SafetyTimerLimit::HOURS_3;
  profile.use2xSafetyTimer = false;
  profile.ledOnWhenChargeDone = true;

  if (!charger.applyChargeProfile(profile)) {
    Serial.println("Failed to apply charge profile");
    while (1) { delay(1000); }
  }

  prevChargeEnabled = charger.isChargeEnabled();
  Serial.println("Manual PG control active.");
  Serial.println("Send 'c' in Serial Monitor to clear flags and reset done latch.");
}

void loop() {
  if (!updateManualPGIndicator()) {
    Serial.println("Failed to update PG indicator");
  }

  if (Serial.available()) {
    char cmd = (char)Serial.read();
    if (cmd == 'c' || cmd == 'C') {
      charger.ClearAllFlags();
      chargeDoneLatched = false;
      Serial.println("Flags cleared, done latch reset.");
    }
  }

  Serial.print("Charge enabled: ");
  Serial.print(charger.isChargeEnabled() ? "yes" : "no");
  Serial.print(" | done latched: ");
  Serial.print(chargeDoneLatched ? "yes" : "no");
  Serial.print(" | PG pulled down: ");
  Serial.println(charger.isPGEnabled() ? "yes" : "no");

  delay(2000);
}
