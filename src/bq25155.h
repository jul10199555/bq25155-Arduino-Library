/*
 * @brief         Header file for bq25155's Arduino library
 * @note          Implementation of I2C functions for controlling 
 *                bq25155 1S LiIon+/LiPo Charger.
 * @version       1.0.3
 * @creation date 2025-06-16
 * @updated date  2025-06-26
 * @author        jul10199555
 * 
 * Repo:
 * https://github.com/jul10199555/bq25155-Arduino-Library
 * 
 */

#ifndef BQ25155_H
#define BQ25155_H

#include <Arduino.h>
#include <Wire.h>

namespace bq25155_const {

// Define I2C address of the bq25155 (7-bit address)
static constexpr auto bq25155_ADDR = 0x6B;

// --- Register Addresses & Bitfield Masks (from Datasheet Table 9-8) ---

// Status Registers

// STAT0 Register
static constexpr auto REG_STAT_0 = (0x00); //[R]: Charger Status 0
// Bitfield masks
static constexpr auto CHRG_CV_STAT_MASK = (1 << 6); //b6: Constant Voltage Charging Mode (Taper Mode) Status
static constexpr auto CHARGE_DONE_STAT_MASK = (1 << 5); //b5: Charge Done Status
static constexpr auto IINLIM_ACTIVE_STAT_MASK = (1 << 4); //b4: Input Current Limit Status
static constexpr auto VDPPM_ACTIVE_STAT_MASK = (1 << 3); //b3: DPPM Status
static constexpr auto VINDPM_ACTIVE_STAT_MASK = (1 << 2); //b2: VINDPM Status
static constexpr auto THERMREG_ACTIVE_STAT_MASK = (1 << 1); //b1: Thermal Regulation Status
static constexpr auto VIN_PGOOD_STAT_MASK = (0x01); //b0: VIN Power Good Status
// VIN_PGOOD_STAT READ
static constexpr auto VIN_NOTGOOD = (0x00); //VIN_PGOOD_STAT = Not Good
static constexpr auto VIN_PGOOD = (0x01); //VIN_PGOOD_STAT = VIN > VUVLO and VIN > VBAT + VSLP and VIN < VOVP

// STAT1 Register
static constexpr auto REG_STAT_1 = (0x01); //[R]: Charger Status 1
// Bitfield masks
static constexpr auto VIN_OVP_FAULT_STAT_MASK = (1 << 7); //b7: VIN Overvoltage Status
static constexpr auto BAT_OCP_FAULT_STAT_MASK = (1 << 5); //b5: Battery Over-Current Protection Status
static constexpr auto BAT_UVLO_FAULT_STAT_MASK = (1 << 4); //b4: Battery voltage below BATUVLO Level Status
static constexpr auto TS_COLD_STAT_MASK = (1 << 3); //b3: TS Cold Status - VTS > VCOLD (charging suspended)
static constexpr auto TS_COOL_STAT_MASK = (1 << 2); //b2: TS Cool Status - VCOOL < VTS < VCOLD (charging current reduced by value set by TS_Registers)
static constexpr auto TS_WARM_STAT_MASK = (1 << 1); //b1: TS Warm - VWARM > VTS > VHOT (charging voltage reduced by value set by TS_Registers)
static constexpr auto TS_HOT_STAT_MASK = (0x01); //b0: TS Hot Status - VTS < VHOT (charging suspended)
// BAT_UVLO_FAULT_STAT READ
static constexpr auto BATUV_OK = (0x00); //BAT_UVLO_FAULT_STAT = VBAT > VBATUVLO
static constexpr auto BATUV_LO = (0x01); //BAT_UVLO_FAULT_STAT = VBAT < VBATUVLO
// STAT READ
static constexpr auto NOT_ACTIVE_STAT = (0x00); //REG = Not Active
static constexpr auto ACTIVE_STAT = (0x01); //REG = Active

// STAT2 Register
static constexpr auto REG_STAT_2 = (0x02); //[R]: ADC Status
// STAT2 bitfield masks
static constexpr auto COMP1_ALARM_STAT_MASK = (1 << 6); //b6: ADC COMPARATOR 1 Status
static constexpr auto COMP2_ALARM_STAT_MASK = (1 << 5); //b5: ADC COMPARATOR 2 Status
static constexpr auto COMP3_ALARM_STAT_MASK = (1 << 4); //b4: ADC COMPARATOR 3 Status
static constexpr auto TS_OPEN_STAT_MASK = (0x01); //b0: TS Open Status
// COMPx_ALARM_STAT SET
static constexpr auto COMPx_ALARM_BELOW = (0x00); //COMPx_ALARM_STAT = ADC does not meet condition set by ADCALARMx_ABOVE bit
static constexpr auto COMPx_ALARM_ABOVE = (0x01); //COMPx_ALARM_STAT = ADC meets condition set by ADCALARMx_ABOVE bit
// TS_OPEN_STAT SET
static constexpr auto VTS_BELOW_VOPEN = (0x00); //TS_OPEN_STAT = VTS < VOPEN
static constexpr auto VTS_ABOVE_VOPEN = (0x01); //TS_OPEN_STAT = VTS > VOPEN

// Flag Registers

// FLAG0 Register
static constexpr auto REG_FLAG_0 = (0x03); //[RC]: Charger Flags 0
// Bitfield masks
static constexpr auto CHRG_CV_FLAG_MASK = (1 << 6); //b6: Constant Voltage Charging Mode (Taper Mode) Flag
static constexpr auto CHARGE_DONE_FLAG_MASK = (1 << 5); //b5: Charge Done Flag
static constexpr auto IINLIM_ACTIVE_FLAG_MASK = (1 << 4); //b4: Input Current Limit Flag
static constexpr auto VDPPM_ACTIVE_FLAG_MASK = (1 << 3); //b3: DPPM Flag
static constexpr auto VINDPM_ACTIVE_FLAG_MASK = (1 << 2); //b2: VINDPM Flag
static constexpr auto THERMREG_ACTIVE_FLAG_MASK = (1 << 1); //b1: Thermal Regulation Flag
static constexpr auto VIN_PGOOD_FLAG_MASK = (0x01); //b0: VIN Power Good Flag

// FLAG1 Register
static constexpr auto REG_FLAG_1 = (0x04); //[RC]: Charger Flags 1
// Bitfield masks
static constexpr auto VIN_OVP_FAULT_FLAG_MASK = (1 << 7); //b7: VIN Over Voltage Fault Flag
static constexpr auto BAT_OCP_FAULT_FLAG_MASK = (1 << 5); //b5: Battery Over-Current Protection Flag
static constexpr auto BAT_UVLO_FAULT_FLAG_MASK = (1 << 4); //b4: Battery Under Voltage Flag
static constexpr auto TS_COLD_FLAG_MASK = (1 << 3); //b3: TS Cold Region Entry Flag
static constexpr auto TS_COOL_FLAG_MASK = (1 << 2); //b2: TS Cold Region Entry Flag
static constexpr auto TS_WARM_FLAG_MASK = (1 << 1); //b1: TS Warm Region Entry Flag
static constexpr auto TS_HOT_FLAG_MASK = (0x01); //b0: TS Hot Region Entry Flag

// FLAG2 Register
static constexpr auto REG_FLAG_2 = (0x05); //[RC]: ADC Flags
// Bitfield masks
static constexpr auto ADC_READY_FLAG_MASK = (1 << 7); //b7: ADC Ready Flag
static constexpr auto COMP1_ALARM_FLAG_MASK = (1 << 6); //b6: ADC COMPARATOR 1 Flag
static constexpr auto COMP2_ALARM_FLAG_MASK = (1 << 5); //b5: ADC COMPARATOR 2 Flag
static constexpr auto COMP3_ALARM_FLAG_MASK = (1 << 4); //b4: ADC COMPARATOR 3 Flag
static constexpr auto TS_OPEN_FLAG_MASK = (0x01); //b0: TS Open Flag

// FLAG3 Register
static constexpr auto REG_FLAG_3 = (0x06); //[RC]: Timer Flags
// Bitfield masks
static constexpr auto WD_FAULT_FLAG_MASK = (1 << 6); //b6: Watchdog Fault Flag
static constexpr auto SAFETY_TMR_FAULT_FLAG_MASK = (1 << 5); //b5: Safety Timer Fault Flag
static constexpr auto LDO_OCP_FAULT_FLAG_MASK = (1 << 4); //b4: LDO Over Current Fault
static constexpr auto MRWAKE1_TIMEOUT_FLAG_MASK = (1 << 2); //b2: MR Wake 1 Timer Flag
static constexpr auto MRWAKE2_TIMEOUT_FLAG_MASK = (1 << 1); //b1: MR Wake 2 Timer Flag
static constexpr auto MRRESET_WARN_FLAG_MASK = (0x01); //b0: MR Reset Warn Timer Flag
// FLAG STAT
static constexpr auto FLAG_NOT_DETECTED = (0x00); //FLAG = Not Detected
static constexpr auto FLAG_DETECTED = (0x01); //FLAG = Detected

// Mask Registers

// MASK0 Register
static constexpr auto REG_MASK_0 = (0x07); //[R/W]: Interrupt Masks 0
// Bitfield masks
static constexpr auto CHRG_CV_MASK = (1 << 6); //b6: Mask for CHRG_CV interrupt
static constexpr auto CHARGE_DONE_MASK = (1 << 5); //b5: Mask for CHARGE_DONE interrupt
static constexpr auto IINLIM_ACTIVE_MASK = (1 << 4); //b4: Mask for IINLIM_ACTIVE interrupt
static constexpr auto VDPPM_ACTIVE_MASK = (1 << 3); //b3: Mask for VDPPM_ACTIVE interrupt
static constexpr auto VINDPM_ACTIVE_MASK = (1 << 2); //b2: Mask for VINDPM_ACTIVE interrupt
static constexpr auto THERMREG_ACTIVE_MASK = (1 << 1); //b1: Mask for THERMREG_ACTIVE interrupt
static constexpr auto VIN_PGOOD_MASK = (0x01); //b0: Mask for VIN_PGOOD interrupt

// MASK1 Register
static constexpr auto REG_MASK_1 = (0x08); //[R/W]: Interrupt Masks 1
// Bitfield masks
static constexpr auto VIN_OVP_FAULT_MASK = (1 << 7); //b7: Mask for VIN_OVP_FAULT interrupt
static constexpr auto BAT_OCP_FAULT_MASK = (1 << 5); //b5: Mask for BAT_OCP_FAULT interrupt
static constexpr auto BAT_UVLO_FAULT_MASK = (1 << 4); //b4: Mask for BAT_UVLO_FAULT interrupt
static constexpr auto TS_COLD_MASK = (1 << 3); //b3: Mask for TS_COLD interrupt
static constexpr auto TS_COOL_MASK = (1 << 2); //b2: Mask for TS_COOL interrupt
static constexpr auto TS_WARM_MASK = (1 << 1); //b1: Mask for TS_WARM interrupt
static constexpr auto TS_HOT_MASK = (0x01); //b0: Mask for TS_HOT interrupt

// MASK2 Register
static constexpr auto REG_MASK_2 = (0x09); //[R/W]: Interrupt Masks 2
// Bitfield masks
static constexpr auto MASK2_DEF = (0x71); //COMP1_ALARM = COMP2_ALARM_MASK = COMP3_ALARM_MASK = TS_OPEN_MASK = 1
static constexpr auto ADC_READY_MASK = (1 << 7); //b7: Mask for ADC_READY Interrupt
static constexpr auto COMP1_ALARM_MASK = (1 << 6); //b6: Mask for COMP1_ALARM Interrupt
static constexpr auto COMP2_ALARM_MASK = (1 << 5); //b5: Mask for COMP2_ALARM Interrupt
static constexpr auto COMP3_ALARM_MASK = (1 << 4); //b4: Mask for COMP3_ALARM Interrupt
static constexpr auto TS_OPEN_MASK = (0x01); //b0: Mask for TS_OPEN Interrupt

// MASK3 Register
static constexpr auto REG_MASK_3 = (0x0A); //[R/W]: Interrupt Masks 3
// Bitfield Masks
static constexpr auto WD_FAULT_MASK = (1 << 6); //b6: Mask for WD_FAULT Interrupt
static constexpr auto SAFETY_TMR_FAULT_MASK = (1 << 5); //b5: Mask for SAFETY_TIMER_FAULT Interrup
static constexpr auto LDO_OCP_FAULT_MASK = (1 << 4); //b4: Mask for LDO_OCP_FAULT Interrupt
static constexpr auto MRWAKE1_TIMEOUT_MASK = (1 << 2); //b2: Mask for MRWAKE1_TIMEOUT Interrupt
static constexpr auto MRWAKE2_TIMEOUT_MASK = (1 << 1); //b1: Mask for MRWAKE2_TIMEOUT Interrupt
static constexpr auto MRRESET_WARN_MASK = (0x01); //b0: Mask for MRRESET_WARN Interrupt
// INT_MASK STAT
static constexpr auto INT_NOT_MASKED = (0x00); //INT_MASK = Interrupt Not Masked
static constexpr auto INT_MASKED = (0x01); //INT_MASK = Interrupt Masked

// V & I Control
// VBAT_CTRL Register (Battery Regulation Voltage)
static constexpr auto REG_VBAT_CTRL = (0x12); //[R/W]: Battery Voltage Control (VREG)
// Bitfield Masks
static constexpr auto VBAT_REG_DEF = (0x3C); //3.6V + 60 * 10 mV = 4.2V
static constexpr auto VBAT_REG_MASK = (0x7F); //b6-0: VBATREG 7-bit mask

// ICHG_CTRL Register (Fast Charge Current)
static constexpr auto REG_ICHG_CTRL = (0x13); //[R/W]: Fast Charge Current Control (ISET)
// Bitfield Masks
static constexpr auto ICHG_CTRL_DEF = (0x08); //1.25 mA * 8 = 10mA (ICHARGE_RANGE = 0), 2.5 mA * 8 = 20mA (ICHARGE_RANGE = 1, 0.5 A max)
static constexpr auto ICHG_CTRL_MASK = (0xFF); //b7-0: ICHARGE_RANGE 8-bit mask

// PCHRGCTRL Register (Pre-Charge Current)
static constexpr auto REG_PCHRGCTRL = (0x14); //[R/W]: Pre-Charge Current Control (IPRECHG and VPRECHG)
// Bitfield Masks
static constexpr auto IPRECHG_DEF = (0x02); //1.25 mA * 2 = 2.5 mA default
static constexpr auto ICHARGE_RANGE_MASK = (1 << 7); //b7: Charge Current Step
static constexpr auto IPRECHG_MASK = (0x1F); //b4-0: Pre-Charge Current 5-bit mask
// ICHARGE SET
static constexpr auto ICHARGE_2_50MA = (0x01); //2.5 mA step (500 mA max charge current)
static constexpr auto ICHARGE_1_25MA = (0x00); //1.25 mA step (318.75 mA max charge current)

// TERMCTRL Register (Termination Current)
static constexpr auto REG_TERMCTRL = (0x15); //[R/W]: Termination Current Control (ITERM)
// Bitfield Masks
static constexpr auto ITERM_DEF = (0x14); //10% of ICHRG default
static constexpr auto ITERM_MASK = (0x3E); //b5-1: Termination Current (1% to 31% of ICHRG) mask
static constexpr auto TERM_DISABLE_MASK = (0x01); //b0: Termination Disable mask

// BUVLO Register (Over Current & Under Voltage Thresholds)
static constexpr auto REG_BUVLO = (0x16); //[R/W]: Battery UVLO and Current Limit Control
// Bitfield Masks
static constexpr auto VLOWV_SEL_MASK = (1 << 5); //b5: Pre-charge to Fast Charge Threshold mask
static constexpr auto IBAT_OCP_ILIM_MASK = (0x18); //b4-3: Battery Over-Current Protection Threshold mask
static constexpr auto BUVLO_MASK = (0x07); //b2-0: Battery UVLO Voltage
// VLOWV SEL
static constexpr auto VLOWV_3_0V = (0x00); //Pre-charge to Fast Charge = 3.0V
static constexpr auto VLOWV_2_8V = (0x01); //Pre-charge to Fast Charge = 2.8V
// IBAT_OCP SEL
static constexpr auto IBAT_OCP_1200MA = (0x00); //Battery Over-Current Protection = 1200 mA
static constexpr auto IBAT_OCP_1500MA = (0x01); //Battery Over-Current Protection = 1500 mA
static constexpr auto IBAT_OCP_DIS = (0x02); //Battery Over-Current Protection = Disabled
// BUVLO SEL
static constexpr auto BUVLO_3_0V = (0x00); //Battery UVLO Voltage = 3.0 V
static constexpr auto BUVLO_2_8V = (0x03); //Battery UVLO Voltage = 2.8 V
static constexpr auto BUVLO_2_6V = (0x04); //Battery UVLO Voltage = 2.6 V
static constexpr auto BUVLO_2_4V = (0x05); //Battery UVLO Voltage = 2.4 V
static constexpr auto BUVLO_2_2V = (0x06); //Battery UVLO Voltage = 2.2 V
static constexpr auto BUVLO_DIS = (0x07); //Battery UVLO Voltage = Disabled

// CHARGERCTRL0 Register
static constexpr auto REG_CHARGERCTRL0 = (0x17); //[R/W]: Charger Control 0 (CHG_EN, Timers)
// Bitfield Masks
static constexpr auto CHARGERCTRL0_DEF = (0x82); //TS_EN = EN & SAFETY_TIMER_LIMIT = 6 Hr

static constexpr auto TS_EN_MASK = (1 << 7); //b7: Mask for TS Function Enable
static constexpr auto TS_CONTROL_MODE_MASK = (1 << 6); //b6: Mask for TS Function Control Mode
static constexpr auto VRH_THRESH_MASK = (1 << 5); //b5: Mask for IINLIM_ACTIVE interrupt
static constexpr auto WATCHDOG_DISABLE_MASK = (1 << 4); //b4: Mask for VDPPM_ACTIVE interrupt
static constexpr auto SFT_2XTMR_EN_MASK = (1 << 3); //b3: Mask for VINDPM_ACTIVE interrupt
static constexpr auto SAFETY_TIMER_LIMIT_MASK = (0x06); //b2-1: Mask for Charger Safety Timer
// SAFETY_TIMER_LIMIT SEL
static constexpr auto SAFETY_TIMER_LIMIT_3H = (0x00); //Charger Safety Timer = 3h
static constexpr auto SAFETY_TIMER_LIMIT_6H = (0x01); //Charger Safety Timer = 6h
static constexpr auto SAFETY_TIMER_LIMIT_12H = (0x02); //Charger Safety Timer = 12h
static constexpr auto SAFETY_TIMER_LIMIT_DIS = (0x03); //Charger Safety Timer = Disabled

// CHARGERCTRL1 Register
static constexpr auto REG_CHARGERCTRL1 = (0x18); //[R/W]: Charger Control 1 (VINDPM, etc.)
// Bitfield Masks
static constexpr auto CHARGERCTRL1_DEF = (0xC2); //VINDPM = EN & VINDPM = 4.6 V & THERM_REG = 90degC

static constexpr auto VINDPM_DIS_MASK = (1 << 7); //b7: Mask for Disable VINDPM Function
static constexpr auto VINPDM_MASK = (0x70); //b6-4: Mask for VINDPM Level Selection
static constexpr auto DPPM_DIS_MASK = (1 << 3); //b3: Mask for DPPM Disable
static constexpr auto THERM_REG_MASK = (0x07); //b2-0: Mask for Thermal Charge Current Foldback Threshold
// VINDPM_DIS SET
static constexpr auto VINDPM_EN = (0x00); //VINDPM Enabled
static constexpr auto VINDPM_DIS = (0x01); //VINDPM Disabled
// VINDPM SEL
static constexpr auto VINDPM_4_2V = (0x00); //VINDPM = 4.2V
static constexpr auto VINDPM_4_3V = (0x01); //VINDPM = 4.3V
static constexpr auto VINDPM_4_4V = (0x02); //VINDPM = 4.4V
static constexpr auto VINDPM_4_5V = (0x03); //VINDPM = 4.5V
static constexpr auto VINDPM_4_6V = (0x04); //VINDPM = 4.6V
static constexpr auto VINDPM_4_7V = (0x05); //VINDPM = 4.7V
static constexpr auto VINDPM_4_8V = (0x06); //VINDPM = 4.8V
static constexpr auto VINDPM_4_9V = (0x07); //VINDPM = 4.9V
// THERM_REG SEL
static constexpr auto THERM_THRS_80C = (0x00); //THERM_REG = 80degC
static constexpr auto THERM_THRS_85C = (0x01); //THERM_REG = 85degC
static constexpr auto THERM_THRS_90C = (0x02); //THERM_REG = 90degC
static constexpr auto THERM_THRS_95C = (0x03); //THERM_REG = 95degC
static constexpr auto THERM_THRS_100C = (0x04); //THERM_REG = 100degC
static constexpr auto THERM_THRS_105C = (0x05); //THERM_REG = 105degC
static constexpr auto THERM_THRS_110C = (0x06); //THERM_REG = 110degC
static constexpr auto THERM_THRS_DIS = (0x07); //THERM_REG = Disabled

// ILIMCTRL Register (Input Current Limit)
static constexpr auto REG_ILIMCTRL = (0x19); //[R/W]: Input Current Limit Control (IIN_LIMIT)
// Bitfield Masks
static constexpr auto ILIM_MASK = (0x07); //b2-0: Mask for Input Current Limit Level Selection
// ILIM_50MA to ILIM_600MA constants
static constexpr auto ILIM_50MA = (0x00);
static constexpr auto ILIM_100MA = (0x01);
static constexpr auto ILIM_150MA = (0x02);
static constexpr auto ILIM_200MA = (0x03);
static constexpr auto ILIM_300MA = (0x04);
static constexpr auto ILIM_400MA = (0x05);
static constexpr auto ILIM_500MA = (0x06);
static constexpr auto ILIM_600MA = (0x07);

// LDOCTRL Register
static constexpr auto REG_LDOCTRL = (0x1D); //[R/W]: LDO Control
// Bitfield Masks
static constexpr auto LDOCTRL_DEF = (0xB0); //LS/LDO = EN, VLDO = 1.8 V, LDO_SWITCH_CONFG = LDO

static constexpr auto EN_LS_LDO_MASK = (1 << 7); //b7: Mask for LS/LDO Enable
static constexpr auto VLDO_MASK = (0x7C); //b6-2: Mask for LDO output voltage set
static constexpr auto LDO_SWITCH_CONFG_MASK = (1 << 1); //b1: LDO / Load Switch Configuration Select
// LS_LDO SET
static constexpr auto LS_LDO_DIS = (0x00); //Disable LS/LDO
static constexpr auto LS_LDO_EN = (0x01); //Enable LS/LDO
// LDO_SWITCH_CONFG SET
static constexpr auto LS_LDO_SWITCH_LDO = (0x00); //LS/LDO = LDO
static constexpr auto LS_LDO_SWITCH_LS = (0x01); //LS/LDO = Load Switch

// IC Control

// MRCTRL Register
static constexpr auto REG_MRCTRL = (0x30); //[R/W]: MR Control (Master Reset)
// Bitfield Masks
static constexpr auto MRCTRL_DEF = (0x2A); //Wake2 Timer = 2s, Reset Warn = MR_HW_RESET - 1.0 s, HW Reset Timer = 8 s

static constexpr auto MR_RESET_VIN_MASK = (1 << 7); //b7: Mask for VIN Power Good gated MR Reset Enable
static constexpr auto MR_WAKE1_TIMER_MASK = (1 << 6); //b6: Mask for Wake 1 Timer setting
static constexpr auto MR_WAKE2_TIMER_MASK = (1 << 5); //b5: Mask for Wake 2 Timer setting
static constexpr auto MR_RESET_WARN_MASK = (0x18); //b4-3: MR Reset Warn Timer setting
static constexpr auto MR_HW_RESET_MASK = (0x06); //b2-1: MR HW Reset Timer setting
// MR_RESET_VIN SET
static constexpr auto MR_SENT_NOT_CARE_VIN = (0x00); //Reset sent when /MR reset time is met regardless of VIN state
static constexpr auto MR_SENT_CARE_VIN = (0x01); //Reset sent when MR reset is met and Vin is valid
// MR_WAKE1_TIMER SET
static constexpr auto MR_WAKE1_125MS = (0x00); //Reset WAKE1 Timer = 125 ms
static constexpr auto MR_WAKE1_500MS = (0x01); //Reset WAKE1 Timer = 500 ms
// MR_WAKE2_TIMER SET
static constexpr auto MR_WAKE2_1S = (0x00); //Reset WAKE2 Timer = 1 s
static constexpr auto MR_WAKE2_2S = (0x01); //Reset WAKE2 Timer = 2 s
// MR_RESET_WARN SEL
static constexpr auto MR_WARN_HW_0_5S = (0x00); //MR Reset Warn Timer = MR_HW_RESET - 0.5 s
static constexpr auto MR_WARN_HW_1_0S = (0x01); //MR Reset Warn Timer = MR_HW_RESET - 1.0 s
static constexpr auto MR_WARN_HW_1_5S = (0x02); //MR Reset Warn Timer = MR_HW_RESET - 1.5 s
static constexpr auto MR_WARN_HW_2_0S = (0x03); //MR Reset Warn Timer = MR_HW_RESET - 2.0 s
// MR_HW_RESET SEL
static constexpr auto MR_HW_RESET_4S = (0x00); //MR HW Reset Timer = 4 s
static constexpr auto MR_HW_RESET_8S = (0x01); //MR HW Reset Timer = 8 s
static constexpr auto MR_HW_RESET_10S = (0x02); //MR HW Reset Timer = 10 s
static constexpr auto MR_HW_RESET_14S = (0x03); //MR HW Reset Timer = 14 s

// ICCTRL0 Register
static constexpr auto REG_ICCTRL0 = (0x35); //[R/W]: IC Control 0
// Bitfield Masks
static constexpr auto ICCTRL0_DEF = (0x10); //AutoWake Timer = 1.2 s

static constexpr auto EN_SHIP_MODE_MASK = (1 << 7); //b7: Ship Mode Enable
static constexpr auto AUTOWAKE_MASK = (0x30); //b5-4: Auto-wakeup Timer (TRESTART) for /MR HW Reset
static constexpr auto GLOBAL_INT_MASK = (1 << 2); //b2: Global Interrupt Mask
static constexpr auto HW_RESET_MASK = (1 << 1); //b1: Hardware Reset
static constexpr auto SW_RESET_MASK = (0x01); //b0: Software Reset
// EN_SHIP_MODE SET
static constexpr auto SHIP_MODE_DIS = (0x00); //Normal operation
static constexpr auto SHIP_MODE_EN = (0x01); //Enter Ship Mode when VIN is not valid and /MR is high
// AUTOWAKE SEL (Auto-wakeup Timer (TRESTART) for /MR HW Reset)
static constexpr auto AUTOWAKE_0_6S = (0x00); //TRESTART = 0.6 s
static constexpr auto AUTOWAKE_1_2S = (0x01); //TRESTART = 1.2 s
static constexpr auto AUTOWAKE_2_4S = (0x02); //TRESTART = 2.4 s
static constexpr auto AUTOWAKE_5_0S = (0x03); //TRESTART = 5.0 s
// GLOBAL_INT_MASK SET
static constexpr auto GLOBAL_INT_NORM = (0x00); //Global Interrupt = Normal Operation
static constexpr auto GLOBAL_INT_MINT = (0x01); //Global Interrupt = Mask all interrupts
// HW_RESET SET
static constexpr auto HW_RESET_DIS = (0x00); //HW_RESET = Normal Operation
static constexpr auto HW_RESET_TRIG = (0x01); //HW_RESET = HW Reset. Temporarily power down all power rails, except VDD.
                                              //                      I2C Register go to default settings
// SW_RESET SET
static constexpr auto SW_RESET_DIS = (0x00); //SW_RESET = Normal Operation
static constexpr auto SW_RESET_TRIG = (0x01); //SW_RESET = SW Reset. I2C Registers go to default settings

// ICCTRL1 Register
static constexpr auto REG_ICCTRL1 = (0x36); //[R/W]: IC Control 1
// Bitfield Masks
static constexpr auto MR_LPRESS_ACTION_MASK = (0xC0); //b7-6: MR Long Press Action
static constexpr auto ADCIN_MODE_MASK = (1 << 5); //b5: ADCIN Pin Mode of Operation
static constexpr auto PG_MODE_MASK = (0x0C); //b3-2: PG Pin Mode of Operation
static constexpr auto PMID_MODE_MASK = (0x03); //b1-0: PMID Control
// MR_LPRESS_ACTION SEL
static constexpr auto MR_LPRESS_HW_RST = (0x00); //MR Long Press = HW Reset (Power Cycle)
static constexpr auto MR_LPRESS_DISABLE = (0x01); //MR Long Press = Do nothing
static constexpr auto MR_LPRESS_SHIPMODE = (0x02); //MR Long Press = Enter Ship Mode
// ADCIN_MODE SET
static constexpr auto ADCIN_MODE_GPADC = (0x00); //ADCIN Pin Mode = General Purpose ADC input (no Internal biasing)
static constexpr auto ADCIN_MODE_NTCADC = (0x01); //ADCIN Pin Mode = 10K NTC ADC input (80 uA biasing)
// PG_MODE SEL
static constexpr auto PG_MODE_VIN = (0x00); //PG_MODE = VIN Power Good
static constexpr auto PG_MODE_DEG_MR = (0x01); //PG_MODE = Deglitched Level Shifted /MR
static constexpr auto PG_MODE_GP_ODO = (0x02); //PG_MODE = General Purpose Open Drain Output
// PMID_MODE SEL
static constexpr auto PMID_BAT_OR_VIN = (0x00); //PMID_MODE = PMID powered from BAT or VIN if present
static constexpr auto PMID_BAT_ONLY = (0x01); //PMID_MODE = PMID powered from BAT only, even if VIN is present
static constexpr auto PMID_DIS_FLT = (0x02); //PMID_MODE = PMID disconnected and left floating
static constexpr auto PMID_DIS_PD = (0x03); //PMID_MODE = PMID disconnected and pulled down

// ICCTRL2 Register
static constexpr auto REG_ICCTRL2 = (0x37); //[R/W]: IC Control 2
// Bitfield Masks
static constexpr auto ICCTRL2_DEF = (0x40); //PMID = 4.5 V

static constexpr auto PMID_REG_CTRL_MASK = (0xE0); //b7-5: System (PMID) Regulation Voltage
static constexpr auto GPO_PG_MASK = (1 << 4); //b4: /PG General Purpose Output State Control
static constexpr auto HWRESET_14S_WD_MASK = (1 << 1); //b1: Enable for 14-second I2C watchdog timer for HW Reset after VIN connection
static constexpr auto CHARGER_DISABLE_MASK = (0x01); //b0: Charge Disable
// PMID_REG_CTRL SEL
static constexpr auto PMID_BAT_TRACK = (0x00); //PMID_REG_CTRL = Battery Tracking
static constexpr auto PMID_4_4V = (0x01); //PMID_REG_CTRL = 4.4 V
static constexpr auto PMID_4_5V = (0x02); //PMID_REG_CTRL = 4.5 V
static constexpr auto PMID_4_6V = (0x03); //PMID_REG_CTRL = 4.6 V
static constexpr auto PMID_4_7V = (0x04); //PMID_REG_CTRL = 4.7 V
static constexpr auto PMID_4_8V = (0x05); //PMID_REG_CTRL = 4.8 V
static constexpr auto PMID_4_9V = (0x06); //PMID_REG_CTRL = 4.9 V
static constexpr auto PMID_VIN_PTHR = (0x07); //PMID_REG_CTRL = Pass-Through (VIN)
// GPO_PG SET
static constexpr auto GPO_PG_PD = (0x00); ///PG = Pulled Down
static constexpr auto GPO_PG_HZ = (0x01); ///PG = High Z
// HWRESET_14S_WD SET
static constexpr auto HWRESET_14S_WD_DIS = (0x00); //HWRESET_14S_WD = Timer disabled
static constexpr auto HWRESET_14S_WD_EN = (0x01); //HWRESET_14S_WD = HW reset if no I2C is done within 14 s after VIN is present
// CHARGER_DISABLE SET
static constexpr auto CHARGER_ENABLE = (0x00); //CHARGER_DISABLE = Charge enabled if /CE pin is low
static constexpr auto CHARGER_DISABLE = (0x01); //CHARGER_DISABLE = Charge disabled

// ADC Configuration

// ADCCTRL0 Register
static constexpr auto REG_ADCCTRL0 = (0x40); //[R/W]: ADC Control 0 (ADC_EN, ADC_RATE, ADC_AVG, ADC_CONT)
// Bitfield Masks
static constexpr auto ADCCTRL0_DEF = (0x02); //ADC Channel Comparator 1 = TS

static constexpr auto ADC_READ_RATE_MASK = (0xC0); //b7-6: Read rate for ADC measurements in BAT Only operation
static constexpr auto ADC_CONV_START_MASK = (1 << 5); //b5: ADC Conversion Start Trigger
static constexpr auto ADC_CONV_SPEED_MASK = (0x18); //b4-3: ADC Conversion Speed
static constexpr auto ADC_COMP1_MASK = (0x07); //b2-0: ADC Channel for Comparator 1
// ADC_READ_RATE SEL
static constexpr auto ADC_READ_RATE_MANUAL = (0x00); //ADC_READ_RATE = Manual Read (Measurement done when ADC_CONV_START is set)
static constexpr auto ADC_READ_RATE_CNTNS = (0x01); //ADC_READ_RATE = Continuous
static constexpr auto ADC_READ_RATE_1S = (0x02); //ADC_READ_RATE = 1 seg
static constexpr auto ADC_READ_RATE_1M = (0x03); //ADC_READ_RATE = 1 min
// ADC_CONV_START SET (Bit goes back to 0 when conversion is complete)
static constexpr auto ADC_NO_CONV = (0x00); ///PG = No ADC conversion
static constexpr auto ADC_INIT_CONV = (0x01); ///PG = Initiates ADC measurement in Manual Read operation
// ADC_CONV_SPEED SEL
static constexpr auto ADC_CONV_SPEED_24MS = (0x00); //ADC_CONV_SPEED = 24 ms (highest accuracy)
static constexpr auto ADC_CONV_SPEED_12MS = (0x01); //ADC_CONV_SPEED = 12 ms
static constexpr auto ADC_CONV_SPEED_6MS = (0x02); //ADC_CONV_SPEED = 6 ms
static constexpr auto ADC_CONV_SPEED_3MS = (0x03); //ADC_CONV_SPEED = 3 ms

// ADCCTRL1 Register
static constexpr auto REG_ADCCTRL1 = (0x41); //[R/W]: ADC Control 1 (ADC_START_MEAS bits)
// Bitfield Masks
static constexpr auto ADCCTRL1_DEF = (0x40); //ADC Channel Comparator 2 = TS

static constexpr auto ADC_COMP2_MASK = (0xE0); //b7-5: ADC Channel for Comparator 1
static constexpr auto ADC_COMP3_MASK = (0x1C); //b4-2: ADC Channel for Comparator 1
// ADC_COMPx SEL
static constexpr auto ADC_COMPx_DIS = (0x00); //ADC_COMPx = Disabled
static constexpr auto ADC_COMPx_ADCIN = (0x01); //ADC_COMPx = ADCIN
static constexpr auto ADC_COMPx_TS = (0x02); //ADC_COMPx = TS
static constexpr auto ADC_COMPx_VBAT = (0x03); //ADC_COMPx = VBAT
static constexpr auto ADC_COMPx_ICHG = (0x04); //ADC_COMPx = ICHARGE
static constexpr auto ADC_COMPx_VIN = (0x05); //ADC_COMPx = VIN
static constexpr auto ADC_COMPx_PMID = (0x06); //ADC_COMPx = PMID
static constexpr auto ADC_COMPx_IIN = (0x07); //ADC_COMPx = IIN

// ADC Readings Registers
static constexpr auto REG_ADC_DATA_VBAT_M = (0x42); //[R]: ADC VBAT Measurement MSB
static constexpr auto REG_ADC_DATA_VBAT_L = (0x43); //[R]: ADC VBAT Measurement LSB
static constexpr auto REG_ADC_DATA_TS_M = (0x44); //[R]: ADC TS Measurement MSB
static constexpr auto REG_ADC_DATA_TS_L = (0x45); //[R]: ADC TS Measurement LSB
static constexpr auto REG_ADC_DATA_ICHG_M = (0x46); //[R]: ADC ICHG Measurement MSB
static constexpr auto REG_ADC_DATA_ICHG_L = (0x47); //[R]: ADC ICHG Measurement LSB
static constexpr auto REG_ADC_DATA_ADCIN_M = (0x48); //[R]: ADC ADCIN Measurement MSB
static constexpr auto REG_ADC_DATA_ADCIN_L = (0x49); //[R]: ADC ADCIN Measurement LSB
static constexpr auto REG_ADC_DATA_VIN_M = (0x4A); //[R]: ADC VIN Measurement MSB
static constexpr auto REG_ADC_DATA_VIN_L = (0x4B); //[R]: ADC VIN Measurement LSB
static constexpr auto REG_ADC_DATA_PMID_M = (0x4C); //[R]: ADC PMID Measurement MSB
static constexpr auto REG_ADC_DATA_PMID_L = (0x4D); //[R]: ADC PMID Measurement LSB
static constexpr auto REG_ADC_DATA_IIN_M = (0x4E); //[R]: ADC IIN Measurement MSB
static constexpr auto REG_ADC_DATA_IIN_L = (0x4F); //[R]: ADC IIN Measurement LSB

static constexpr auto MAX16BIT = 65535;
static constexpr auto MAX14BIT = 16383;
static constexpr auto MAX12BIT = 4095;
static constexpr auto MAX10BIT = 1023;

// ADC Alarms

// ADCALARM_COMP1 Register
static constexpr auto REG_ADCALARM_COMP1_M = (0x52); //[R/W]: ADC Comparator 1 Threshold MSB
static constexpr auto REG_ADCALARM_COMP1_L = (0x53); //[R/W]: ADC Comparator 1 Threshold LSB
// Bitfield Masks
static constexpr auto ADCALARM_COMP1_M_DEF = (0x23); //8b00100011
static constexpr auto ADCALARM_COMP1_L_DEF = (0x20); //8b00100000

// ADCALARM_COMP2 Register
static constexpr auto REG_ADCALARM_COMP2_M = (0x54); //[R/W]: ADC Comparator 2 Threshold MSB
static constexpr auto REG_ADCALARM_COMP2_L = (0x55); //[R/W]: ADC Comparator 2 Threshold LSB
// Bitfield Masks
static constexpr auto ADCALARM_COMP2_M_DEF = (0x38); //8b00111000
static constexpr auto ADCALARM_COMP2_L_DEF = (0x90); //8b10010000

// ADCALARM_COMP3 Register
static constexpr auto REG_ADCALARM_COMP3_M = (0x56); //R/W: ADC Comparator 3 Threshold MSB
static constexpr auto REG_ADCALARM_COMP3_L = (0x57); //R/W: ADC Comparator 3 Threshold LSB

// Bitfield Masks
static constexpr auto ADCALARM_LSB = (0xF0); //b7-4: ADC Comparators Threshold LSB
static constexpr auto ADCALARM_POL = (1 << 3); //b3: ADC Comparators Polarity
// ADCALARM_ABOVE SET
static constexpr auto ADC_ALARM_BELOW = (0x00); //ADCALARM_ABOVE = Set Flag and send interrupt if ADC < comparator threshold
static constexpr auto ADC_ALARM_ABOVE = (0x01); //ADCALARM_ABOVE = Set Flag and send interrupt if ADC > comparator threshold

// ADC Enable

// ADC_READ_EN Register
static constexpr auto REG_ADC_READ_EN = (0x58); //[R/W]: ADC Channel Enable
// Bitfield Masks
static constexpr auto EN_IIN_READ_MASK = (1 << 7); //b7: Enable measurement for Input Current (IIN) Channel
static constexpr auto EN_PMID_READ_MASK = (1 << 6); //b6: Enable measurement for PMID Channel
static constexpr auto EN_ICHG_READ_MASK = (1 << 5); //b5: Enable measurement for Charge Current Channel
static constexpr auto EN_VIN_READ_MASK = (1 << 4); //b4: Enable measurement for Input Voltage (VIN) Channel
static constexpr auto EN_VBAT_READ_MASK = (1 << 3); //b3: Enable measurement for Battery Voltage (VBAT) Channel
static constexpr auto EN_TS_READ_MASK = (1 << 2); //b2: Enable measurement for TS Channel
static constexpr auto EN_ADCIN_READ_MASK = (1 << 1); //b1: Enable measurement for ADCIN Channel

static constexpr auto VALID_ADC_MASKS = (
        EN_IIN_READ_MASK  |
        EN_PMID_READ_MASK |
        EN_ICHG_READ_MASK |
        EN_VIN_READ_MASK  |
        EN_VBAT_READ_MASK |
        EN_TS_READ_MASK   |
        EN_ADCIN_READ_MASK );

static constexpr auto DIS_ALL_ADC_CH = (0x01); //Keep value of reserved bit 0
static constexpr auto EN_ALL_ADC_CH = (0xFE); //Keep value of reserved bit 0

// TS Settings

// TS_FASTCHGCTRL Register
static constexpr auto REG_TS_FASTCHGCTRL = (0x61); //[R/W]: TS Charge Control
// Bitfield Masks
static constexpr auto TS_FASTCHGCTRL_DEF = (0x34); //8b00110100

static constexpr auto TS_VBAT_REG_MASK = (0x70); //b6-4: Reduced target battery voltage during Warm
static constexpr auto TS_ICHRG_MASK = (0x07); //b2-0: Fast charge current when decreased by TS function
// TS_VBAT SEL
static constexpr auto TS_VBAT_NO_RED = (0x00); //TS_VBAT = Disabled
static constexpr auto TS_VBAT_50MV = (0x01); //TS_VBAT = VBAT_REG - 50 mV
static constexpr auto TS_VBAT_100MV = (0x02); //TS_VBAT = VBAT_REG - 100 mV
static constexpr auto TS_VBAT_150MV = (0x03); //TS_VBAT = VBAT_REG - 150 mV
static constexpr auto TS_VBAT_200MV = (0x04); //TS_VBAT = VBAT_REG - 200 mV
static constexpr auto TS_VBAT_250MV = (0x05); //TS_VBAT = VBAT_REG - 250 mV
static constexpr auto TS_VBAT_300MV = (0x06); //TS_VBAT = VBAT_REG - 300 mV
static constexpr auto TS_VBAT_350MV = (0x07); //TS_VBAT = VBAT_REG - 350 mV
// TS_ICHRG SEL
static constexpr auto TS_ICHRG_NO_RED = (0x00); //TS_ICHRG = Disabled
static constexpr auto TS_ICHRG_X_875 = (0x01); //TS_ICHRG = 0.875 x ICHG
static constexpr auto TS_ICHRG_X_750 = (0x02); //TS_ICHRG = 0.750 x ICHG
static constexpr auto TS_ICHRG_X_625 = (0x03); //TS_ICHRG = 0.625 x ICHG
static constexpr auto TS_ICHRG_X_500 = (0x04); //TS_ICHRG = 0.500 x ICHG
static constexpr auto TS_ICHRG_X_375 = (0x05); //TS_ICHRG = 0.375 x ICHG
static constexpr auto TS_ICHRG_X_250 = (0x06); //TS_ICHRG = 0.250 x ICHG
static constexpr auto TS_ICHRG_X_125 = (0x07); //TS_ICHRG = 0.125 x ICHG

// TS_COLD Register
static constexpr auto REG_TS_COLD = (0x62); //[R/W]: TS Cold Threshold
// Bitfield Mask
static constexpr auto TS_COLD_DEF = (0x7C); //8b01111100

// TS_COOL Register
static constexpr auto REG_TS_COOL = (0x63); //[R/W]: TS Cool Threshold
// Bitfield Mask
static constexpr auto TS_COOL_DEF = (0x6D); //8b01101101

// TS_WARM Register
static constexpr auto REG_TS_WARM = (0x64); //[R/W]: TS Warm Threshold
// Bitfield Mask
static constexpr auto TS_WARM_DEF = (0x38); //8b00111000

// TS_HOT Register
static constexpr auto REG_TS_HOT = (0x65); //[R/W]: TS Hot Threshold
// Bitfield Mask
static constexpr auto TS_HOT_DEF = (0x27); //8b00100111

static constexpr auto TS_TH_MV = 4.688f;

// IC Device ID

// DEVICE_ID Register
static constexpr auto REG_DEVICE_ID = (0x6F); //[R]: Device ID
// Bitfield Mask
static constexpr auto DEVICE_ID_DEF = (0x35); //8b00110101 = bq25155

enum class ChargeStatus : uint8_t {
    BQ_NOT_CHARGING = 0,
    BQ_PRECHARGE,
    BQ_FASTCHARGE,
    BQ_CHARGE_DONE,
    BQ_UNKNOWN_STATUS
};


} // namespace bq25155_const

