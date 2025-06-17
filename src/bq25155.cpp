#include "bq25155.h"

bq25155::bq25155() : _i2cPort(&Wire), _i2cAddress(bq25155_ADDR) {}
bq25155::bq25155(TwoWire *wire, uint8_t address) : _i2cPort(wire), _i2cAddress(address) {}

bool bq25155::begin(uint8_t CHEN_pin, uint8_t INT_pin, uint8_t LPM_pin) {
    if (CHEN_pin == 0xFF || INT_pin == 0xFF || LPM_pin == 0xFF) { return false; }

    this->_CHEN_pin = CHEN_pin;
    this->_INT_pin  = INT_pin;
    this->_LPM_pin  = LPM_pin;

    pinMode(this->_LPM_pin, OUTPUT); // Set LPM output pin

    digitalWrite(this->_LPM_pin, HIGH); // HIGH to allow I2C communication when VIN is not present
    
    _i2cPort->begin(); // Initialize I2C communication
    _i2cPort->beginTransmission(_i2cAddress);
    
    if (_i2cPort->endTransmission() == 0) {
        // Check if device is 8b00110101 = bq25155
        if (getDeviceID() != DEVICE_ID_DEF)
            return false;

        pinMode(this->_INT_pin, INPUT); // Set input the Interruption pin (Future Update, integrate Interruption on MCU)
        pinMode(this->_CHEN_pin, OUTPUT); // Set CHGEN output pin
        
        digitalWrite(this->_CHEN_pin, HIGH); // HIGH to Disable charging, Wait for initCHG() command
        digitalWrite(this->_LPM_pin, LOW); // LOW to disable I2C communication when VIN is not present.

        return true;
    } else
        return false;
}

bool bq25155::initCHG(uint16_t BATVoltage_mV, bool En_FSCHG, uint32_t CHGCurrent_uA, uint32_t PCHGCurrent_uA, 
                      uint16_t inputCurrentLimit_mA, uint8_t ChgSftyTimer_hours) {
    // --- Apply Settings (using new register mappings) ---
    if(!setChargeVoltage(BATVoltage_mV)) { return false; }
    if(En_FSCHG)
        { if(!EnableFastCharge()) { return false; } }
    else
        { if(!DisableFastCharge()) { return false; } }
    if(!setChargeCurrent(CHGCurrent_uA)) { return false; }
    if(!setPreChargeCurrent(PCHGCurrent_uA)) { return false; }

    if(inputCurrentLimit_mA < 75)
        setILIMto50mA();
    else if(inputCurrentLimit_mA < 125)
        setILIMto100mA();
    else if(inputCurrentLimit_mA < 175)
        setILIMto150mA();
    else if(inputCurrentLimit_mA < 250)
        setILIMto200mA();
    else if(inputCurrentLimit_mA < 350)
        setILIMto300mA();
    else if(inputCurrentLimit_mA < 450)
        setILIMto400mA();
    else if(inputCurrentLimit_mA < 550)
        setILIMto500mA();
    else if(inputCurrentLimit_mA < 650)
        setILIMto600mA();
    else
        setILIMto150mA(); // default suggested option for small batteries

    if(ChgSftyTimer_hours == 0)
        DisableChgSafetyTimer();
    else if(ChgSftyTimer_hours < 25)
        setChgSafetyTimerto1_5h();
    else if(ChgSftyTimer_hours < 45)
        setChgSafetyTimerto3h();
    else if(ChgSftyTimer_hours < 90)
        setChgSafetyTimerto6h();
    else if(ChgSftyTimer_hours < 150)
        setChgSafetyTimerto12h();
    
    return EnableCharge();
}

bool bq25155::writeRegister(uint8_t reg, uint8_t value) {
    digitalWrite(this->_LPM_pin, HIGH); // HIGH to allow I2C communication when VIN is not present

    _i2cPort->beginTransmission(_i2cAddress);
    _i2cPort->write(reg);
    _i2cPort->write(value);
    uint8_t finishedi2c = _i2cPort->endTransmission();
    
    digitalWrite(this->_LPM_pin, LOW); // LOW to disable I2C communication when VIN is not present.

    // Returns true if I2C write succeeded, otherwise false 
    return (finishedi2c == 0);
}

uint8_t bq25155::readRegister(uint8_t reg) {
    digitalWrite(this->_LPM_pin, HIGH); // HIGH to allow I2C communication when VIN is not present

    _i2cPort->beginTransmission(_i2cAddress);
    _i2cPort->write(reg);
    _i2cPort->endTransmission(false); // Send restart
    _i2cPort->requestFrom(_i2cAddress, (size_t)1);

    uint8_t availablei2c = _i2cPort->available();
    if (availablei2c){
        uint8_t valuei2c = _i2cPort->read();
        digitalWrite(this->_LPM_pin, LOW); // LOW to disable I2C communication when VIN is not present.

        return valuei2c;
    }
    
    return 0x00; // Return 0x00 if no data or error present
}

// --- Begin Helper Functions for Value Conversion ---
// Reads 16-bit values (MSB and LSB)
uint16_t bq25155::readRaw16BitRegister(uint8_t msb_reg, uint8_t lsb_reg) {
    uint8_t msb = readRegister(msb_reg);
    uint8_t lsb = readRegister(lsb_reg);
    // The 12-bit ADC result is left-justified in a 16-bit register.
    return ((uint16_t)msb << 8) | lsb;
}

uint16_t bq25155::KeepDecimals(uint32_t value, uint8_t digits) {
    if (digits == 0) return 0;

    if (digits > 5) digits = 5;

    uint32_t factor = 1;
    for (uint8_t i = 1; i < digits; ++i) {
        factor *= 10;
    }

    uint32_t rounded = (value / factor) * factor;
    if (rounded > 65534) return 65535;

    return (uint16_t)rounded;
}

uint16_t bq25155::GenADCVRead(uint8_t ADC_DATA_MSB, uint8_t ADC_DATA_LSB, uint8_t KeepDec, bool VRef) {
    uint16_t ADC_Reading = readRaw16BitRegister(ADC_DATA_MSB, ADC_DATA_LSB);
    
    uint16_t scale = (VRef ? 6000 : 1200);

    uint32_t mVolts = ((uint32_t)ADC_Reading * scale) / MAX16BIT;

    return KeepDecimals(mVolts, KeepDec);
}

uint32_t bq25155::GenADCIRead(uint8_t ADC_DATA_MSB, uint8_t ADC_DATA_LSB, uint8_t KeepDec) {
    // Needs implementation: IIN reading only valid when VIN > VUVLO and VIN < VOVP
    uint16_t ADC_Reading = readRaw16BitRegister(ADC_DATA_MSB, ADC_DATA_LSB);

    uint32_t scale = (getILIM() > 2) ? 750000UL : 375000UL;

    uint32_t uAmps = ((uint32_t)ADC_Reading * scale) / MAX16BIT;

    return KeepDecimals(uAmps, KeepDec);
}

uint32_t bq25155::GenADCIPRead(uint8_t ADC_DATA_MSB, uint8_t ADC_DATA_LSB, uint8_t KeepDec) {
    // Needs implementation: Where ICHARGE is the charge current setting.
    // Note that if the device is in pre-charge or in the TS COLD region,
    // ICHARGE will be the current set by the IPRECHRG and TS_ICHRG bits respectively
    uint16_t ADC_Reading = readRaw16BitRegister(ADC_DATA_MSB, ADC_DATA_LSB);

    // Scale: 100% / (0.8 × 65536) ≈ 100000 / 52429
    // Keeps 3 implied decimal digits: 12345 > 12.345%
    uint32_t percent_scaled = ((uint32_t)ADC_Reading * 100000UL) / 52429;

    return KeepDecimals(percent_scaled, KeepDec);
}
// --- End Helper Functions for Value Conversion ---

// --- STATUS Registers ---

// --- Begin STAT0 Register - Charger Status ---
bool bq25155::is_CHRG_CV() { return (readRegister(REG_STAT_0) & CHRG_CV_STAT_MASK) != 0; }
bool bq25155::is_CHARGE_DONE() { return (readRegister(REG_STAT_0) & CHARGE_DONE_STAT_MASK) != 0; }
bool bq25155::is_IINLIM_EN() { return (readRegister(REG_STAT_0) & IINLIM_ACTIVE_STAT_MASK) != 0; }
bool bq25155::is_VDPPM_EN() { return (readRegister(REG_STAT_0) & VDPPM_ACTIVE_STAT_MASK) != 0; }
bool bq25155::is_VINDPM_EN() { return (readRegister(REG_STAT_0) & VINDPM_ACTIVE_STAT_MASK) != 0; }
bool bq25155::is_THERMREG_EN() { return (readRegister(REG_STAT_0) & THERMREG_ACTIVE_STAT_MASK) != 0; }
bool bq25155::is_VIN_PGOOD() { return (readRegister(REG_STAT_0) & VIN_PGOOD_STAT_MASK) != 0; }
// --- End STAT0 Register - Charger Status ---

// --- Begin STAT1 Register - Charger Status ---
bool bq25155::is_VIN_OVP_TRIG() { return (readRegister(REG_STAT_1) & VIN_OVP_FAULT_STAT_MASK) != 0; }
bool bq25155::is_BAT_OCP_TRIG() { return (readRegister(REG_STAT_1) & BAT_OCP_FAULT_STAT_MASK) != 0; }
bool bq25155::is_BAT_UVLO() { return (readRegister(REG_STAT_1) & BAT_UVLO_FAULT_STAT_MASK) != 0; }
bool bq25155::is_BAT_COLD() { return (readRegister(REG_STAT_1) & TS_COLD_STAT_MASK) != 0; }
bool bq25155::is_BAT_COOL() { return (readRegister(REG_STAT_1) & TS_COOL_STAT_MASK) != 0; }
bool bq25155::is_BAT_WARM() { return (readRegister(REG_STAT_1) & TS_WARM_STAT_MASK) != 0; }
bool bq25155::is_BAT_HOT() { return (readRegister(REG_STAT_1) & TS_HOT_STAT_MASK) != 0; }
bool bq25155::is_CHG_SUSPENDED() { return (is_BAT_COLD() || is_BAT_HOT()); }
// --- End STAT1 Register - Charger Status ---

// --- Begin STAT2 Register - ADC Status ---
bool bq25155::is_Alarm_TRIG(uint8_t AlarmCh) {
    uint8_t mask = 0x00;

    switch (AlarmCh) {
        case 1: mask = COMP1_ALARM_STAT_MASK; break;
        case 2: mask = COMP2_ALARM_STAT_MASK; break;
        case 3: mask = COMP3_ALARM_STAT_MASK; break;
        default: return false;
    }

    return (readRegister(REG_STAT_2) & mask) != 0;
}

// if VTS>0.9 = TRUE, VTS<0.9 = FALSE
bool bq25155::is_TS_OPEN() { return (readRegister(REG_STAT_2) & TS_OPEN_STAT_MASK) != 0; }
// --- End STAT2 Register - ADC Status ---

