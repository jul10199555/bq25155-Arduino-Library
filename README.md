<a id="readme-top"></a>
# bq25155 Arduino Library

[![Version Badge][rel-ver]][release]
[![License: MIT][lic-shield]][license]

Arduino library for the **Texas Instruments [bq25155](https://www.ti.com/product/BQ25155)** battery charger and power-path IC (I2C).
This library provides register-level access plus convenience functions for charge configuration,
status/fault monitoring, ADC reads, and low-power control.

## Features

- I2C control with LPM pin handling (access even when VIN is not present)
- Full access to configuration/status/flag/mask registers
- Cached FLAGx reads (clear-on-read) for safe fault handling
- Charge configuration:
  - VBAT regulation voltage (3.6 V to 4.6 V, 10 mV steps)
  - Fast charge and pre-charge current
  - Termination current (1% to 31% of ICHG)
- Input current limit (50 mA to 600 mA)
- Safety timer (3 h, 6 h, 12 h, or disabled)
- ADC reads for VIN/PMID/VBAT/TS/ADCIN/IIN/ICHG
- LDO or Load Switch control (0.6 V to 3.7 V, 100 mV steps)

## Important semantics

- Safety timer: the hardware supports 3/6/12 hours (or disabled). The 2x slow mode only
  applies outside CC/CV regulation, so effective time can be longer than the base setting.
- MR reset warning helper: `setRstWarnTimerms(ms)` chooses the closest warning offset based
  on the current HW reset timer (MR_HW_RESET).

## Getting Started

### Dependencies

- Arduino core
- Wire (I2C)

### Installation

Clone this repository into your Arduino `libraries/` folder.

```bash
git clone https://github.com/jul10199555/bq25155-Arduino-Library.git
```

### Basic Example

```cpp
#include "bq25155.h"

bq25155 charger;

void setup() {
    Serial.begin(115200);
    if (charger.begin(2, 5, 20)) {
        // Configure charger: 4.2 V, 100 mA, 3 h safety timer
        charger.initCHG(4200, true, 100000, 25000, 150, 3);
    }
}

void loop() {
    delay(1000);
}
```

## Examples

- `examples/BasicSetup` - initialize the device and apply a charge profile
- `examples/BasicUsage` - read basic status and voltage ADCs
- `examples/StatusAndFaults` - flag caching and fault/status decoding
- `examples/AdcMonitoring` - full ADC channel reads
- `examples/PowerAndLdo` - LDO/load switch control
- `examples/ThermistorAndTS` - TS thresholds and JEITA behavior

[lic-shield]: https://img.shields.io/badge/License-MIT-yellow.svg
[license]: https://github.com/jul10199555/bq25155-Arduino-Library/blob/main/LICENSE

[rel-ver]: https://img.shields.io/badge/-v1.0.3-green
[release]: https://github.com/jul10199555/bq25155-Arduino-Library/releases