class bq25155 {
public:
    bq25155();
    bq25155(TwoWire *wire, uint8_t address = bq25155_const::bq25155_ADDR);

    // begin I2C Communication, and initial settings for configuration pins
    bool begin(uint8_t CHEN_pin = 2, uint8_t INT_pin = 5, uint8_t LPM_pin = 20);
    
    // Defaults: 100mA fast charge, 4.2V charge voltage, 150mA input limit, safety timer nearest to 4h request
    bool initCHG(uint16_t BATVoltage_mV = 4200, bool En_FSCHG = true, uint32_t CHGCurrent_uA = 100000, uint32_t PCHGCurrent_uA = 20000,
                 uint16_t inputCurrentLimit_mA = 200, uint8_t ChgSftyTimer_hours = 4);

    // --- Configuration Functions ---
    // --- STAT0 Functions ---
    bool is_CHRG_CV();
    bool is_CHARGE_DONE();
    bool is_IINLIM_EN();
    bool is_VDPPM_EN();
    bool is_VINDPM_EN();
    bool is_THERMREG_EN();
    bool is_VIN_PGOOD();
    
    // --- STAT1 Functions ---
    bool is_VIN_OVP_TRIG();
    bool is_BAT_OCP_TRIG();
    bool is_BAT_UVLO();
    bool is_BAT_COLD();
    bool is_BAT_COOL();
    bool is_BAT_WARM();
    bool is_BAT_HOT();
    bool is_CHG_SUSPENDED();