// --- FLAG Registers ---
// Read FLAGs to only clear them all at once
void bq25155::ClearAllFlags() {
    // We just need to read to clear 'em all.
    readRegister(REG_FLAG_0);
    readRegister(REG_FLAG_1);
    readRegister(REG_FLAG_2);
    readRegister(REG_FLAG_3);
}

// Reads and caches all FLAG registers at once
void bq25155::readAllFLAGS() {
    cachedFlag0 = readRegister(REG_FLAG_0);
    cachedFlag1 = readRegister(REG_FLAG_1);
    cachedFlag2 = readRegister(REG_FLAG_2);
    cachedFlag3 = readRegister(REG_FLAG_3);
}

// Count how many flags are triggered per FLAG register.
void bq25155::FaultsDetected(uint8_t* faultsOut) {
    uint8_t faultsReg[4] = {
        cachedFlag0,
        cachedFlag1,
        cachedFlag2,
        cachedFlag3
    };

    // Initialize output array to zero.
    for (uint8_t i = 0; i < 4; i++) {
        faultsOut[i] = 0;
    }

    // Count how many bits are set (flags triggered) per register.
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t b = 0; b < 8; b++) {
            if (faultsReg[i] & (1 << b))
                faultsOut[i]++;
        }
    }
}

// --- Begin FLAG0 Register - Charger Status ---
// Once FLAG0 is readed, its values are cleared.
uint8_t bq25155::readFLAG0() {
    cachedFlag0 = readRegister(REG_FLAG_0);
    return cachedFlag0;
}

// Get cached FLAG0 (not triggers a hardware read)
uint8_t bq25155::getCachedFLAG0() { return cachedFlag0; }
// 1b0 = not detected, 1b1 = operation detected
// Triggered when charger enters Constant Voltage operation.
bool bq25155::CHRG_CV_Flag() { return (cachedFlag0 & CHRG_CV_FLAG_MASK) != 0; }
// Triggered when charger reaches termination.
bool bq25155::CHRG_DONE_Flag() { return (cachedFlag0 & CHARGE_DONE_FLAG_MASK) != 0; }
// Triggered when Input Current Limit loop is active.
bool bq25155::IINLIM_Flag() { return (cachedFlag0 & IINLIM_ACTIVE_FLAG_MASK) != 0; }
// Triggered when DPPM loop is active.
bool bq25155::VDPPM_Flag() { return (cachedFlag0 & VDPPM_ACTIVE_FLAG_MASK) != 0; }
// Triggered when VINDPM loop is active.
bool bq25155::VINDPM_Flag() { return (cachedFlag0 & VINDPM_ACTIVE_FLAG_MASK) != 0; }
// Triggered when Thermal Charge Current Foldback (Thermal Regulation) loop is active.
bool bq25155::THERMREG_Flag() { return (cachedFlag0 & THERMREG_ACTIVE_FLAG_MASK) != 0; }
// Triggered when VIN changes PGOOD status.
bool bq25155::VIN_PGOOD_Flag() { return (cachedFlag0 & VIN_PGOOD_FLAG_MASK) != 0; }
// --- End FLAG0 Register - Charger Status ---

// --- Begin FLAG1 Register - Battery Status ---
// Once FLAG1 is readed, its values are cleared.
uint8_t bq25155::readFLAG1() {
    cachedFlag1 = readRegister(REG_FLAG_1);
    return cachedFlag1;
}

// Get cached FLAG1 (not triggers a hardware read)
uint8_t bq25155::getCachedFLAG1() { return cachedFlag1; }
// 1b0 = no condition detected, 1b1 = condition detected
// Triggered when VIN > VOVP.
bool bq25155::VIN_OVP_Flag() { return (cachedFlag1 & VIN_OVP_FAULT_FLAG_MASK) != 0; }
// Triggered when IBAT > IBATOCP.
bool bq25155::BAT_OCP_Flag() { return (cachedFlag1 & BAT_OCP_FAULT_FLAG_MASK) != 0; }
// Triggered when VBAT < VBATUVLO.
bool bq25155::BAT_UVLO_Flag() { return (cachedFlag1 & BAT_UVLO_FAULT_FLAG_MASK) != 0; }
// 1b0 = Entry not detected, 1b1 = Entry detected
// Triggered when VTS > VTS_COLD.
bool bq25155::TS_COLD_Flag() { return (cachedFlag1 & TS_COLD_FLAG_MASK) != 0; }
// Triggered when VTS_COLD > VTS > VTS_COOL.
bool bq25155::TS_COOL_Flag() { return (cachedFlag1 & TS_COOL_FLAG_MASK) != 0; }
// Triggered when VTS_HOT < VTS < VTS_WARM.
bool bq25155::TS_WARM_Flag() { return (cachedFlag1 & TS_WARM_FLAG_MASK) != 0; }
// Triggered when VTS < VHOT.
bool bq25155::TS_HOT_Flag() { return (cachedFlag1 & TS_HOT_FLAG_MASK) != 0; }
// --- End FLAG1 Register - Battery Status ---

// --- Begin FLAG2 Register - ADC Status ---
// Once FLAG2 is readed, its values are cleared.
uint8_t bq25155::readFLAG2() {
    cachedFlag2 = readRegister(REG_FLAG_2);
    return cachedFlag2;
}

// Get cached FLAG2 (not triggers a hardware read)
uint8_t bq25155::getCachedFLAG2() { return cachedFlag2; }
// 1b0 = No crossing detected, 1b1 = measurement crossed condition set
// Triggered when ADC conversion is completed.
bool bq25155::ADC_READY_Flag() { return (cachedFlag2 & ADC_READY_FLAG_MASK) != 0; }
// Triggered when ADC measurement meets programmed condition.
bool bq25155::ADC_Alarm_1_Flag() { return (cachedFlag2 & COMP1_ALARM_FLAG_MASK) != 0; }
// Triggered when ADC measurement meets programmed condition.
bool bq25155::ADC_Alarm_2_Flag() { return (cachedFlag2 & COMP2_ALARM_FLAG_MASK) != 0; }
// Triggered when ADC measurement meets programmed condition.
bool bq25155::ADC_Alarm_3_Flag() { return (cachedFlag2 & COMP3_ALARM_FLAG_MASK) != 0; }
//1b0 = No fault detected, 1b1 = Fault detected
// Triggered when VTS > VTS_OPEN
bool bq25155::VTS_OPEN_Flag() { return (cachedFlag2 & TS_OPEN_FLAG_MASK) != 0; }
// --- End FLAG2 Register - ADC Status ---

// --- Begin FLAG3 Register - Timers & MR Status ---
// Once FLAG3 is readed, its values are cleared.
uint8_t bq25155::readFLAG3() {
    cachedFlag3 = readRegister(REG_FLAG_3);
    return cachedFlag3;
}

// Get cached FLAG3 (not triggers a hardware read)
uint8_t bq25155::getCachedFLAG3() { return cachedFlag3;}
// 1b0 = Timer not expired, 1b1 = Timer expired
// Triggered when I2C watchdog timer expires.
bool bq25155::WD_FAULT_Flag() { return (cachedFlag3 & WD_FAULT_FLAG_MASK) != 0; }
// Set when safety timer expires. Cleared after VIN or CE toggles.
bool bq25155::SAFETY_TMR_Flag() { return (cachedFlag3 & SAFETY_TMR_FAULT_FLAG_MASK) != 0; }
// Set when LDO output current exceeds OCP condition.
bool bq25155::LDO_OCP_Flag() { return (cachedFlag3 & LDO_OCP_FAULT_FLAG_MASK) != 0; }
// Set when MR is low for at least tWAKE1.
bool bq25155::MRWAKE1_TMR_Flag() { return (cachedFlag3 & MRWAKE1_TIMEOUT_FLAG_MASK) != 0; }
// Set when MR is low for at least tWAKE2.
bool bq25155::MRWAKE2_TMR_Flag() { return (cachedFlag3 & MRWAKE2_TIMEOUT_FLAG_MASK) != 0; }
// Set when MR is low for at least tRESETWARN.
bool bq25155::MRST_WARN_Flag() { return (cachedFlag3 & MRRESET_WARN_FLAG_MASK) != 0; }
// --- End FLAG3 Register - Timers & MR Status ---

// --- MASK Registers ---
// --- Begin MASK0 Register - Charger Status ---
// 1b0 = Interrupt Not Masked, 1b1 = Interrupt Masked
bool bq25155::get_CHRG_CV_MASK() { return (readRegister(REG_MASK_0) & CHRG_CV_MASK) != 0; }
bool bq25155::set_CHRG_CV_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_0); r = m ? (r | CHRG_CV_MASK) : (r & ~CHRG_CV_MASK);
    return writeRegister(REG_MASK_0, r);
}

bool bq25155::get_CHARGE_DONE_MASK() { return (readRegister(REG_MASK_0) & CHARGE_DONE_MASK) != 0; }
bool bq25155::set_CHARGE_DONE_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_0); r = m ? (r | CHARGE_DONE_MASK) : (r & ~CHARGE_DONE_MASK);
    return writeRegister(REG_MASK_0, r);
}

bool bq25155::get_IINLIM_ACTIVE_MASK() { return (readRegister(REG_MASK_0) & IINLIM_ACTIVE_MASK) != 0; }
bool bq25155::set_IINLIM_ACTIVE_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_0); r = m ? (r | IINLIM_ACTIVE_MASK) : (r & ~IINLIM_ACTIVE_MASK);
    return writeRegister(REG_MASK_0, r);
}

bool bq25155::get_VDPPM_ACTIVE_MASK() { return (readRegister(REG_MASK_0) & VDPPM_ACTIVE_MASK) != 0; }
bool bq25155::set_VDPPM_ACTIVE_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_0); r = m ? (r | VDPPM_ACTIVE_MASK) : (r & ~VDPPM_ACTIVE_MASK);
    return writeRegister(REG_MASK_0, r);
}

bool bq25155::get_VINDPM_ACTIVE_MASK() { return (readRegister(REG_MASK_0) & VINDPM_ACTIVE_MASK) != 0; }
bool bq25155::set_VINDPM_ACTIVE_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_0); r = m ? (r | VINDPM_ACTIVE_MASK) : (r & ~VINDPM_ACTIVE_MASK);
    return writeRegister(REG_MASK_0, r);
}

bool bq25155::get_THERMREG_ACTIVE_MASK() { return (readRegister(REG_MASK_0) & THERMREG_ACTIVE_MASK) != 0; }
bool bq25155::set_THERMREG_ACTIVE_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_0); r = m ? (r | THERMREG_ACTIVE_MASK) : (r & ~THERMREG_ACTIVE_MASK);
    return writeRegister(REG_MASK_0, r);
}

bool bq25155::get_VIN_PGOOD_MASK() { return (readRegister(REG_MASK_0) & VIN_PGOOD_MASK) != 0; }
bool bq25155::set_VIN_PGOOD_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_0); r = m ? (r | VIN_PGOOD_MASK) : (r & ~VIN_PGOOD_MASK);
    return writeRegister(REG_MASK_0, r);
}
// --- End MASK0 Register - Charger Status ---

