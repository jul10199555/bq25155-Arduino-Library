#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin
static constexpr BatteryChemistry BQ_CHEM = LI_ION_4V2;

bq25155 charger;

static void printMilli(const char *label, uint32_t mV) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(mV);
  Serial.println(" mV");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM, BQ_CHEM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  Serial.print(charger.getDeviceIDString());
  Serial.println(" found!");

  // Configure charging profile
  ChargeProfile profile;
  profile.chargeVoltage_mV = 4200;
  profile.enableFastCharge = true;
  profile.chargeCurrent_uA = 150000;
  profile.prechargeCurrent_uA = 30000;
  profile.inputCurrentLimit = ILIMLevel::ILIM_150mA;
  profile.safetyTimer = SafetyTimerLimit::HOURS_6;
  profile.use2xSafetyTimer = false;
  if (!charger.applyChargeProfile(profile)) {
    Serial.println("Failed to apply charge profile");
    while (1) { delay(1000); }
  }
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  // ADC sampling for voltage readings
  charger.ADC1sSamp();
  charger.EnableVBATADCCh();
  charger.EnableVINADCCh();
}

void loop() {
  bool chargeDisabled = false;
  if (!charger.enforceSafetyFaultPolicy(&chargeDisabled)) {
    Serial.println("Safety policy check failed");
  } else if (chargeDisabled) {
    Serial.println("Safety policy disabled charging");
  }

  if (charger.is_CHARGE_DONE()) {
    Serial.println("Charge complete");
  } else if (charger.is_VIN_PGOOD()) {
    Serial.println("Input voltage good");
  } else {
    Serial.println("Input voltage not good");
  }

  printMilli("Input Voltage", charger.readVIN(0));
  printMilli("Battery Voltage", charger.readVBAT(0));

  Serial.println("--------------------");
  delay(2000);
}