    // --- STAT2 Functions ---
    bool is_Alarm_TRIG(uint8_t AlarmCh);
    bool is_TS_OPEN();

    // --- FLAGx Functions ---
    void ClearAllFlags();
    void readAllFLAGS();
    void FaultsDetected(uint8_t* faultsOut);
    
    // --- FLAG0 Functions ---
    uint8_t readFLAG0();
    uint8_t getCachedFLAG0();
    bool CHRG_CV_Flag();
    bool CHRG_DONE_Flag();
    bool IINLIM_Flag();
    bool VDPPM_Flag();
    bool VINDPM_Flag();
    bool THERMREG_Flag();
    bool VIN_PGOOD_Flag();

    // --- FLAG1 Functions ---
    uint8_t readFLAG1();
    uint8_t getCachedFLAG1();
    bool VIN_OVP_Flag();
    bool BAT_OCP_Flag();
    bool BAT_UVLO_Flag();
    bool TS_COLD_Flag();
    bool TS_COOL_Flag();
    bool TS_WARM_Flag();
    bool TS_HOT_Flag();

    // --- FLAG2 Functions ---
    uint8_t readFLAG2();
    uint8_t getCachedFLAG2();
    bool ADC_READY_Flag();
    bool ADC_Alarm_1_Flag();
    bool ADC_Alarm_2_Flag();
    bool ADC_Alarm_3_Flag();
    bool VTS_OPEN_Flag();