// --- Begin MASK1 Register - BAT Status ---
// 1b0 = Interrupt Not Masked, 1b1 = Interrupt Masked
bool bq25155::get_VIN_OVP_FAULT_MASK() { return (readRegister(REG_MASK_1) & VIN_OVP_FAULT_MASK) != 0; }
bool bq25155::set_VIN_OVP_FAULT_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_1); r = m ? (r | VIN_OVP_FAULT_MASK) : (r & ~VIN_OVP_FAULT_MASK);
    return writeRegister(REG_MASK_1, r);
}

bool bq25155::get_BAT_OCP_FAULT_MASK() { return (readRegister(REG_MASK_1) & BAT_OCP_FAULT_MASK) != 0; }
bool bq25155::set_BAT_OCP_FAULT_MASK(bool m){
    uint8_t r = readRegister(REG_MASK_1); r = m ? (r | BAT_OCP_FAULT_MASK) : (r & ~BAT_OCP_FAULT_MASK);
    return writeRegister(REG_MASK_1, r);
}

bool bq25155::get_BAT_UVLO_FAULT_MASK() { return (readRegister(REG_MASK_1) & BAT_UVLO_FAULT_MASK) != 0; }
bool bq25155::set_BAT_UVLO_FAULT_MASK(bool m){
    uint8_t r = readRegister(REG_MASK_1); r = m ? (r | BAT_UVLO_FAULT_MASK) : (r & ~BAT_UVLO_FAULT_MASK);
    return writeRegister(REG_MASK_1, r);
}

bool bq25155::get_TS_COLD_MASK() { return (readRegister(REG_MASK_1) & TS_COLD_MASK) != 0; }
bool bq25155::set_TS_COLD_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_1); r = m ? (r | TS_COLD_MASK) : (r & ~TS_COLD_MASK);
    return writeRegister(REG_MASK_1, r);
}

bool bq25155::get_TS_COOL_MASK() { return (readRegister(REG_MASK_1) & TS_COOL_MASK) != 0; }
bool bq25155::set_TS_COOL_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_1); r = m ? (r | TS_COOL_MASK) : (r & ~TS_COOL_MASK);
    return writeRegister(REG_MASK_1, r);
}

bool bq25155::get_TS_WARM_MASK() { return (readRegister(REG_MASK_1) & TS_WARM_MASK) != 0; }
bool bq25155::set_TS_WARM_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_1); r = m ? (r | TS_WARM_MASK) : (r & ~TS_WARM_MASK);
    return writeRegister(REG_MASK_1, r);
}

bool bq25155::get_TS_HOT_MASK() { return (readRegister(REG_MASK_1) & TS_HOT_MASK) != 0; }
bool bq25155::set_TS_HOT_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_1); r = m ? (r | TS_HOT_MASK) : (r & ~TS_HOT_MASK);
    return writeRegister(REG_MASK_1, r);
}
// --- End MASK1 Register - BAT Status ---

// --- Begin MASK2 Register - ADC Status ---
// 1b0 = Interrupt Not Masked, 1b1 = Interrupt Masked
bool bq25155::get_ADC_READY_MASK() { return (readRegister(REG_MASK_2) & ADC_READY_MASK) != 0; }
bool bq25155::set_ADC_READY_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_2); r = m ? (r | ADC_READY_MASK) : (r & ~ADC_READY_MASK);
    return writeRegister(REG_MASK_2, r);
}

bool bq25155::get_COMP1_ALARM_MASK() { return (readRegister(REG_MASK_2) & COMP1_ALARM_MASK) != 0; }
bool bq25155::set_COMP1_ALARM_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_2); r = m ? (r | COMP1_ALARM_MASK) : (r & ~COMP1_ALARM_MASK);
    return writeRegister(REG_MASK_2, r);
}

bool bq25155::get_COMP2_ALARM_MASK() { return (readRegister(REG_MASK_2) & COMP2_ALARM_MASK) != 0; }
bool bq25155::set_COMP2_ALARM_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_2); r = m ? (r | COMP2_ALARM_MASK) : (r & ~COMP2_ALARM_MASK);
    return writeRegister(REG_MASK_2, r);
}

bool bq25155::get_COMP3_ALARM_MASK() { return (readRegister(REG_MASK_2) & COMP3_ALARM_MASK) != 0; }
bool bq25155::set_COMP3_ALARM_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_2); r = m ? (r | COMP3_ALARM_MASK) : (r & ~COMP3_ALARM_MASK);
    return writeRegister(REG_MASK_2, r);
}

bool bq25155::get_TS_OPEN_MASK() { return (readRegister(REG_MASK_2) & TS_OPEN_MASK) != 0; }
bool bq25155::set_TS_OPEN_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_2); r = m ? (r | TS_OPEN_MASK) : (r & ~TS_OPEN_MASK);
    return writeRegister(REG_MASK_2, r);
}
// --- End MASK2 Register - ADC Status ---

// --- Begin MASK3 Register - Timers & MR Status ---
// 1b0 = Interrupt Not Masked, 1b1 = Interrupt Masked
bool bq25155::get_WD_FAULT_MASK() { return (readRegister(REG_MASK_3) & WD_FAULT_MASK) != 0; }
bool bq25155::set_WD_FAULT_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_3); r = m ? (r | WD_FAULT_MASK) : (r & ~WD_FAULT_MASK);
    return writeRegister(REG_MASK_3, r);
}

bool bq25155::get_SAFETY_TMR_FAULT_MASK() { return (readRegister(REG_MASK_3) & SAFETY_TMR_FAULT_MASK) != 0; }
bool bq25155::set_SAFETY_TMR_FAULT_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_3); r = m ? (r | SAFETY_TMR_FAULT_MASK) : (r & ~SAFETY_TMR_FAULT_MASK);
    return writeRegister(REG_MASK_3, r);
}

bool bq25155::get_LDO_OCP_FAULT_MASK() { return (readRegister(REG_MASK_3) & LDO_OCP_FAULT_MASK) != 0; }
bool bq25155::set_LDO_OCP_FAULT_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_3); r = m ? (r | LDO_OCP_FAULT_MASK) : (r & ~LDO_OCP_FAULT_MASK);
    return writeRegister(REG_MASK_3, r);
}

bool bq25155::get_MRWAKE1_TIMEOUT_MASK() { return (readRegister(REG_MASK_3) & MRWAKE1_TIMEOUT_MASK) != 0; }
bool bq25155::set_MRWAKE1_TIMEOUT_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_3); r = m ? (r | MRWAKE1_TIMEOUT_MASK) : (r & ~MRWAKE1_TIMEOUT_MASK);
    return writeRegister(REG_MASK_3, r);
}

bool bq25155::get_MRWAKE2_TIMEOUT_MASK() { return (readRegister(REG_MASK_3) & MRWAKE2_TIMEOUT_MASK) != 0; }
bool bq25155::set_MRWAKE2_TIMEOUT_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_3); r = m ? (r | MRWAKE2_TIMEOUT_MASK) : (r & ~MRWAKE2_TIMEOUT_MASK);
    return writeRegister(REG_MASK_3, r);
}

bool bq25155::get_MRRESET_WARN_MASK() { return (readRegister(REG_MASK_3) & MRRESET_WARN_MASK) != 0; }
bool bq25155::set_MRRESET_WARN_MASK(bool m) {
    uint8_t r = readRegister(REG_MASK_3); r = m ? (r | MRRESET_WARN_MASK) : (r & ~MRRESET_WARN_MASK);
    return writeRegister(REG_MASK_3, r);
}
// --- End MASK3 Register - Timers & MR Status ---

// --- V & I Charge Control Registers ---

// --- Begin VBAT charging Register ---
uint16_t bq25155::getChargeVoltage() {
    uint8_t VBATREG = readRegister(REG_VBAT_CTRL); // Read VBAT_CTRL
    VBATREG &= VBAT_REG_MASK; // Mask bits 6:0

    uint16_t vbat_mv = 3600 + (VBATREG * 10); // VBATREG = 3.6 V + vbat_bits x 10 mV

    return vbat_mv;
}

bool bq25155::setChargeVoltage(uint16_t target_mV) {
    if (target_mV < 3600) { return false; }

    // Calculate new VBAT_REG value
    uint8_t vbat_bits = (target_mV - 3600) / 10; // VBATREG = 3.6 V + vbat_bits x 10 mV
    // If a value greater than 4.6 V is written, keep 4.6 V (datasheet Vmax)
    if (vbat_bits > 100) { vbat_bits = 100; }

    uint8_t VBATREG = readRegister(REG_VBAT_CTRL); // Read VBAT_CTRL

    VBATREG &= ~VBAT_REG_MASK; // Clear bits 6:0
    VBATREG |= vbat_bits; // Set bits 6:0

    return writeRegister(REG_VBAT_CTRL, VBATREG);
}
// --- End VBAT charging Register ---

// --- Begin Fast Charge Settings ---
bool bq25155::isFastChargeEnabled() { return (readRegister(REG_PCHRGCTRL) & ICHARGE_RANGE_MASK) != 0; }
bool bq25155::DisableFastCharge() {
    uint8_t ICHG_STEP = readRegister(REG_PCHRGCTRL); // Read PCHRGCTRL
    ICHG_STEP &= ~ICHARGE_RANGE_MASK; // 1b0 = 1.25 mA step (318.75 mA max charge current)
    return writeRegister(REG_PCHRGCTRL, ICHG_STEP);
}
bool bq25155::EnableFastCharge() {
    uint8_t ICHG_STEP = readRegister(REG_PCHRGCTRL); // Read PCHRGCTRL
    ICHG_STEP |= ICHARGE_RANGE_MASK; // 1b1 = 2.5 mA step (500 mA max charge current)
    return writeRegister(REG_PCHRGCTRL, ICHG_STEP);
}
// --- End Fast Charge Settings ---

// --- Begin Charging Current Settings ---
uint32_t bq25155::getChargeCurrent() {
    uint8_t ICHGbits = readRegister(REG_ICHG_CTRL);
    uint32_t current_uA = 0;

    if (isFastChargeEnabled()) {
        // 500,000 µA max charge current = 200 * 2500
        current_uA = ICHGbits * 2500;
        if (current_uA > 500000)
            current_uA = 500000;
    } else {
        // 318,750 µA max charge current = 255 * 1250
        current_uA = ICHGbits * 1250;
        if (current_uA > 318750)
            current_uA = 318750;
    }

    return current_uA;
}

bool bq25155::setChargeCurrent(uint32_t current_uA) {
    uint8_t Ibits = 0;

    if (isFastChargeEnabled()) {
        // 500,000 µA max charge current = 200 * 2500
        if (current_uA >= 500000)
            Ibits = 200;
        else
            Ibits = current_uA / 2500;
    } else {
        // 318,750 µA max charge current = 255 * 1250
        if (current_uA >= 318750)
            Ibits = 255;
        else
            Ibits = current_uA / 1250;
    }
    
    // idk what happens if 0
    if (Ibits < 1) Ibits = ICHG_CTRL_DEF;

    return writeRegister(REG_ICHG_CTRL, Ibits);
}
// --- End Charging Current Settings ---

