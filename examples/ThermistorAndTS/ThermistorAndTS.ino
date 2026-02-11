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

  // Enable TS function and JEITA-style control.
  charger.EnableTS();
  charger.CustomTSControl();
  // Safety timer supports 3/6/12 hours (0 disables); 2x slow only applies outside CC/CV.
  // setRstWarnTimerms(ms) selects the closest MR warning offset based on current HW reset timer.

  // Configure TS thresholds (0-1200 mV range).
  charger.setTSVAL(1000, bq25155_const::REG_TS_COLD);
  charger.setTSVAL(900,  bq25155_const::REG_TS_COOL);
  charger.setTSVAL(600,  bq25155_const::REG_TS_WARM);
  charger.setTSVAL(500,  bq25155_const::REG_TS_HOT);

  // Reduce charge voltage by 100 mV in WARM region.
  charger.setTSVCHG(100);

  // Reduce charge current to 0.5x in COOL/WARM regions.
  charger.setTSICHG(500); // 500 = 0.500x

  Serial.println("TS thresholds configured.");
}

void loop() {
  Serial.print("TS COLD threshold: ");
  Serial.print(charger.getTSVAL(bq25155_const::REG_TS_COLD));
  Serial.println(" mV");
  Serial.print("TS COOL threshold: ");
  Serial.print(charger.getTSVAL(bq25155_const::REG_TS_COOL));
  Serial.println(" mV");
  Serial.print("TS WARM threshold: ");
  Serial.print(charger.getTSVAL(bq25155_const::REG_TS_WARM));
  Serial.println(" mV");
  Serial.print("TS HOT threshold: ");
  Serial.print(charger.getTSVAL(bq25155_const::REG_TS_HOT));
  Serial.println(" mV");

  Serial.println("--------------------");
  delay(5000);
}
