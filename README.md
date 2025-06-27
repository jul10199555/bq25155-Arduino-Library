<a id="readme-top"></a>
# bq25155 Arduino Library #

[![Version Badge][rel-ver]][release]
[![License: MIT][lic-shield]][license]

A robust and fully-featured Arduino library for interfacing with the **Texas Instruments [bq25155](https://www.ti.com/product/BQ25155)** battery charger IC via I2C.  
Built with embedded safety in mind, this library offers full control over all registers, configuration of charging parameters, real-time monitoring, and low-power mode operation.

Supports full register-level access, interrupt masking, and safe flag reading/caching.

## ✨ Features ##

- 📡 I2C communication with smart **LPM pin toggling** (VIN-independent)
- 🔁 Full read/write access to **all configuration registers**
- 💾 **Caching support** for one-time-read FLAGx registers
- 🧪 Detect charger and battery faults
- 📦 Fully modular and non-blocking
- 🔋 Read internal ADCs
- ⚙️ **Charge settings:**
	- Target voltage (3600–4600 mV)
	- Fast charge and pre-charge current (1.25–500 mA)
	- Termination current (1–31%)
- 🔌 **Input Current Limits** (50 mA–600 mA)
- ⏱️ **Safety Timers:**
	- 1.5 h, 3 h, 6 h, 12 h
	- or **disabled**
- ❌ **Disable or enable termination detection**
- 📉 **Under-voltage lockout (UVLO) thresholds**
- 🔒 **Interrupt masking (MASK0–MASK3)** for selective notifications
- 🔎 **Monitoring and Fault Detection**
	- 🧪 Read real-time charger status and fault flags:
		- FLAG0 to FLAG3, with per-bit cached status
	- 📈 Read **internal ADCs** for VBAT, VBUS, TS, and system status
	- 🔥 Thermal and safety fault detection
- 💤 LPM (Low Power Mode) pin handling for communication without VIN

## Getting Started ## 

### Dependencies ###

- Arduino Environment
- I2C (`Wire`)
- Tested with a nRF52840

### Installation ###

Clone or download this repository and place it in your Arduino `libraries/` folder.

```bash
git clone https://github.com/jul10199555/bq25155-Arduino-Library.git
```

### Example ###

```cpp
#include "bq25155.h"

bq25155 charger;

void setup() {
    Serial.begin(115200);
    if (charger.begin(2, 3, 4)) {
        Serial.println("bq25155 OK");
        charger.setChgSafetyTimerto3h();
    }
}

void loop() {
    delay(1000);
}
```

[lic-shield]: https://img.shields.io/badge/License-MIT-yellow.svg
[license]: https://github.com/jul10199555/bq25155-Arduino-Library/blob/main/LICENSE

[rel-ver]: https://img.shields.io/badge/-v1.0.0-green
[release]: https://github.com/jul10199555/bq25155-Arduino-Library/releases