// --- Begin Pre-Charging Current Settings ---
uint32_t bq25155::getPrechargeCurrent() {
    uint8_t IPCHGbits = readRegister(REG_PCHRGCTRL) & IPRECHG_MASK;
    uint32_t current_uA = 0;

    if (isFastChargeEnabled()) {
        // 77,500 µA max pre-charge current = 31 * 2500
        current_uA = IPCHGbits * 2500;
        if (current_uA > 77500)
            current_uA = 77500;
    } else {
        // 38,750 µA max pre-charge current = 31 * 1250
        current_uA = IPCHGbits * 1250;
        if (current_uA > 38750)
            current_uA = 38750;
    }

    return current_uA;
}

bool bq25155::setPreChargeCurrent(uint32_t current_uA) {
    uint8_t IPCHGbits = readRegister(REG_PCHRGCTRL);
    uint8_t Ibits = 0;

    if (isFastChargeEnabled()) {
        // 77,500 µA max pre-charge current = 31 * 2500
        if (current_uA >= 77500)
            Ibits = 31;
        else
            Ibits = current_uA / 2500;
    } else {
        // 38,750 µA max pre-charge current = 31 * 1250
        if (current_uA >= 38750)
            Ibits = 31;
        else
            Ibits = current_uA / 1250;
    }
    
    // idk what happens if 0
    if (Ibits < 1) Ibits = IPRECHG_DEF;

    IPCHGbits &= ~IPRECHG_MASK; // Clear b4:0
    IPCHGbits |= Ibits; // Set new bits

    return writeRegister(REG_PCHRGCTRL, IPCHGbits);
}
// --- End Pre-Charging Current Settings ---

// --- Begin Termination Current Settings ---
uint8_t bq25155::getITERM() { return (readRegister(REG_TERMCTRL) & ITERM_MASK) >> 1; } // Extract bits 5:1
bool bq25155::setITERM(uint8_t percent) {
    if (percent > 31) percent = 31;  // Range = 1% to 31% of ICHRG
    
    percent <<= 1;

    uint8_t r = readRegister(REG_TERMCTRL);
    
    r &= ~ITERM_MASK; // Clear bits 5:1
    r |= percent; // Set new ITERM bits

    return writeRegister(REG_TERMCTRL, r);
}

bool bq25155::isTermEnabled() { return (readRegister(REG_TERMCTRL) & TERM_DISABLE_MASK) == 0; }
bool bq25155::EnableTermination() {
    uint8_t r = readRegister(REG_TERMCTRL);
    r &= ~TERM_DISABLE_MASK; //1b0 = Termination Enabled
    return writeRegister(REG_TERMCTRL, r);
}
bool bq25155::DisableTermination() {
    uint8_t r = readRegister(REG_TERMCTRL);
    r |= TERM_DISABLE_MASK; //1b1 = Termination Disabled
    return writeRegister(REG_TERMCTRL, r);
}

// --- End Pre-Charging Current Settings ---

// --- Begin Over Current & Under Voltage Thresholds Settings ---
// false = 3.0 V, true = 2.8 V
bool bq25155::getVLOWThrs() { return (readRegister(REG_BUVLO) & VLOWV_SEL_MASK) != 0; }
bool bq25155::setVLOWThrsTo3_0V() {
    uint8_t r = readRegister(REG_BUVLO);
    r &= ~VLOWV_SEL_MASK; // 1b0 = 3.0 V
    return writeRegister(REG_BUVLO, r);
}
bool bq25155::setVLOWThrsTo2_8V() {
    uint8_t r = readRegister(REG_BUVLO);
    r |= VLOWV_SEL_MASK; // 1b1 = 2.8 V
    return writeRegister(REG_BUVLO, r);
}

uint8_t bq25155::getIBATOCP() { return (readRegister(REG_BUVLO) & IBAT_OCP_ILIM_MASK) >> 3; }
bool bq25155::setIBATOCPto1200mA() {
    uint8_t r = readRegister(REG_BUVLO);
    
    r &= ~IBAT_OCP_ILIM_MASK; // Clear bits 4–3
    
    return writeRegister(REG_BUVLO, r);
}
bool bq25155::setIBATOCPto1500mA() {
    uint8_t r = readRegister(REG_BUVLO);
    
    r &= ~IBAT_OCP_ILIM_MASK; // Clear bits 4–3
    r |= (IBAT_OCP_1500MA << 3);
    
    return writeRegister(REG_BUVLO, r);
}
bool bq25155::DisableIBATOCP() {
    uint8_t r = readRegister(REG_BUVLO);
    
    r &= ~IBAT_OCP_ILIM_MASK; // Clear bits 4–3
    r |= (IBAT_OCP_DIS << 3);
    
    return writeRegister(REG_BUVLO, r);
}

uint8_t bq25155::getUVLO() { return (readRegister(REG_BUVLO) & BUVLO_MASK); }
bool bq25155::setUVLO(uint8_t code) {
    if (code > 7) return false;  // Only 3 bits allowed

    uint8_t r = readRegister(REG_BUVLO);

    r &= ~BUVLO_MASK;
    r |= code;

    return writeRegister(REG_BUVLO, r);
}
bool bq25155::setUVLOto3000mV() { return setUVLO(BUVLO_3_0V); }
bool bq25155::setUVLOto2800mV() { return setUVLO(BUVLO_2_8V); }
bool bq25155::setUVLOto2600mV() { return setUVLO(BUVLO_2_6V); }
bool bq25155::setUVLOto2400mV() { return setUVLO(BUVLO_2_4V); }
bool bq25155::setUVLOto2200mV() { return setUVLO(BUVLO_2_2V); }
bool bq25155::DisableUVLO() { return setUVLO(BUVLO_DIS); }

// --- End Over Current & Under Voltage Thresholds Settings ---

// --- Begin CHARGERCTRL0 Settings ---
// TS Function Enable (bit 7)
bool bq25155::isTSEnabled() { return (readRegister(REG_CHARGERCTRL0) & TS_EN_MASK) != 0; }
bool bq25155::DisableTS() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r &= ~TS_EN_MASK; // 1b0 = TS function disabled (Only charge control is disabled. TS_OPEN detection and TS ADC monitoring stays enabled)
    return writeRegister(REG_CHARGERCTRL0, r);
}
bool bq25155::EnableTS() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r |= TS_EN_MASK; // 1b1 = TS function enabled
    return writeRegister(REG_CHARGERCTRL0, r);
}

// TS Function Control Mode (bit 6)
bool bq25155::getTSControlMode() { return (readRegister(REG_CHARGERCTRL0) & TS_CONTROL_MODE_MASK) != 0; }
bool bq25155::CustomTSControl() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r &= ~TS_CONTROL_MODE_MASK; // 1b0 = Custom (JEITA)
    return writeRegister(REG_CHARGERCTRL0, r);
}
bool bq25155::DisableTSControl() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r |= TS_CONTROL_MODE_MASK; // 1b1 = Disable charging on HOT/COLD Only
    return writeRegister(REG_CHARGERCTRL0, r);
}

// VRH_THRESH (bit 5)
bool bq25155::getVRHThrs() { return (readRegister(REG_CHARGERCTRL0) & VRH_THRESH_MASK) != 0; }
bool bq25155::setVRHThrsTo140mV() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r &= ~VRH_THRESH_MASK; // 1b0 = 140 mV
    return writeRegister(REG_CHARGERCTRL0, r);
}
bool bq25155::setVRHThrsTo200mV() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r |= VRH_THRESH_MASK; // 1b1 = 200 mV
    return writeRegister(REG_CHARGERCTRL0, r);
}

// WATCHDOG_DISABLE (bit 4)
bool bq25155::isWatchdogTEnabled() { return (readRegister(REG_CHARGERCTRL0) & WATCHDOG_DISABLE_MASK) == 0; }
bool bq25155::EnableWatchdogT() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r &= ~WATCHDOG_DISABLE_MASK; // 1b0 = Watchdog timer enabled
    return writeRegister(REG_CHARGERCTRL0, r);
}
bool bq25155::DisableWatchdogT() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r |= WATCHDOG_DISABLE_MASK; // 1b1 = Watchdog timer disabled
    return writeRegister(REG_CHARGERCTRL0, r);
}

// 2XTMR_EN (bit 3)
bool bq25155::getSafetyTimerX() { return (readRegister(REG_CHARGERCTRL0) & SFT_2XTMR_EN_MASK) != 0; }
bool bq25155::set1xSafetyTimer() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r &= ~SFT_2XTMR_EN_MASK; // 1b0 = The timer is not slowed at any time
    return writeRegister(REG_CHARGERCTRL0, r);
}
bool bq25155::set2xSafetyTimer() {
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    r |= SFT_2XTMR_EN_MASK; // 1b1 = The timer is slowed by 2x when in any control other than CC or CV
    return writeRegister(REG_CHARGERCTRL0, r);
}

// SAFETY_TIMER_LIMIT (bits 2:1)
uint8_t bq25155::getChgSafetyTimer() {
    uint8_t reg = readRegister(REG_CHARGERCTRL0);
    bool is2x = (reg & SFT_2XTMR_EN_MASK) != 0;
    uint8_t timerCode = (reg & SAFETY_TIMER_LIMIT_MASK) >> 1;

    uint8_t baseTenths = 0;

    switch (timerCode) {
        case SAFETY_TIMER_LIMIT_3H:  baseTenths = 30; break;
        case SAFETY_TIMER_LIMIT_6H:  baseTenths = 60; break;
        case SAFETY_TIMER_LIMIT_12H: baseTenths = 120; break;
        case SAFETY_TIMER_LIMIT_DIS: return 0; // Disabled
        default: return 0;
    }
    // Returns timer in tenths of an hour (e.g., 15 = 1.5h, 30 = 3h)
    return is2x ? (baseTenths / 2) : baseTenths;
}
bool bq25155::setChgSafetyTimer(uint8_t code) {
    if (code > 3) return false;
    uint8_t r = readRegister(REG_CHARGERCTRL0);
    code <<= 1;
    r &= ~SAFETY_TIMER_LIMIT_MASK;
    r |= code;
    return writeRegister(REG_CHARGERCTRL0, r);
}
bool bq25155::setChgSafetyTimerto1_5h() { return set2xSafetyTimer() && setChgSafetyTimer(SAFETY_TIMER_LIMIT_3H); }
bool bq25155::setChgSafetyTimerto3h() { return set1xSafetyTimer() && setChgSafetyTimer(SAFETY_TIMER_LIMIT_3H); }
bool bq25155::setChgSafetyTimerto6h() { return set1xSafetyTimer() && setChgSafetyTimer(SAFETY_TIMER_LIMIT_6H); }
bool bq25155::setChgSafetyTimerto12h() { return set1xSafetyTimer() && setChgSafetyTimer(SAFETY_TIMER_LIMIT_12H); }
bool bq25155::DisableChgSafetyTimer() { return set1xSafetyTimer() && setChgSafetyTimer(SAFETY_TIMER_LIMIT_DIS); }
// --- End CHARGERCTRL0 Settings ---

