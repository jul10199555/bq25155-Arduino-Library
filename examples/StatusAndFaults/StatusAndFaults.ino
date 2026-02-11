#include <Wire.h>
#include "bq25155.h"

// Pin setup
static constexpr uint8_t BQ_CHEN = 2;  // Charge enable pin
static constexpr uint8_t BQ_INT  = 5;  // Interrupt pin (open-drain)
static constexpr uint8_t BQ_LPM  = 20; // Low power mode pin

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

  if (!charger.begin(BQ_CHEN, BQ_INT, BQ_LPM)) {
    Serial.println("bq25155 not found!");
    while (1) { delay(1000); }
  }

  Serial.print(charger.getDeviceIDString());
  Serial.println(" found!");

  charger.initCHG(
    4200,    // Charge voltage in mV
    true,    // Enable fast charge
    100000,  // Charge current in uA
    20000,   // Pre-charge current in uA
    150,     // Input limit in mA
    3        // Safety timer request in hours
  );
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

  Serial.println("--------------------");
  delay(2000);
}
