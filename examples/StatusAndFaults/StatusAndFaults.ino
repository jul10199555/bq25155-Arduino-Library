#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin
static constexpr BatteryChemistry BQ_CHEM = LI_ION_4V2;

bq25155 charger;

static void printFlags(const uint8_t *flags) {
  Serial.print("FLAG0/1/2/3 counts: ");
  Serial.print(flags[0]); Serial.print(" ");
  Serial.print(flags[1]); Serial.print(" ");
  Serial.print(flags[2]); Serial.print(" ");
  Serial.println(flags[3]);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM, BQ_CHEM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }
  // Require 2 consecutive severe samples to trip, 2 clear samples to re-arm.
  if (!charger.setFaultAutoDisableFilter(2, 2)) {
    Serial.println("Invalid fault filter configuration");
    while (1) { delay(1000); }
  }

  Serial.print(charger.getDeviceIDString());
  Serial.println(" found!");

  ChargeProfile profile;
  profile.chargeVoltage_mV = 4200;
  profile.enableFastCharge = true;
  profile.chargeCurrent_uA = 100000;
  profile.prechargeCurrent_uA = 20000;
  profile.inputCurrentLimit = ILIMLevel::ILIM_150mA;
  profile.safetyTimer = SafetyTimerLimit::HOURS_3;
  profile.use2xSafetyTimer = false;
  if (!charger.applyChargeProfile(profile)) {
    Serial.println("Failed to apply charge profile");
    while (1) { delay(1000); }
  }
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  // Clear any stale flags at startup
  charger.ClearAllFlags();
}

void loop() {
  // Read and cache all flags in one shot (RC registers)
  charger.readAllFLAGS();

  uint8_t faults[4] = {0, 0, 0, 0};
  charger.FaultsDetected(faults);
  printFlags(faults);

  if (charger.CHRG_DONE_Flag()) {
    Serial.println("FLAG: Charge done");
  }
  if (charger.VIN_OVP_Flag()) {
    Serial.println("FLAG: VIN over-voltage");
  }
  if (charger.BAT_OCP_Flag()) {
    Serial.println("FLAG: Battery over-current");
  }
  if (charger.TS_HOT_Flag() || charger.TS_COLD_Flag()) {
    Serial.println("FLAG: TS hot/cold");
  }

  // Live status bits (non-clearing)
  if (charger.is_CHRG_CV()) {
    Serial.println("STAT: Charging in CV mode");
  }
  if (charger.is_THERMREG_EN()) {
    Serial.println("STAT: Thermal regulation active");
  }
  if (charger.is_VIN_PGOOD()) {
    Serial.println("STAT: VIN power good");
  } else {
    Serial.println("STAT: VIN not good");
  }

  // Policy helper is non-destructive by default and uses cached FLAG values plus STAT bits.
  bool chargeDisabled = false;
  if (!charger.enforceSafetyFaultPolicy(&chargeDisabled)) {
    Serial.println("Safety policy check failed");
  } else if (chargeDisabled) {
    Serial.println("Safety policy disabled charging due to severe fault");
  }

  Serial.println("--------------------");
  delay(2000);
}