// --- Begin CHARGERCTRL1 Settings ---
bool bq25155::isVINDPMEnabled() { return (readRegister(REG_CHARGERCTRL1) & VINDPM_DIS_MASK) == 0; }
bool bq25155::EnableVINDPM() {
    uint8_t r = readRegister(REG_CHARGERCTRL1);
    r &= ~VINDPM_DIS_MASK; // 1b0 = VINDPM Enabled
    return writeRegister(REG_CHARGERCTRL1, r);
}
bool bq25155::DisableVINDPM() {
    uint8_t r = readRegister(REG_CHARGERCTRL1);
    r |= VINDPM_DIS_MASK; // 1b1 = VINDPM Disabled
    return writeRegister(REG_CHARGERCTRL1, r);
}

uint8_t bq25155::getVINDPM() { return (readRegister(REG_CHARGERCTRL1) & VINPDM_MASK) >> 4; }
bool bq25155::setVINDPM(uint8_t code) {
    if (code > 7) return false;
    
    uint8_t r = readRegister(REG_CHARGERCTRL1);
    code <<= 4;

    r &= ~VINPDM_MASK;
    r |= code;
    return writeRegister(REG_CHARGERCTRL1, r);
}
bool bq25155::setVINDPMto4200mV() { return setVINDPM(VINDPM_4_2V); }
bool bq25155::setVINDPMto4300mV() { return setVINDPM(VINDPM_4_3V); }
bool bq25155::setVINDPMto4400mV() { return setVINDPM(VINDPM_4_4V); }
bool bq25155::setVINDPMto4500mV() { return setVINDPM(VINDPM_4_5V); }
bool bq25155::setVINDPMto4600mV() { return setVINDPM(VINDPM_4_6V); }
bool bq25155::setVINDPMto4700mV() { return setVINDPM(VINDPM_4_7V); }
bool bq25155::setVINDPMto4800mV() { return setVINDPM(VINDPM_4_8V); }
bool bq25155::setVINDPMto4900mV() { return setVINDPM(VINDPM_4_9V); }

bool bq25155::isDPPMEnabled() { return (readRegister(REG_CHARGERCTRL1) & DPPM_DIS_MASK) == 0; }
bool bq25155::EnableDPPM() {
    uint8_t r = readRegister(REG_CHARGERCTRL1);
    r &= ~DPPM_DIS_MASK; // 1b0 = DPPM function enabled
    return writeRegister(REG_CHARGERCTRL1, r);
}
bool bq25155::DisableDPPM() {
    uint8_t r = readRegister(REG_CHARGERCTRL1);
    r |= DPPM_DIS_MASK; // 1b1 = DPPM function disabled
    return writeRegister(REG_CHARGERCTRL1, r);
}

uint8_t bq25155::getThermalThreshold() { return (readRegister(REG_CHARGERCTRL1) & THERM_REG_MASK); }
bool bq25155::setThermalThreshold(uint8_t code) {
    if (code > 7) return false;
    
    uint8_t r = readRegister(REG_CHARGERCTRL1);
    
    r &= ~THERM_REG_MASK;
    r |= code;
    return writeRegister(REG_CHARGERCTRL1, r);
}
bool bq25155::setThermalTemperature(uint8_t Temp_C) {
    uint8_t code = 0;
    if (Temp_C < 1)
        code = THERM_THRS_DIS; // Disabled
    else if (Temp_C < 85)
        code = THERM_THRS_80C;
    else if(Temp_C < 90)
        code = THERM_THRS_85C;
    else if(Temp_C < 95)
        code = THERM_THRS_90C;
    else if(Temp_C < 100)
        code = THERM_THRS_95C;
    else if(Temp_C < 105)
        code = THERM_THRS_100C;
    else if(Temp_C < 110)
        code = THERM_THRS_105C;
    else if(Temp_C < 120)
        code = THERM_THRS_110C;
    else
        return false;

    return setThermalThreshold(code);
}
// --- End CHARGERCTRL1 Settings ---

// --- Begin ILIMCTRL Settings - Input Current Limit Level Selection ---
uint8_t bq25155::getILIM() { return (readRegister(REG_ILIMCTRL) & ILIM_MASK); }
bool bq25155::setILIM(uint8_t code) {
    if (code > 7) return false;

    uint8_t r = readRegister(REG_ILIMCTRL);

    r &= ~ILIM_MASK;
    r |= code;
    return writeRegister(REG_ILIMCTRL, r);
}
bool bq25155::setILIMto50mA() { return setILIM(ILIM_50MA); }
bool bq25155::setILIMto100mA() { return setILIM(ILIM_100MA); }
bool bq25155::setILIMto150mA() { return setILIM(ILIM_150MA); }
bool bq25155::setILIMto200mA() { return setILIM(ILIM_200MA); }
bool bq25155::setILIMto300mA() { return setILIM(ILIM_300MA); }
bool bq25155::setILIMto400mA() { return setILIM(ILIM_400MA); }
bool bq25155::setILIMto500mA() { return setILIM(ILIM_500MA); }
bool bq25155::setILIMto600mA() { return setILIM(ILIM_600MA); }
// --- End ILIMCTRL Settings - Input Current Limit Level Selection ---

// --- Begin LDOCTRL Settings - LDO / Load Switch Configuration ---
bool bq25155::isLSLDOEnabled() { return (readRegister(REG_LDOCTRL) & EN_LS_LDO_MASK) != 0; }
bool bq25155::DisableLSLDO() {
    uint8_t r = readRegister(REG_LDOCTRL);
    r &= ~EN_LS_LDO_MASK; // 1b0 = Disable LS/LDO
    return writeRegister(REG_LDOCTRL, r);
}
bool bq25155::EnableLSLDO() {
    uint8_t r = readRegister(REG_LDOCTRL);
    r |= EN_LS_LDO_MASK; // 1b1 = Enable LS/LDO
    return writeRegister(REG_LDOCTRL, r);
}

uint16_t bq25155::getLDOVoltage() {
    uint8_t VLDO_bits = readRegister(REG_LDOCTRL); // Read REG_LDOCTRL
    VLDO_bits &= VLDO_MASK; // Mask bits 6:2
    VLDO_bits >>= 2; // Right-shift 2 bits

    uint16_t vldo_mv = 600 + (VLDO_bits * 100); // VLDO = 600 mV + VLDO_bits x 100 mV

    return vldo_mv;
}

bool bq25155::setLDOVoltage(uint16_t target_mV) {
    if (target_mV < 600 || target_mV > 3700) { return false; }

    // Calculate new VLDOREG value
    uint8_t VLDO_bits = (target_mV - 600) / 100; // VLDOREG = 600 mV + (VLDO_bits x 100 mV)
    // If a value greater than 3700 mV is written, keep 3700 mV  (datasheet VLDOmax)
    if (VLDO_bits > 31)
        VLDO_bits = 31;

    VLDO_bits <<= 2; // Left-shift 2 bits

    uint8_t VLDOREG = readRegister(REG_LDOCTRL); // Read REG_LDOCTRL

    VLDOREG &= ~VLDO_MASK; // Clear bits 6:2
    VLDOREG |= VLDO_bits; // Set bits 6:2

    return writeRegister(REG_LDOCTRL, VLDOREG);
}

bool bq25155::isLDOEnabled() { return (readRegister(REG_LDOCTRL) & LDO_SWITCH_CONFG_MASK) == 0; }
bool bq25155::isLSEnabled() { return (readRegister(REG_LDOCTRL) & LDO_SWITCH_CONFG_MASK) != 0; }
bool bq25155::EnableLDO() {
    uint8_t r = readRegister(REG_LDOCTRL);
    r &= ~LDO_SWITCH_CONFG_MASK; // 1b0 = LDO
    return writeRegister(REG_LDOCTRL, r);
}
bool bq25155::EnableLS() {
    uint8_t r = readRegister(REG_LDOCTRL);
    r |= LDO_SWITCH_CONFG_MASK; // 1b1 = Load Switch
    return writeRegister(REG_LDOCTRL, r);
}
// --- End LDOCTRL Settings - LDO / Load Switch Configuration ---

// --- Begin MRCTRL Settings - MR behavior ---
bool bq25155::isMRResetVINGated() { return (readRegister(REG_MRCTRL) & MR_RESET_VIN_MASK) != 0; }
bool bq25155::DisableMRResetVINGated() {
    uint8_t r = readRegister(REG_MRCTRL);
    r &= ~MR_RESET_VIN_MASK; // 1b0 = Reset sent when /MR reset time is met regardless of VIN state
    return writeRegister(REG_MRCTRL, r);
}
bool bq25155::EnableMRResetVINGated() {
    uint8_t r = readRegister(REG_MRCTRL);
    r |= MR_RESET_VIN_MASK; //1b1 = Reset sent when /MR reset is met and Vin is valid
    return writeRegister(REG_MRCTRL, r);
}

bool bq25155::getWake1Timer() { return (readRegister(REG_MRCTRL) & MR_WAKE1_TIMER_MASK) != 0; }
bool bq25155::setWake1TimerTo125ms() {
    uint8_t r = readRegister(REG_MRCTRL);
    r &= ~MR_WAKE1_TIMER_MASK; // 1b0 = 125 ms
    return writeRegister(REG_MRCTRL, r);
}
bool bq25155::setWake1TimerTo500ms() {
    uint8_t r = readRegister(REG_MRCTRL);
    r |= MR_WAKE1_TIMER_MASK; // 1b1 = 500 ms
    return writeRegister(REG_MRCTRL, r);
}

bool bq25155::getWake2Timer() { return (readRegister(REG_MRCTRL) & MR_WAKE2_TIMER_MASK) != 0; }
bool bq25155::setWake2TimerTo1s() {
    uint8_t r = readRegister(REG_MRCTRL);
    r &= ~MR_WAKE2_TIMER_MASK; // 1b0 = 1 s
    return writeRegister(REG_MRCTRL, r);
}
bool bq25155::setWake2TimerTo2s() {
    uint8_t r = readRegister(REG_MRCTRL);
    r |= MR_WAKE2_TIMER_MASK; // 1b1 = 2 s
    return writeRegister(REG_MRCTRL, r);
}

uint8_t bq25155::getRstWarnTimer() { return (readRegister(REG_MRCTRL) & MR_RESET_WARN_MASK) >> 3; }
bool bq25155::setRstWarnTimer(uint8_t code) {
    if (code > 3) return false;
    uint8_t r = readRegister(REG_MRCTRL);

    code <<= 3;

    r &= ~MR_RESET_WARN_MASK;
    r |= code;

    return writeRegister(REG_MRCTRL, r);
}
bool bq25155::setRstWarnTimerms(uint16_t mrst_ms) {
    uint8_t code = 0;
    if (mrst_ms < 750)
        code = MR_WARN_HW_0_5S;
    else if (mrst_ms < 1250)
        code = MR_WARN_HW_1_0S;
    else if (mrst_ms < 1750)
        code = MR_WARN_HW_1_5S;
    else if (mrst_ms < 2250)
        code = MR_WARN_HW_2_0S;
    else
        return false;

    return setRstWarnTimer(code);
}