    // --- FLAG3 Functions ---
    uint8_t readFLAG3();
    uint8_t getCachedFLAG3();
    bool WD_FAULT_Flag();
    bool SAFETY_TMR_Flag();
    bool LDO_OCP_Flag();
    bool MRWAKE1_TMR_Flag();
    bool MRWAKE2_TMR_Flag();
    bool MRST_WARN_Flag();

    // --- MASK0 Functions ---
    bool get_CHRG_CV_MASK();
    bool set_CHRG_CV_MASK(bool m);
    bool get_CHARGE_DONE_MASK();
    bool set_CHARGE_DONE_MASK(bool m);
    bool get_IINLIM_ACTIVE_MASK();
    bool set_IINLIM_ACTIVE_MASK(bool m);
    bool get_VDPPM_ACTIVE_MASK();
    bool set_VDPPM_ACTIVE_MASK(bool m);
    bool get_VINDPM_ACTIVE_MASK();
    bool set_VINDPM_ACTIVE_MASK(bool m);
    bool get_THERMREG_ACTIVE_MASK();
    bool set_THERMREG_ACTIVE_MASK(bool m);
    bool get_VIN_PGOOD_MASK();
    bool set_VIN_PGOOD_MASK(bool m);
// --- MASK1 Functions ---
    bool get_VIN_OVP_FAULT_MASK();
    bool set_VIN_OVP_FAULT_MASK(bool m);
    bool get_BAT_OCP_FAULT_MASK();
    bool set_BAT_OCP_FAULT_MASK(bool m);
    bool get_BAT_UVLO_FAULT_MASK();
    bool set_BAT_UVLO_FAULT_MASK(bool m);
    bool get_TS_COLD_MASK();
    bool set_TS_COLD_MASK(bool m);
    bool get_TS_COOL_MASK();
    bool set_TS_COOL_MASK(bool m);
    bool get_TS_WARM_MASK();
    bool set_TS_WARM_MASK(bool m);
    bool get_TS_HOT_MASK();
    bool set_TS_HOT_MASK(bool m);
// --- MASK2 Functions ---
    bool get_ADC_READY_MASK();
    bool set_ADC_READY_MASK(bool m);
    bool get_COMP1_ALARM_MASK();
    bool set_COMP1_ALARM_MASK(bool m);
    bool get_COMP2_ALARM_MASK();
    bool set_COMP2_ALARM_MASK(bool m);
    bool get_COMP3_ALARM_MASK();
    bool set_COMP3_ALARM_MASK(bool m);
    bool get_TS_OPEN_MASK();
    bool set_TS_OPEN_MASK(bool m);
// --- MASK3 Functions ---
    bool get_WD_FAULT_MASK();
    bool set_WD_FAULT_MASK(bool m);
    bool get_SAFETY_TMR_FAULT_MASK();
    bool set_SAFETY_TMR_FAULT_MASK(bool m);
    bool get_LDO_OCP_FAULT_MASK();
    bool set_LDO_OCP_FAULT_MASK(bool m);
    bool get_MRWAKE1_TIMEOUT_MASK();
    bool set_MRWAKE1_TIMEOUT_MASK(bool m);
    bool get_MRWAKE2_TIMEOUT_MASK();
    bool set_MRWAKE2_TIMEOUT_MASK(bool m);
    bool get_MRRESET_WARN_MASK();
    bool set_MRRESET_WARN_MASK(bool m);
// --- VBAT Functions ---
    uint16_t getChargeVoltage();
    bool setChargeVoltage(uint16_t target_mV);
// --- Fast Charge Functions ---
    bool isFastChargeEnabled();
    bool DisableFastCharge();
    bool EnableFastCharge();
// --- Charging Current Functions ---
    uint32_t getChargeCurrent();
    bool setChargeCurrent(uint32_t current_uA);
// --- Pre-Charging Current Functions ---
    uint32_t getPrechargeCurrent();
    bool setPreChargeCurrent(uint32_t current_uA);
// --- Termination Current Functions ---
    uint8_t getITERM();
    bool setITERM(uint8_t percent);
    bool isTermEnabled();
    bool EnableTermination();
    bool DisableTermination();
// --- Under Voltage Threshold Functions ---
    bool getVLOWThrs();
    bool setVLOWThrsTo3_0V();
    bool setVLOWThrsTo2_8V();
// --- Over Current Set Functions ---
    uint8_t getIBATOCP();
    bool setIBATOCPto1200mA();
    bool setIBATOCPto1500mA();
    bool DisableIBATOCP();
// --- Under Voltage Set Functions ---
    uint8_t getUVLO();
    bool setUVLO(uint8_t code);
    bool setUVLOto3000mV();
    bool setUVLOto2800mV();
    bool setUVLOto2600mV();
    bool setUVLOto2400mV();
    bool setUVLOto2200mV();
    bool DisableUVLO();
// --- CHARGERCTRL0 Functions ---
    bool isTSEnabled();
    bool DisableTS();
    bool EnableTS();
    bool getTSControlMode();
    bool CustomTSControl();
    bool DisableTSControl();
    bool getVRHThrs();
    bool setVRHThrsTo140mV();
    bool setVRHThrsTo200mV();
    bool isWatchdogTEnabled();
    bool EnableWatchdogT();
    bool DisableWatchdogT();
    bool getSafetyTimerX();
    bool set1xSafetyTimer();
    bool set2xSafetyTimer();
    // Returns base timer setting (3/6/12h). If 2XTMR_EN is set, effective time may be longer outside CC/CV.
    uint8_t getChgSafetyTimer();
    bool setChgSafetyTimer(uint8_t code);
    bool setChgSafetyTimerto3h();
    bool setChgSafetyTimerto6h();
    bool setChgSafetyTimerto12h();
    bool DisableChgSafetyTimer();
// --- CHARGERCTRL1 Functions ---
    bool isVINDPMEnabled();
    bool EnableVINDPM();
    bool DisableVINDPM();
    uint8_t getVINDPM();
    bool setVINDPM(uint8_t code);
    bool setVINDPMto4200mV();
    bool setVINDPMto4300mV();
    bool setVINDPMto4400mV();
    bool setVINDPMto4500mV();
    bool setVINDPMto4600mV();
    bool setVINDPMto4700mV();
    bool setVINDPMto4800mV();
    bool setVINDPMto4900mV();
    bool isDPPMEnabled();
    bool EnableDPPM();
    bool DisableDPPM();
    uint8_t getThermalThreshold();
    bool setThermalThreshold(uint8_t code);
    bool setThermalTemperature(uint8_t Temp_C);
// --- ILIMCTRL Functions ---
    uint8_t getILIM();
    bool setILIM(uint8_t code);
    bool setILIMto50mA();
    bool setILIMto100mA();
    bool setILIMto150mA();
    bool setILIMto200mA();
    bool setILIMto300mA();
    bool setILIMto400mA();
    bool setILIMto500mA();
    bool setILIMto600mA();
// --- LDOCTRL Functions ---
    bool isLSLDOEnabled();
    bool DisableLSLDO();
    bool EnableLSLDO();
    uint16_t getLDOVoltage();
    bool setLDOVoltage(uint16_t target_mV);
    bool isLDOEnabled();
    bool isLSEnabled();
    bool EnableLDO();
    bool EnableLS();
// --- MRCTRL Functions ---
    bool isMRResetVINGated();
    bool DisableMRResetVINGated();
    bool EnableMRResetVINGated();
    bool getWake1Timer();
    bool setWake1TimerTo125ms();
    bool setWake1TimerTo500ms();
    bool getWake2Timer();
    bool setWake2TimerTo1s();
    bool setWake2TimerTo2s();
    uint8_t getRstWarnTimer();
    bool setRstWarnTimer(uint8_t code);
    bool setRstWarnTimerms(uint16_t mrst_ms);
    uint8_t getHWRstTimer();
    bool setHWRstTimer(uint8_t code);
    bool setHWRstTimerto4s();
    bool setHWRstTimerto8s();
    bool setHWRstTimerto10s();
    bool setHWRstTimerto14s();
// --- ICCTRL0 Functions ---
    bool isShipModeSet();
    bool DisableShipMode();
    bool EnableShipMode();
    uint8_t getAutoWKPTimer();
    bool setAutoWKPTimer(uint8_t code);
    bool setAutoWKPto0_6s();
    bool setAutoWKPto1_2s();
    bool setAutoWKPto2_4s();
    bool setAutoWKPto5s();
    bool AreGlobalIntMasksEnabled();
    bool DisableGlobalIntMasks();
    bool EnableGlobalIntMasks();
    bool isHWResetEnabled();
    bool DisableHWReset();
    bool EnableHWReset();
    bool isSWResetEnabled();
    bool DisableSWReset();
    bool EnableSWReset();
// --- ICCTRL1 Functions ---
    uint8_t getMRLongPressAction();
    bool setMRLongPressAsHWRST();
    bool setMRLongPressAsShipMode();
    bool DisableMRLongPress();
    bool getADCINMode();
    bool setADCINasGP();
    bool setADCINasNTC();
    uint8_t getPGMode();
    bool setPGasVINPG();
    bool setPGasDegMR();
    bool setPGasGPOD();
    uint8_t getPMIDMode();
    bool PwrPMIDwithVINorBAT();
    bool PwrPMIDwithBAT();
    bool FloatPMID();
    bool PullDownPMID();
// --- ICCTRL2 Functions ---
    uint8_t getPMID();
    bool setPMID(uint8_t code);
    bool setPMIDmV(uint16_t PMID_mV);
    bool isPGEnabled();
    bool EnablePG();
    bool DisablePG();
    bool isRST14sWDEnabled();
    bool DisableRST14sWD();
    bool EnableRST14sWD();
    bool isChargeEnabled();
    bool EnableCharge();
    bool DisableCharge();
// --- ADC Control and Read Functions ---
// --- REG_ADCCTRL0 Functions ---
    uint8_t getADCReadRate();
    bool ADCManualRead();
    bool ADCContinuousSamp();
    bool ADC1sSamp();
    bool ADC1mSamp();
    bool getADCConvStart();
    bool NoADCConv();
    bool InitADCManualMeas();
    uint8_t getADCConvSpeed();
    bool setADCSpeedTo24ms();
    bool setADCSpeedTo12ms();
    bool setADCSpeedTo6ms();
    bool setADCSpeedTo3ms();
    uint8_t getADCComp1Ch();
    bool setADCComp1Ch(uint8_t code);
    bool DisableADCComp1Ch();
// --- REG_ADCCTRL1 Functions ---
    uint8_t getADCComp2Ch();
    bool setADCComp2Ch(uint8_t code);
    bool DisableADCComp2Ch();
    uint8_t getADCComp3Ch();
    bool setADCComp3Ch(uint8_t code);
    bool DisableADCComp3Ch();
    // ADC readings
    uint16_t readVIN(uint8_t Vdecims);
    uint16_t readPMID(uint8_t Vdecims);
    uint32_t readIIN(uint8_t Vdecims);
    uint16_t readVBAT(uint8_t Vdecims);
    uint16_t readTS(uint8_t Vdecims);
    uint16_t readADCIN(uint8_t Vdecims);
    uint32_t readICHG(uint8_t Vdecims);
    // High-level functions for convenience
    // float getVBATVoltage();
// --- ADCALARM_COMPx Functions ---
    uint16_t readADCAlarms(uint8_t ADCAlarmCh, bool AlarmVal);
    bool setADCAlarms(uint8_t ADCAlarmCh, uint16_t AlarmVal, bool Polarity);
// --- ADC_READ_EN Functions ---
    bool isADCEnabled(uint8_t ADC_Ch);
    bool setADCChannel(uint8_t ADC_Ch, bool Ch_val);
    bool DisableAllADCCh();
    bool EnableAllADCCh();
    bool isIINADCChEnabled();
    bool DisableIINADCCh();
    bool EnableIINADCCh();
    bool isPMIDADCChEnabled();
    bool DisablePMIDADCCh();
    bool EnablePMIDADCCh();
    bool isICHGADCChEnabled();
    bool DisableICHGADCCh();
    bool EnableICHGADCCh();
    bool isVINADCChEnabled();
    bool DisableVINADCCh();
    bool EnableVINADCCh();
    bool isVBATADCChEnabled();
    bool DisableVBATADCCh();
    bool EnableVBATADCCh();
    bool isTSADCChEnabled();
    bool DisableTSADCCh();
    bool EnableTSADCCh();
    bool isADCINADCChEnabled();
    bool DisableADCINADCCh();
    bool EnableADCINADCCh();
// --- ADC_READ_EN Functions ---
    uint16_t getTSVCHG();
    bool setTSVCHG(uint16_t mV_TS);
    uint32_t getTSICHG();
    bool setTSICHG(uint16_t multiple);
// --- TS_THRESHOLDS Functions ---
    float getTSVAL(uint8_t TS_REG);
    bool setTSVAL(uint16_t TS_THRS_mV, uint8_t TS_REG);
// --- DEVICE_ID Functions ---
    uint8_t getDeviceID();
    String getDeviceIDString();
// --- Status and Faults (WIP) ---
//    bq25155_const::ChargeStatus getChargeStatus(); // Uses REG_STAT_0 (0x00)

private:
    TwoWire *_i2cPort; // Pointer to the Wire object
    uint8_t _i2cAddress;
    
    // Cached configuration pins
    uint8_t _CHEN_pin = 0xFF;
    uint8_t _INT_pin  = 0xFF;
    uint8_t _LPM_pin  = 0xFF;

    // Cached copies of FLAG registers
    uint8_t cachedFlag0 = 0;
    uint8_t cachedFlag1 = 0;
    uint8_t cachedFlag2 = 0;
    uint8_t cachedFlag3 = 0;

    // --- Low-level I2C access (for debugging or advanced use) ---
    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    uint16_t readRaw16BitRegister(uint8_t msb_reg, uint8_t lsb_reg);

    // --- Helper functions for converting values to register bits and vice-versa ---
    uint16_t KeepDecimals(uint32_t value, uint8_t digits);
    uint16_t GenADCVRead(uint8_t ADC_DATA_MSB, uint8_t ADC_DATA_LSB, uint8_t KeepDec, bool VRef);
    uint32_t GenADCIRead(uint8_t ADC_DATA_MSB, uint8_t ADC_DATA_LSB, uint8_t KeepDec);
    uint32_t GenADCIPRead(uint8_t ADC_DATA_MSB, uint8_t ADC_DATA_LSB, uint8_t KeepDec);
};

#endif // BQ25155_H
