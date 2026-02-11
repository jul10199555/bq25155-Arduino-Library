#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin

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

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  // Use small currents to avoid ADC saturation in this example.
  charger.initCHG(
    4200,    // Charge voltage in mV
    false,   // Disable fast charge (smaller steps)
    10000,   // Charge current in uA
    5000,    // Pre-charge current in uA
    50,      // Input limit in mA
    3        // Safety timer request in hours
  );
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  // Enable ADC channels and 1s sampling.
  charger.ADC1sSamp();
  charger.EnableAllADCCh();
}

void loop() {
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