uint8_t bq25155::getHWRstTimer() { return (readRegister(REG_MRCTRL) & MR_HW_RESET_MASK) >> 1; }
bool bq25155::setHWRstTimer(uint8_t code) {
    if (code > 3) return false;
    uint8_t r = readRegister(REG_MRCTRL);

    code <<= 1;

    r &= ~MR_HW_RESET_MASK;
    r |= code;

    return writeRegister(REG_MRCTRL, r);
}
bool bq25155::setHWRstTimerto4s() { return setHWRstTimer(MR_HW_RESET_4S); }
bool bq25155::setHWRstTimerto8s() { return setHWRstTimer(MR_HW_RESET_8S); }
bool bq25155::setHWRstTimerto10s() { return setHWRstTimer(MR_HW_RESET_10S); }
bool bq25155::setHWRstTimerto14s() { return setHWRstTimer(MR_HW_RESET_14S); }
// --- End MRCTRL Settings - MR behavior ---

// --- Begin ICCTRL0 Settings - Reset & MR behavior ---
bool bq25155::isShipModeSet() { return (readRegister(REG_ICCTRL0) & EN_SHIP_MODE_MASK) != 0; }
bool bq25155::DisableShipMode() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r &= ~EN_SHIP_MODE_MASK; // 1b0 = Normal operation
    return writeRegister(REG_ICCTRL0, r);
}
bool bq25155::EnableShipMode() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r |= EN_SHIP_MODE_MASK; // 1b1 = Enter Ship Mode when VIN is not valid and /MR is high
    return writeRegister(REG_ICCTRL0, r);
}

uint8_t bq25155::getAutoWKPTimer() { return (readRegister(REG_ICCTRL0) & AUTOWAKE_MASK) >> 4; }
bool bq25155::setAutoWKPTimer(uint8_t code) {
    if (code > 3) return false;
    uint8_t r = readRegister(REG_ICCTRL0);

    code <<= 4;

    r &= ~AUTOWAKE_MASK;
    r |= code;

    return writeRegister(REG_ICCTRL0, r);
}
bool bq25155::setAutoWKPto0_6s() { return setAutoWKPTimer(AUTOWAKE_0_6S); }
bool bq25155::setAutoWKPto1_2s() { return setAutoWKPTimer(AUTOWAKE_1_2S); }
bool bq25155::setAutoWKPto2_4s() { return setAutoWKPTimer(AUTOWAKE_2_4S); }
bool bq25155::setAutoWKPto5s() { return setAutoWKPTimer(AUTOWAKE_5_0S); }

bool bq25155::AreGlobalIntMasksEnabled() { return (readRegister(REG_ICCTRL0) & GLOBAL_INT_MASK) != 0; }
bool bq25155::DisableGlobalIntMasks() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r &= ~GLOBAL_INT_MASK; // 1b0 = Normal operation
    return writeRegister(REG_ICCTRL0, r);
}
bool bq25155::EnableGlobalIntMasks() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r |= GLOBAL_INT_MASK; // 1b1 = Mask all interrupts
    return writeRegister(REG_ICCTRL0, r);
}

bool bq25155::isHWResetEnabled() { return (readRegister(REG_ICCTRL0) & HW_RESET_MASK) != 0; }
bool bq25155::DisableHWReset() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r &= ~HW_RESET_MASK; // 1b0 = Normal operation
    return writeRegister(REG_ICCTRL0, r);
}
bool bq25155::EnableHWReset() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r |= HW_RESET_MASK; // 1b1 = HW Reset. Temporarily power down all power rails, except VDD. I2C Register go to default settings.
    return writeRegister(REG_ICCTRL0, r);
}

bool bq25155::isSWResetEnabled() { return (readRegister(REG_ICCTRL0) & SW_RESET_MASK) != 0; }
bool bq25155::DisableSWReset() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r &= ~SW_RESET_MASK; // 1b0 = Normal operation
    return writeRegister(REG_ICCTRL0, r);
}
bool bq25155::EnableSWReset() {
    uint8_t r = readRegister(REG_ICCTRL0);
    r |= SW_RESET_MASK; // 1b1 = SW Reset. I2C Registers go to default settings.
    return writeRegister(REG_ICCTRL0, r);
}
// --- End ICCTRL0 Settings - Reset & MR behavior ---

// --- Begin ICCTRL1 Settings - MR behavior, ADCIN mode, PG mode, PMID mode ---
uint8_t bq25155::getMRLongPressAction() { return (readRegister(REG_ICCTRL1) & MR_LPRESS_ACTION_MASK) >> 6; }
// 2b00 = HW Reset (Power Cycle)
bool bq25155::setMRLongPressAsHWRST() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~MR_LPRESS_ACTION_MASK;
    r |= (MR_LPRESS_HW_RST << 6);
    return writeRegister(REG_ICCTRL1, r);
}
// 2b10 = 2b11 = Enter Ship Mode
bool bq25155::setMRLongPressAsShipMode() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~MR_LPRESS_ACTION_MASK;
    r |= (MR_LPRESS_SHIPMODE << 6);
    return writeRegister(REG_ICCTRL1, r);
}
// 2b01 = Do nothing
bool bq25155::DisableMRLongPress() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~MR_LPRESS_ACTION_MASK;
    r |= (MR_LPRESS_DISABLE << 6);
    return writeRegister(REG_ICCTRL1, r);
}

bool bq25155::getADCINMode() { return (readRegister(REG_ICCTRL1) & ADCIN_MODE_MASK) != 0; }
bool bq25155::setADCINasGP() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~ADCIN_MODE_MASK; // 1b0 = General Purpose ADC input (no Internal biasing)
    return writeRegister(REG_ICCTRL1, r);
}
bool bq25155::setADCINasNTC() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r |= ADCIN_MODE_MASK; // 1b1 = 10K NTC ADC input (80 µA biasing)
    return writeRegister(REG_ICCTRL1, r);
}

uint8_t bq25155::getPGMode() { return (readRegister(REG_ICCTRL1) & PG_MODE_MASK) >> 2; }
// 2b00 = VIN Power Good
bool bq25155::setPGasVINPG() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~PG_MODE_MASK;
    r |= (PG_MODE_VIN << 2);
    return writeRegister(REG_ICCTRL1, r);
}
// 2b01 = Deglitched Level Shifted /MR
bool bq25155::setPGasDegMR() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~PG_MODE_MASK;
    r |= (PG_MODE_DEG_MR << 2);
    return writeRegister(REG_ICCTRL1, r);
}
// 2b11 = 2b10 = General Purpose Open Drain Output
bool bq25155::setPGasGPOD() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~PG_MODE_MASK;
    r |= (PG_MODE_GP_ODO << 2);
    return writeRegister(REG_ICCTRL1, r);
}

uint8_t bq25155::getPMIDMode() { return (readRegister(REG_ICCTRL1) & PMID_MODE_MASK); }
// 2b00 = PMID powered from BAT or VIN if present
bool bq25155::PwrPMIDwithVINorBAT() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~PMID_MODE_MASK;
    r |= PMID_BAT_OR_VIN;
    return writeRegister(REG_ICCTRL1, r);
}
// 2b01 = PMID powered from BAT only, even if VIN is present
bool bq25155::PwrPMIDwithBAT() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~PMID_MODE_MASK;
    r |= PMID_BAT_ONLY;
    return writeRegister(REG_ICCTRL1, r);
}
// 2b10 = PMID disconnected and left floating
bool bq25155::FloatPMID() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~PMID_MODE_MASK;
    r |= PMID_DIS_FLT;
    return writeRegister(REG_ICCTRL1, r);
}
// 2b11 = PMID disconnected and pulled down.
bool bq25155::PullDownPMID() {
    uint8_t r = readRegister(REG_ICCTRL1);
    r &= ~PMID_MODE_MASK;
    r |= PMID_DIS_PD;
    return writeRegister(REG_ICCTRL1, r);
}
// --- End ICCTRL1 Settings - MR behavior, ADCIN mode, PG mode, PMID mode ---

// --- Begin ICCTRL2 Settings - PMID Vset, PG stat, HWRST WD, CE set ---
uint8_t bq25155::getPMID() { return (readRegister(REG_ICCTRL2) & PMID_REG_CTRL_MASK) >> 5; }
bool bq25155::setPMID(uint8_t code) {
    if (code > 7) return false;

    uint8_t r = readRegister(REG_ICCTRL2);
    
    code <<= 5;

    r &= ~PMID_REG_CTRL_MASK;
    r |= code;
    return writeRegister(REG_ICCTRL2, r);
}
bool bq25155::setPMIDmV(uint16_t PMID_mV) {
    uint8_t code = 0;

    if (PMID_mV < 4300)
        code = PMID_BAT_TRACK;
    else if(PMID_mV < 4450)
        code = PMID_4_4V;
    else if(PMID_mV < 4550)
        code = PMID_4_5V;
    else if(PMID_mV < 4650)
        code = PMID_4_6V;
    else if(PMID_mV < 4750)
        code = PMID_4_7V;
    else if(PMID_mV < 4850)
        code = PMID_4_8V;
    else if(PMID_mV < 4950)
        code = PMID_4_9V;
    else if(PMID_mV < 5100)
        code = PMID_VIN_PTHR;
    else
        return false;

    return setPMID(code);
}

bool bq25155::isPGEnabled() { return (readRegister(REG_ICCTRL2) & GPO_PG_MASK) == 0; }
bool bq25155::EnablePG() {
    uint8_t r = readRegister(REG_ICCTRL2);
    r &= ~GPO_PG_MASK; // 1b0 = Pulled Down
    return writeRegister(REG_ICCTRL2, r);
}
bool bq25155::DisablePG() {
    uint8_t r = readRegister(REG_ICCTRL2);
    r |= GPO_PG_MASK; // 1b1 = High Z
    return writeRegister(REG_ICCTRL2, r);
}

bool bq25155::isRST14sWDEnabled() { return (readRegister(REG_ICCTRL2) & HWRESET_14S_WD_MASK) != 0; }
bool bq25155::DisableRST14sWD() {
    uint8_t r = readRegister(REG_ICCTRL2);
    r &= ~HWRESET_14S_WD_MASK; // 1b0 = Timer disabled
    return writeRegister(REG_ICCTRL2, r);
}
bool bq25155::EnableRST14sWD() {
    uint8_t r = readRegister(REG_ICCTRL2);
    r |= HWRESET_14S_WD_MASK; // 1b1 = Device will perform HW reset if no I2C transaction is done within 14s after VIN is present.
    return writeRegister(REG_ICCTRL2, r);
}

