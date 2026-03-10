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

static void printMilli(const char *label, uint32_t mV) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(mV);
  Serial.println(" mV");
}

static void printMicroAsMilli(const char *label, uint32_t uA) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(uA / 1000);
  Serial.println(" mA");
}

static void printPercentScaled(const char *label, uint32_t percentScaled) {
  Serial.print(label);
  Serial.print(": ");
  Serial.print(percentScaled / 1000);
  Serial.print(".");
  uint32_t frac = percentScaled % 1000;
  if (frac < 100) Serial.print("0");
  if (frac < 10) Serial.print("0");
  Serial.print(frac);
  Serial.println(" %");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); }

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM, BQ_CHEM, BQ_USE_PG_LED)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  // Use small currents to avoid ADC saturation in this example.
  ChargeProfile profile;
  profile.chargeVoltage_mV = 4200;
  profile.enableFastCharge = false; // smaller steps
  profile.chargeCurrent_uA = 10000;
  profile.prechargeCurrent_uA = 5000;
  profile.inputCurrentLimit = ILIMLevel::ILIM_50mA;
  profile.safetyTimer = SafetyTimerLimit::HOURS_3;
  profile.use2xSafetyTimer = false;
  profile.ledOnWhenChargeDone = BQ_LED_ON_WHEN_DONE;
  if (!charger.applyChargeProfile(profile)) {
    Serial.println("Failed to apply charge profile");
    while (1) { delay(1000); }
  }
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  // Enable ADC channels and 1s sampling.
  charger.ADC1sSamp();
  charger.EnableAllADCCh();
}

void loop() {
  charger.enforceSafetyFaultPolicy();

  printMilli("VIN", charger.readVIN(0));
  printMilli("PMID", charger.readPMID(0));
  printMilli("VBAT", charger.readVBAT(0));
  printMilli("TS", charger.readTS(0));
  printMilli("ADCIN", charger.readADCIN(0));

  printMicroAsMilli("IIN", charger.readIIN(0));
  printPercentScaled("ICHG (percent of ICHG setting)", charger.readICHG(0));

  Serial.println("--------------------");
  delay(2000);
}