bool bq25155::isChargeEnabled() { return (readRegister(REG_ICCTRL2) & CHARGER_DISABLE_MASK) == 0; }
bool bq25155::EnableCharge() {
    digitalWrite(this->_CHEN_pin, LOW); // LOW to Enable charging
    uint8_t r = readRegister(REG_ICCTRL2);
    r &= ~CHARGER_DISABLE_MASK; // 1b0 = Charge enabled if /CE pin is low
    return writeRegister(REG_ICCTRL2, r);
}
bool bq25155::DisableCharge() {
    digitalWrite(this->_CHEN_pin, HIGH); // HIGH to Disable charging
    uint8_t r = readRegister(REG_ICCTRL2);
    r |= CHARGER_DISABLE_MASK; // 1b1 = Charge disabled
    return writeRegister(REG_ICCTRL2, r);
}
// --- End ICCTRL2 Settings - MR behavior, ADCIN mode, PG mode, PMID mode ---

// --- ADC Functions ---

// --- Begin REG_ADCCTRL0 Settings - ADC read & conversions params ---
uint8_t bq25155::getADCReadRate() { return (readRegister(REG_ADCCTRL0) & ADC_READ_RATE_MASK) >> 6; }
// 2b00 = Manual Read (Measurement done when ADC_CONV_START is set)
bool bq25155::ADCManualRead() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_READ_RATE_MASK;
    r |= ADC_READ_RATE_MANUAL;
    return writeRegister(REG_ADCCTRL0, r);
}
// 2b01 = Continuous
bool bq25155::ADCContinuousSamp() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_READ_RATE_MASK;
    r |= ADC_READ_RATE_CNTNS;
    return writeRegister(REG_ADCCTRL0, r);
}
// 2b10 = Every 1 second
bool bq25155::ADC1sSamp() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_READ_RATE_MASK;
    r |= ADC_READ_RATE_1S;
    return writeRegister(REG_ADCCTRL0, r);
}
// 2b11 = Every 1 minute
bool bq25155::ADC1mSamp() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_READ_RATE_MASK;
    r |= ADC_READ_RATE_1M;
    return writeRegister(REG_ADCCTRL0, r);
}

// ADC Conversion Start Trigger
bool bq25155::getADCConvStart() { return (readRegister(REG_ADCCTRL0) & ADC_CONV_START_MASK) == 0; }
bool bq25155::NoADCConv() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_CONV_START_MASK; // 1b0 = No ADC conversion
    return writeRegister(REG_ADCCTRL0, r);
}
// ******Bit goes back to 0 when conversion is complete******
bool bq25155::InitADCManualMeas() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r |= ADC_CONV_START_MASK; // 1b1 = Initiates ADC measurement in Manual Read operation
    return writeRegister(REG_ADCCTRL0, r);
}

uint8_t bq25155::getADCConvSpeed() { return (readRegister(REG_ADCCTRL0) & ADC_CONV_SPEED_MASK) >> 3; }
// 2b00 = 24 ms (highest accuracy) (datasheet p11, samp>6 = 12-bit read?)
bool bq25155::setADCSpeedTo24ms() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_CONV_SPEED_MASK;
    r |= ADC_CONV_SPEED_24MS;
    return writeRegister(REG_ADCCTRL0, r);
}
// 2b01 = 12 ms (datasheet p11, samp>6 = 12-bit read?)
bool bq25155::setADCSpeedTo12ms() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_CONV_SPEED_MASK;
    r |= ADC_CONV_SPEED_12MS;
    return writeRegister(REG_ADCCTRL0, r);
}
// 2b10 = 6 ms (datasheet p11, samp<6 = 10-bit read?)
bool bq25155::setADCSpeedTo6ms() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_CONV_SPEED_MASK;
    r |= ADC_CONV_SPEED_6MS;
    return writeRegister(REG_ADCCTRL0, r);
}
// 2b11 = 3 ms (datasheet p11, samp<6 = 10-bit read?)
bool bq25155::setADCSpeedTo3ms() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_CONV_SPEED_MASK;
    r |= ADC_CONV_SPEED_3MS;
    return writeRegister(REG_ADCCTRL0, r);
}

uint8_t bq25155::getADCComp1Ch() { return (readRegister(REG_ADCCTRL0) & ADC_COMP1_MASK); }
bool bq25155::setADCComp1Ch(uint8_t code) {
    if (code > 7) return false;
    
    uint8_t r = readRegister(REG_ADCCTRL0);
    
    r &= ~ADC_COMP1_MASK;
    r |= code;
    return writeRegister(REG_ADCCTRL0, r);
}
bool bq25155::DisableADCComp1Ch() {
    uint8_t r = readRegister(REG_ADCCTRL0);
    r &= ~ADC_COMP1_MASK;
    return writeRegister(REG_ADCCTRL0, r);
}
// --- End REG_ADCCTRL0 Settings - ADC read & conversions params ---

// --- Begin REG_ADCCTRL1 Settings - ADC Comparators Set ---
uint8_t bq25155::getADCComp2Ch() { return (readRegister(REG_ADCCTRL1) & ADC_COMP2_MASK) >> 5; }
bool bq25155::setADCComp2Ch(uint8_t code) {
    if (code > 7) return false;
    
    uint8_t r = readRegister(REG_ADCCTRL1);
    
    code <<= 5;

    r &= ~ADC_COMP2_MASK;
    r |= code;
    return writeRegister(REG_ADCCTRL1, r);
}
bool bq25155::DisableADCComp2Ch() {
    uint8_t r = readRegister(REG_ADCCTRL1);
    r &= ~ADC_COMP2_MASK;
    return writeRegister(REG_ADCCTRL1, r);
}

uint8_t bq25155::getADCComp3Ch() { return (readRegister(REG_ADCCTRL1) & ADC_COMP3_MASK) >> 2; }
bool bq25155::setADCComp3Ch(uint8_t code) {
    if (code > 7) return false;
    
    uint8_t r = readRegister(REG_ADCCTRL1);
    
    code <<= 2;

    r &= ~ADC_COMP3_MASK;
    r |= code;
    return writeRegister(REG_ADCCTRL1, r);
}
bool bq25155::DisableADCComp3Ch() {
    uint8_t r = readRegister(REG_ADCCTRL1);
    r &= ~ADC_COMP3_MASK;
    return writeRegister(REG_ADCCTRL1, r);
}
// --- End REG_ADCCTRL1 Settings - ADC Comparators Set ---

// --- Begin ADC Readings ---
uint16_t bq25155::readVIN(uint8_t Vdecims) { return GenADCVRead(REG_ADC_DATA_VIN_M, REG_ADC_DATA_VIN_L, Vdecims, 1); }
uint16_t bq25155::readPMID(uint8_t Vdecims) { return GenADCVRead(REG_ADC_DATA_PMID_M, REG_ADC_DATA_PMID_L, Vdecims, 1); }
uint32_t bq25155::readIIN(uint8_t Vdecims) { return GenADCIRead(REG_ADC_DATA_IIN_M, REG_ADC_DATA_IIN_L, Vdecims); }
uint16_t bq25155::readVBAT(uint8_t Vdecims) { return GenADCVRead(REG_ADC_DATA_VBAT_M, REG_ADC_DATA_VBAT_L, Vdecims, 1); }
uint16_t bq25155::readTS(uint8_t Vdecims) { return GenADCVRead(REG_ADC_DATA_TS_M, REG_ADC_DATA_TS_L, Vdecims, 0); }
uint16_t bq25155::readADCIN(uint8_t Vdecims) { return GenADCVRead(REG_ADC_DATA_ADCIN_M, REG_ADC_DATA_ADCIN_L, Vdecims, 0); }
uint32_t bq25155::readICHG(uint8_t Vdecims) { return GenADCIPRead(REG_ADC_DATA_ICHG_M, REG_ADC_DATA_ICHG_L, Vdecims); }
// --- End ADC Readings ---

// --- Begin ADCALARM_COMPx Settings - ADC Comparators Values ---
uint16_t bq25155::readADCAlarms(uint8_t ADCAlarmCh, bool AlarmVal) {
    uint8_t Alarm_MSB = 0;
    uint8_t Alarm_LSB = 0;

    switch (ADCAlarmCh) {
        case 1:
            Alarm_MSB = readRegister(REG_ADCALARM_COMP1_M);
            Alarm_LSB = readRegister(REG_ADCALARM_COMP1_L);
            break;
        case 2:
            Alarm_MSB = readRegister(REG_ADCALARM_COMP2_M);
            Alarm_LSB = readRegister(REG_ADCALARM_COMP2_L);
            break;
        case 3:
            Alarm_MSB = readRegister(REG_ADCALARM_COMP3_M);
            Alarm_LSB = readRegister(REG_ADCALARM_COMP3_L);
            break;
        default:
            return 0;
    }

    if (AlarmVal) {
        // Concatenate 7:0 MSB & 7:4 LSB, For a total 12-bit value
        uint16_t threshold = (uint16_t)(Alarm_MSB << 4) | ((Alarm_LSB & ADCALARM_LSB) >> 4);
        return threshold;
    } else {
        // Return ADCALARM_POL reading
        return (Alarm_LSB & ADCALARM_POL) ? 1 : 0;
    }
}

bool bq25155::setADCAlarms(uint8_t ADCAlarmCh, uint16_t AlarmVal, bool Polarity) {
    if (AlarmVal > 4095) return false;  // Comparator value is 12-bit max

    uint8_t Alarm_MSB = (uint8_t)(AlarmVal >> 4);       // MSB = b11:4
    uint8_t bits_LSB  = (uint8_t)((AlarmVal & 0x0F) << 4); // LSB = b3:0 left-shifted to b7:4

    uint8_t Alarm_LSB = 0;

    switch (ADCAlarmCh) {
        case 1:
            writeRegister(REG_ADCALARM_COMP1_M, Alarm_MSB);
            Alarm_LSB = readRegister(REG_ADCALARM_COMP1_L);
            break;
        case 2:
            writeRegister(REG_ADCALARM_COMP2_M, Alarm_MSB);
            Alarm_LSB = readRegister(REG_ADCALARM_COMP2_L);
            break;
        case 3:
            writeRegister(REG_ADCALARM_COMP3_M, Alarm_MSB);
            Alarm_LSB = readRegister(REG_ADCALARM_COMP3_L);
            break;
        default:
            return false;
    }

    // Clear bits
    Alarm_LSB &= ~ADCALARM_LSB; // Clear ADCALARM_LSB (b7:4)
    Alarm_LSB &= ~ADCALARM_POL; // Clear ADCALARM_POL (b3)

    // Set new bit values
    Alarm_LSB |= bits_LSB; 
    if (Polarity) Alarm_LSB |= ADCALARM_POL;

    switch (ADCAlarmCh) {
        case 1:
            return writeRegister(REG_ADCALARM_COMP1_L, Alarm_LSB);
        case 2:
            return writeRegister(REG_ADCALARM_COMP2_L, Alarm_LSB);
        case 3:
            return writeRegister(REG_ADCALARM_COMP3_L, Alarm_LSB);
        default:
            return false;
    }
}
// --- End ADCALARM_COMPx Settings - ADC Comparators Values ---

// --- Begin ADC_READ_EN Settings - ADC Channels Enable/Disable ---
bool bq25155::isADCEnabled(uint8_t ADC_Ch) {
    if ((ADC_Ch & VALID_ADC_MASKS) == 0) return false; // not a valid mask
    return (readRegister(REG_ADC_READ_EN) & ADC_Ch) != 0;
}

bool bq25155::setADCChannel(uint8_t ADC_Ch, bool Ch_val) {
    if ((ADC_Ch & VALID_ADC_MASKS) == 0) return false; // not a valid mask

    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    if (!Ch_val)
        regValue &= ~ADC_Ch; // 1b0 = ADC measurement disabled
    else
        regValue |= ADC_Ch; // 1b1 = ADC measurement enabled

    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::DisableAllADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= DIS_ALL_ADC_CH; // Keep value of reserved b0
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::EnableAllADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_ALL_ADC_CH; // Keep value of reserved b0
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::isIINADCChEnabled() { return (readRegister(REG_ADC_READ_EN) & EN_IIN_READ_MASK) != 0; }
bool bq25155::DisableIINADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= ~EN_IIN_READ_MASK; // 1b0 = ADC measurement disabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
bool bq25155::EnableIINADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_IIN_READ_MASK; // 1b1 = ADC measurement enabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::isPMIDADCChEnabled() { return (readRegister(REG_ADC_READ_EN) & EN_PMID_READ_MASK) != 0; }
bool bq25155::DisablePMIDADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= ~EN_PMID_READ_MASK; // 1b0 = ADC measurement disabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
bool bq25155::EnablePMIDADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_PMID_READ_MASK; // 1b1 = ADC measurement enabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::isICHGADCChEnabled() { return (readRegister(REG_ADC_READ_EN) & EN_ICHG_READ_MASK) != 0; }
bool bq25155::DisableICHGADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= ~EN_ICHG_READ_MASK; // 1b0 = ADC measurement disabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
bool bq25155::EnableICHGADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_ICHG_READ_MASK; // 1b1 = ADC measurement enabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::isVINADCChEnabled() { return (readRegister(REG_ADC_READ_EN) & EN_VIN_READ_MASK) != 0; }
bool bq25155::DisableVINADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= ~EN_VIN_READ_MASK; // 1b0 = ADC measurement disabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
bool bq25155::EnableVINADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_VIN_READ_MASK; // 1b1 = ADC measurement enabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::isVBATADCChEnabled() { return (readRegister(REG_ADC_READ_EN) & EN_VBAT_READ_MASK) != 0; }
bool bq25155::DisableVBATADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= ~EN_VBAT_READ_MASK; // 1b0 = ADC measurement disabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
bool bq25155::EnableVBATADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_VBAT_READ_MASK; // 1b1 = ADC measurement enabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::isTSADCChEnabled() { return (readRegister(REG_ADC_READ_EN) & EN_TS_READ_MASK) != 0; }
bool bq25155::DisableTSADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= ~EN_TS_READ_MASK; // 1b0 = ADC measurement disabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
bool bq25155::EnableTSADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_TS_READ_MASK; // 1b1 = ADC measurement enabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}

bool bq25155::isADCINADCChEnabled() { return (readRegister(REG_ADC_READ_EN) & EN_ADCIN_READ_MASK) != 0; }
bool bq25155::DisableADCINADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue &= ~EN_ADCIN_READ_MASK; // 1b0 = ADC measurement disabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
bool bq25155::EnableADCINADCCh() {
    uint8_t regValue = readRegister(REG_ADC_READ_EN);
    regValue |= EN_ADCIN_READ_MASK; // 1b1 = ADC measurement enabled
    return writeRegister(REG_ADC_READ_EN, regValue);
}
// --- End ADC_READ_EN Settings - ADC Channels Enable/Disable ---

// --- Begin TS_FASTCHGCTRL Settings - Charger V & I control respect TS ---
uint16_t bq25155::getTSVCHG() {
    uint8_t TS_mV_read = (readRegister(REG_TS_FASTCHGCTRL) & TS_VBAT_REG_MASK) >> 4;
    uint16_t mV_BAT_set = getChargeVoltage(); // in mV

    switch (TS_mV_read) {
        case TS_VBAT_NO_RED: return mV_BAT_set;
        case TS_VBAT_50MV:   return (mV_BAT_set > 50)  ? (mV_BAT_set - 50)  : 0;
        case TS_VBAT_100MV:  return (mV_BAT_set > 100) ? (mV_BAT_set - 100) : 0;
        case TS_VBAT_150MV:  return (mV_BAT_set > 150) ? (mV_BAT_set - 150) : 0;
        case TS_VBAT_200MV:  return (mV_BAT_set > 200) ? (mV_BAT_set - 200) : 0;
        case TS_VBAT_250MV:  return (mV_BAT_set > 250) ? (mV_BAT_set - 250) : 0;
        case TS_VBAT_300MV:  return (mV_BAT_set > 300) ? (mV_BAT_set - 300) : 0;
        case TS_VBAT_350MV:  return (mV_BAT_set > 350) ? (mV_BAT_set - 350) : 0;
        default: return 0;
    }
}

bool bq25155::setTSVCHG(uint16_t mV_TS) { // in mV
    uint8_t bits = 0;
    
    if (mV_TS < 25)
        bits = TS_VBAT_NO_RED;
    else if (mV_TS < 75)
        bits = TS_VBAT_50MV;
    else if (mV_TS < 125)
        bits = TS_VBAT_100MV;
    else if (mV_TS < 175)
        bits = TS_VBAT_150MV;
    else if (mV_TS < 225)
        bits = TS_VBAT_200MV;
    else if (mV_TS < 275)
        bits = TS_VBAT_250MV;
    else if (mV_TS < 325)
        bits = TS_VBAT_300MV;
    else
        bits = TS_VBAT_350MV;

    bits <<= 4; // Move bits to b6:4
    
    uint8_t regValue = readRegister(REG_TS_FASTCHGCTRL);
    regValue &= ~TS_VBAT_REG_MASK; // Clear TS_VBAT (b6:4)
    regValue |= bits; // Set new TS_VBAT bits

    return writeRegister(REG_TS_FASTCHGCTRL, regValue);
}

uint32_t bq25155::getTSICHG() {
    uint8_t TS_mA_read = readRegister(REG_TS_FASTCHGCTRL) & TS_ICHRG_MASK;
    uint32_t uA_BAT_set = getChargeCurrent(); // in uA

    switch (TS_mA_read) {
        case TS_ICHRG_NO_RED: return uA_BAT_set;
        case TS_ICHRG_X_875:  return (uA_BAT_set * 875UL) / 1000;
        case TS_ICHRG_X_750:  return (uA_BAT_set * 750UL) / 1000;
        case TS_ICHRG_X_625:  return (uA_BAT_set * 625UL) / 1000;
        case TS_ICHRG_X_500:  return (uA_BAT_set * 500UL) / 1000;
        case TS_ICHRG_X_375:  return (uA_BAT_set * 375UL) / 1000;
        case TS_ICHRG_X_250:  return (uA_BAT_set * 250UL) / 1000;
        case TS_ICHRG_X_125:  return (uA_BAT_set * 125UL) / 1000;
        default: return 0;
    }
}

bool bq25155::setTSICHG(uint16_t multiple) { // multiples using integer
    uint8_t bits = 0;
    
    if (multiple < 63)
        bits = TS_ICHRG_NO_RED;
    else if (multiple < 188)
        bits = TS_ICHRG_X_125;
    else if (multiple < 313)
        bits = TS_ICHRG_X_250;
    else if (multiple < 438)
        bits = TS_ICHRG_X_375;
    else if (multiple < 563)
        bits = TS_ICHRG_X_500;
    else if (multiple < 688)
        bits = TS_ICHRG_X_625;
    else if (multiple < 813)
        bits = TS_ICHRG_X_750;
    else
        bits = TS_ICHRG_X_875;

    uint8_t regValue = readRegister(REG_TS_FASTCHGCTRL);
    regValue &= ~TS_ICHRG_MASK; // Clear TS_ICHG (b2:0)
    regValue |= bits; // Set new TS_ICHG bits

    return writeRegister(REG_TS_FASTCHGCTRL, regValue);
}
// --- End ADC_READ_EN Settings - Charger V & I control respect TS ---

// --- Begin TS_THRESHOLDS Settings - TS Comparators Thresholds ---
float bq25155::getTSVAL(uint8_t TS_REG){
    switch(TS_REG){
        case REG_TS_COLD: return readRegister(REG_TS_COLD) * TS_TH_MV;
        case REG_TS_COOL: return readRegister(REG_TS_COOL) * TS_TH_MV;
        case REG_TS_WARM: return readRegister(REG_TS_WARM) * TS_TH_MV;
        case REG_TS_HOT: return readRegister(REG_TS_HOT) * TS_TH_MV;
        default: return 0.0f;
    }
}

bool bq25155::setTSVAL(uint16_t TS_THRS_mV, uint8_t TS_REG){
    // Check for max val
    if (TS_THRS_mV>1200) TS_THRS_mV = 1200;
    
    // Convert to bits
    uint8_t TS_bits = TS_THRS_mV / TS_TH_MV;

    switch(TS_REG){
        case REG_TS_COLD: return writeRegister(REG_TS_COLD, TS_bits);
        case REG_TS_COOL: return writeRegister(REG_TS_COOL, TS_bits);
        case REG_TS_WARM: return writeRegister(REG_TS_WARM, TS_bits);
        case REG_TS_HOT: return writeRegister(REG_TS_HOT, TS_bits);
        default: return false;
    }
}
// --- End TS_THRESHOLDS Settings - TS Comparators Thresholds ---

// --- Begin DEVICE_ID ---
uint8_t bq25155::getDeviceID() {
    uint8_t deviceId = readRegister(REG_DEVICE_ID);
    if (deviceId == 0x35) // 0b00110101
        return deviceId;
    else
        return 0x00;
}

String bq25155::getDeviceIDString() {
    uint8_t deviceId = readRegister(REG_DEVICE_ID);
    if (deviceId == 0x35) // 0b00110101
        return "bq25155";
    else
        return "Unknown ID: 0x" + String(deviceId, HEX);
}
// --- End DEVICE_ID ---

/** // WIP
bqChargeStatus bq25155::getChargeStatus() {
    uint8_t chargeStatBits = readRegister(REG_STAT_0);

    if ((chargeStatBits & ) == STAT_CHG_NOT_CHARGING) return BQ_NOT_CHARGING;
    if ((chargeStatBits & ) == STAT_CHG_PRECHARGE) return BQ_PRECHARGE;
    if ((chargeStatBits & ) == STAT_CHG_FASTCHARGE) return BQ_FASTCHARGE;
    if ((chargeStatBits & CHARGE_DONE_STAT_MASK) == 1) return BQ_CHARGE_DONE;
    
    return BQ_UNKNOWN_STATUS;
}
**/