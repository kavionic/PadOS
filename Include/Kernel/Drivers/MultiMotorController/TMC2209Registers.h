// This file is part of PadOS.
//
// Copyright (C) 1021-2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 20.03.2021 15:30

#pragma once

static constexpr uint8_t TMC2209_REG_GCONF          = 0x00;
static constexpr uint8_t TMC2209_REG_GSTAT          = 0x01;
static constexpr uint8_t TMC2209_REG_IFCNT          = 0x02;
static constexpr uint8_t TMC2209_REG_NODECONF       = 0x03;
static constexpr uint8_t TMC2209_REG_OTP_PROG       = 0x04;
static constexpr uint8_t TMC2209_REG_OTP_READ       = 0x05;
static constexpr uint8_t TMC2209_REG_IOIN           = 0x06;
static constexpr uint8_t TMC2209_REG_FACTORY_CONF   = 0x07;
static constexpr uint8_t TMC2209_REG_IHOLD_IRUN     = 0x10;
static constexpr uint8_t TMC2209_REG_TPOWERDOWN     = 0x11;
static constexpr uint8_t TMC2209_REG_TSTEP          = 0x12;
static constexpr uint8_t TMC2209_REG_TPWMTHRS       = 0x13;
static constexpr uint8_t TMC2209_REG_TCOOLTHRS      = 0x14;
static constexpr uint8_t TMC2209_REG_VACTUAL        = 0x22;
static constexpr uint8_t TMC2209_REG_SGTHRS         = 0x40;
static constexpr uint8_t TMC2209_REG_SG_RESULT      = 0x41;
static constexpr uint8_t TMC2209_REG_COOLCONF       = 0x42;
static constexpr uint8_t TMC2209_REG_MSCNT          = 0x6A;
static constexpr uint8_t TMC2209_REG_MSCURACT       = 0x6B;
static constexpr uint8_t TMC2209_REG_CHOPCONF       = 0x6C;
static constexpr uint8_t TMC2209_REG_DRV_STATUS     = 0x6F;
static constexpr uint8_t TMC2209_REG_PWMCONF        = 0x70;
static constexpr uint8_t TMC2209_REG_PWM_SCALE      = 0x71;
static constexpr uint8_t TMC2209_REG_PWM_AUTO       = 0x72;



static constexpr uint32_t TMC2209_GCONF_I_SCALE_ANALOG_Pos      = 0;    // I_scale_analog (Reset default=1)
static constexpr uint32_t TMC2209_GCONF_I_SCALE_ANALOG          = 0x1 << TMC2209_GCONF_I_SCALE_ANALOG_Pos;
static constexpr uint32_t TMC2209_GCONF_INTERNAL_RSENSE_Pos     = 1;    // internal_Rsense (Reset default: OTP)
static constexpr uint32_t TMC2209_GCONF_INTERNAL_RSENSE         = 0x1 << TMC2209_GCONF_INTERNAL_RSENSE_Pos;
static constexpr uint32_t TMC2209_GCONF_EN_SPREADCYCLE_Pos      = 2;    // en_spreadCycle (Reset default: OTP)
static constexpr uint32_t TMC2209_GCONF_EN_SPREADCYCLE          = 0x1 << TMC2209_GCONF_EN_SPREADCYCLE_Pos;
static constexpr uint32_t TMC2209_GCONF_SHAFT_Pos               = 3;    // controls motor direction
static constexpr uint32_t TMC2209_GCONF_SHAFT                   = 0x1 << TMC2209_GCONF_SHAFT_Pos;
static constexpr uint32_t TMC2209_GCONF_INDEX_OTPW_Pos          = 4;    // index_otpw
static constexpr uint32_t TMC2209_GCONF_INDEX_OTPW              = 0x1 << TMC2209_GCONF_INDEX_OTPW_Pos;
static constexpr uint32_t TMC2209_GCONF_INDEX_STEP_Pos          = 5;    // index_step
static constexpr uint32_t TMC2209_GCONF_INDEX_STEP              = 0x1 << TMC2209_GCONF_INDEX_STEP_Pos;
static constexpr uint32_t TMC2209_GCONF_PDN_DISABLE_Pos         = 6;    // pdn_disable
static constexpr uint32_t TMC2209_GCONF_PDN_DISABLE             = 0x1 << TMC2209_GCONF_PDN_DISABLE_Pos;
static constexpr uint32_t TMC2209_GCONF_MSTEP_REG_SELECT_Pos    = 7;    // mstep_reg_select
static constexpr uint32_t TMC2209_GCONF_MSTEP_REG_SELECT        = 0x1 << TMC2209_GCONF_MSTEP_REG_SELECT_Pos;
static constexpr uint32_t TMC2209_GCONF_MULTISTEP_FILT_Pos      = 8;    // multistep_filt (Reset default=1)
static constexpr uint32_t TMC2209_GCONF_MULTISTEP_FILT          = 0x1 << TMC2209_GCONF_MULTISTEP_FILT_Pos;
static constexpr uint32_t TMC2209_GCONF_TEST_MODE_Pos           = 9;    // test_mode 0
static constexpr uint32_t TMC2209_GCONF_TEST_MODE               = 0x1 << TMC2209_GCONF_TEST_MODE_Pos;

static constexpr uint32_t TMC2209_GSTAT_RESET_Pos               = 0;    // reset
static constexpr uint32_t TMC2209_GSTAT_RESET                   = 0x1 << TMC2209_GSTAT_RESET_Pos;
static constexpr uint32_t TMC2209_GSTAT_DRV_ERR_Pos             = 1;    // drv_err
static constexpr uint32_t TMC2209_GSTAT_DRV_ERR                 = 0x1 << TMC2209_GSTAT_DRV_ERR_Pos;
static constexpr uint32_t TMC2209_GSTAT_UV_CP_Pos               = 2;    // uv_cp
static constexpr uint32_t TMC2209_GSTAT_UV_CP                   = 0x1 << TMC2209_GSTAT_UV_CP_Pos;

static constexpr uint32_t TMC2209_IFCNT_IFCNT_Pos               = 0;    // Interface transmission counter. This register becomes incremented with each
                                                                        // successful UART interface write access. Read out to check the serial
                                                                        // transmission for lost data. Read accesses do not change the content.
                                                                        // The counter wraps around from 255 to 0.
static constexpr uint32_t TMC2209_IFCNT_IFCNT_Msk               = 0xFF << TMC2209_IFCNT_IFCNT_Pos; // min.: 0, max.: 255, default: 0

static constexpr uint32_t TMC2209_NODECONF_SENDDELAY_Pos        = 8;    // Send delay for read access (time until reply is sent):
                                                                        //  0, 1:    8 bit times
                                                                        //  2, 3:    3*8 bit times
                                                                        //  4, 5:    5*8 bit times
                                                                        //  6, 7:    7*8 bit times
                                                                        //  8, 9:    9*8 bit times
                                                                        //  10, 11:  11*8 bit times
                                                                        //  12, 13:  13*8 bit times
                                                                        //  14, 15:  15*8 bit times
static constexpr uint32_t TMC2209_NODECONF_SENDDELAY_Msk        = 0xF << TMC2209_NODECONF_SENDDELAY_Pos; // min.: 0, max.: 15, default: 0

static constexpr uint32_t TMC2209_OTP_PROG_OTPBIT_Pos           = 0;    // Selection of OTP bit to be programmed to the selected byte location (n=0..7: programs bit n to a logic 1)
static constexpr uint32_t TMC2209_OTP_PROG_OTPBIT_Msk           = 0x7 << TMC2209_OTP_PROG_OTPBIT_Pos; // min.: 0, max.: 7, default: 0
static constexpr uint32_t TMC2209_OTP_PROG_OTPBYTE_Pos          = 4;    // Selection of OTP programming location (0, 1 or 2)
static constexpr uint32_t TMC2209_OTP_PROG_OTPBYTE_Msk          = 0x3 << TMC2209_OTP_PROG_OTPBYTE_Pos; // min.: 0, max.: 3, default: 0
static constexpr uint32_t TMC2209_OTP_PROG_OTPMAGIC_Pos         = 8;    // Set to 0xBD to enable programming. A programming time of
                                                                        // minimum 10ms per bit is recommended (check by reading OTP_READ).
static constexpr uint32_t TMC2209_OTP_PROG_OTPMAGIC_Msk         = 0xFF << TMC2209_OTP_PROG_OTPMAGIC_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_OTP_READ_FCLKTRIM_Pos         = 0;    // Reset default for FCLKTRIM;
                                                                        //  0: lowest frequency setting …
                                                                        //  31: highest frequency setting.
                                                                        // Attention: This value is pre-programmed by factory clock trimming to the default
                                                                        // clock frequency of 12MHz and differs between individual ICs! It should not be altered.
static constexpr uint32_t TMC2209_OTP_READ_FCLKTRIM_Msk         = 0x1F << TMC2209_OTP_READ_FCLKTRIM_Pos;     // Reset default for FCLKTRIM
static constexpr uint32_t TMC2209_OTP_READ_OTTRIM_Pos           = 5;    // Reset default for OTTRIM:
                                                                        //  0: OTTRIM=%00 (143°C)
                                                                        //  1: OTTRIM=%01 (150°C) (internal power stage temperature about 10°C above the sensor temperature limit)
static constexpr uint32_t TMC2209_OTP_READ_OTTRIM               = 0x1  << TMC2209_OTP_READ_OTTRIM_Pos;       // Reset default for OTTRIM
static constexpr uint32_t TMC2209_OTP_READ_INTERNAL_RSENSE_Pos  = 6;    // Reset default for GCONF.internal_Rsense:
                                                                        //  0: External sense resistors
                                                                        //  1: Internal sense resistors
static constexpr uint32_t TMC2209_OTP_READ_INTERNAL_RSENSE      = 0x1  << TMC2209_OTP_READ_INTERNAL_RSENSE_Pos; // Reset default for GCONF.internal_Rsense
static constexpr uint32_t TMC2209_OTP_READ_TBL_Pos              = 7;    // Reset default for TBL: 0: TBL=%10  1: TBL=%01
static constexpr uint32_t TMC2209_OTP_READ_TBL                  = 0x1  << TMC2209_OTP_READ_TBL_Pos;          // Reset default for TBL
static constexpr uint32_t TMC2209_OTP_READ_PWM_GRAD_Pos         = 8;    // Depending on otp_en_SpreadCycle. Reset default for PWM_GRAD as defined by (0..15):
                                                                        //  0→14, 1→16, 2→18, 3→21, 4→24, 5→27, 6→31, 7→35,
                                                                        //  8→40, 9→46, 10→52, 11→59, 12→67, 13→77, 14→88, 15→100
static constexpr uint32_t TMC2209_OTP_READ_PWM_GRAD_Msk         = 0xF  << TMC2209_OTP_READ_PWM_GRAD_Pos;      // Reset default for PWM_GRAD
static constexpr uint32_t TMC2209_OTP_READ_PWM_AUTOGRAD_Pos     = 12;   // Depending on otp_en_SpreadCycle: 0: pwm_autograd=1  1: pwm_autograd=0
static constexpr uint32_t TMC2209_OTP_READ_PWM_AUTOGRAD         = 0x1  << TMC2209_OTP_READ_PWM_AUTOGRAD_Pos;  // Depending on otp_en_SpreadCycle
static constexpr uint32_t TMC2209_OTP_READ_TPWM_THRS_Pos        = 13;   // Depending on otp_en_SpreadCycle. Reset default for TPWM_THRS as defined by (0..7):
                                                                        //  0→0, 1→200, 2→300, 3→400, 4→500, 5→800, 6→1200, 7→4000
static constexpr uint32_t TMC2209_OTP_READ_TPWM_THRS_Msk        = 0x7  << TMC2209_OTP_READ_TPWM_THRS_Pos;      // Reset default for TPWM_THRS
static constexpr uint32_t TMC2209_OTP_READ_PWM_OFS_Pos          = 16;   // Depending on otp_en_SpreadCycle:
                                                                        //  0: PWM_OFS=36
                                                                        //  1: PWM_OFS=00 (no feed forward scaling); pwm_autograd=0
static constexpr uint32_t TMC2209_OTP_READ_PWM_OFS              = 0x1  << TMC2209_OTP_READ_PWM_OFS_Pos;       // Depending on otp_en_SpreadCycle
static constexpr uint32_t TMC2209_OTP_READ_PWM_REG_Pos          = 17;   // Reset default for PWM_REG:
                                                                        //  0: PWM_REG=%1000 (max. 4 increments / cycle)
                                                                        //  1: PWM_REG=%0010 (max. 1 increment / cycle)
static constexpr uint32_t TMC2209_OTP_READ_PWM_REG              = 0x1  << TMC2209_OTP_READ_PWM_REG_Pos;       // Reset default for PWM_REG
static constexpr uint32_t TMC2209_OTP_READ_PWM_FREQ_Pos         = 18;   // Reset default for PWM_FREQ:
                                                                        //  0: PWM_FREQ=%01=2/683
                                                                        //  1: PWM_FREQ=%10=2/512
static constexpr uint32_t TMC2209_OTP_READ_PWM_FREQ             = 0x1  << TMC2209_OTP_READ_PWM_FREQ_Pos;      // Reset default for PWM_FREQ
static constexpr uint32_t TMC2209_OTP_READ_IHOLDDELAY_Pos       = 19;   // Reset default for IHOLDDELAY: %00→1, %01→2, %10→4, %11→8
static constexpr uint32_t TMC2209_OTP_READ_IHOLDDELAY_Msk       = 0x3  << TMC2209_OTP_READ_IHOLDDELAY_Pos;    // Reset default for IHOLDDELAY
static constexpr uint32_t TMC2209_OTP_READ_IHOLD_Pos            = 21;   // Reset default for standstill current IHOLD (used only if current reduction
                                                                        // enabled, e.g. pin PDN_UART low).
                                                                        //  %00→16 (≈53% IRUN), %01→2 (≈9% IRUN), %10→8 (≈28% IRUN), %11→24 (≈78% IRUN).
                                                                        // (Reset default IRUN=31.)
static constexpr uint32_t TMC2209_OTP_READ_IHOLD_Msk            = 0x3  << TMC2209_OTP_READ_IHOLD_Pos;         // Reset default for IHOLD
static constexpr uint32_t TMC2209_OTP_READ_EN_SPREADCYCLE_Pos   = 23;   // This flag determines if the driver defaults to SpreadCycle or to StealthChop.
                                                                        //  0:  Default StealthChop (GCONF.en_SpreadCycle=0); OTP 1.0..1.7 & 2.0 used for
                                                                        //      StealthChop; SpreadCycle defaults: HEND=0; HSTART=5; TOFF=3.
                                                                        //  1:  Default SpreadCycle (GCONF.en_SpreadCycle=1); OTP 1.0..1.7 & 2.0 used for
                                                                        //      SpreadCycle; StealthChop defaults: PWM_GRAD=0; TPWM_THRS=0; PWM_OFS=36; pwm_autograd=1.
static constexpr uint32_t TMC2209_OTP_READ_EN_SPREADCYCLE       = 0x1  << TMC2209_OTP_READ_EN_SPREADCYCLE_Pos; // Default mode selection

static constexpr uint32_t TMC2209_IOIN_ENN_Pos                  = 0;
static constexpr uint32_t TMC2209_IOIN_ENN                      = 0x1 << TMC2209_IOIN_ENN_Pos;
static constexpr uint32_t TMC2209_IOIN_MS1_Pos                  = 2;
static constexpr uint32_t TMC2209_IOIN_MS1                      = 0x1 << TMC2209_IOIN_MS1_Pos;
static constexpr uint32_t TMC2209_IOIN_MS2_Pos                  = 3;
static constexpr uint32_t TMC2209_IOIN_MS2                      = 0x1 << TMC2209_IOIN_MS2_Pos;
static constexpr uint32_t TMC2209_IOIN_DIAG_Pos                 = 4;
static constexpr uint32_t TMC2209_IOIN_DIAG                     = 0x1 << TMC2209_IOIN_DIAG_Pos;
static constexpr uint32_t TMC2209_IOIN_PDN_UART_Pos             = 6;
static constexpr uint32_t TMC2209_IOIN_PDN_UART                 = 0x1 << TMC2209_IOIN_PDN_UART_Pos;
static constexpr uint32_t TMC2209_IOIN_STEP_Pos                 = 7;
static constexpr uint32_t TMC2209_IOIN_STEP                     = 0x1 << TMC2209_IOIN_STEP_Pos;
static constexpr uint32_t TMC2209_IOIN_SPREAD_EN_Pos            = 8;
static constexpr uint32_t TMC2209_IOIN_SPREAD_EN                = 0x1 << TMC2209_IOIN_SPREAD_EN_Pos;
static constexpr uint32_t TMC2209_IOIN_DIR_Pos                  = 9;
static constexpr uint32_t TMC2209_IOIN_DIR                      = 0x1 << TMC2209_IOIN_DIR_Pos;
static constexpr uint32_t TMC2209_IOIN_VERSION_Pos              = 24;   // VERSION: 0x21=first version of the IC Identical numbers mean full digital compatibility.
static constexpr uint32_t TMC2209_IOIN_VERSION_Msk              = 0xFF << TMC2209_IOIN_VERSION_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_IOIN_VERSION_CURRENT          = 0x21 << TMC2209_IOIN_VERSION_Pos;

static constexpr uint32_t TMC2209_FACTORY_CONF_FCLKTRIM_Pos     = 0;    // FCLKTRIM (Reset default: OTP)
                                                                        //  0-31:   Lowest to highest clock frequency. Check at charge pump output.
                                                                        //          The frequency span is not guaranteed, but it is tested, that tuning to
                                                                        //          12MHz internal clock is possible. The devices come preset to 12MHz clock
                                                                        //          frequency by OTP programming.
static constexpr uint32_t TMC2209_FACTORY_CONF_FCLKTRIM_Msk     = 0x1F << TMC2209_FACTORY_CONF_FCLKTRIM_Pos; // min.: 0, max.: 31, default: 0
static constexpr uint32_t TMC2209_FACTORY_CONF_OTTRIM_Pos       = 8;    // OTTRIM (Default: OTP)
                                                                        //  %00:  OT=143°C, OTPW=120°C
                                                                        //  %01:  OT=150°C, OTPW=120°C
                                                                        //  %10:  OT=150°C, OTPW=143°C
                                                                        //  %11:  OT=157°C, OTPW=143°C
static constexpr uint32_t TMC2209_FACTORY_CONF_OTTRIM_Msk       = 0x3 << TMC2209_FACTORY_CONF_OTTRIM_Pos; // min.: 0, max.: 3, default: 0

static constexpr uint32_t TMC2209_IHOLD_IRUN_IHOLD_Pos          = 0;    // IHOLD (Reset default: OTP) Standstill current (0=1/32...31=32/32) In combination
                                                                        // with stealthChop mode, setting IHOLD=0 allows to choose freewheeling or coil short
                                                                        // circuit (passive braking) for motor stand still.
static constexpr uint32_t TMC2209_IHOLD_IRUN_IHOLD_Msk          = 0x1F << TMC2209_IHOLD_IRUN_IHOLD_Pos; // min.: 0, max.: 31, default: 0
static constexpr uint32_t TMC2209_IHOLD_IRUN_IRUN_Pos           = 8;    // IRUN (Reset default=31) Motor run current (0=1/32...31=32/32) Hint: Choose sense
                                                                        // resistors in a way, that normal IRUN is 16 to 31 for best microstep performance.
static constexpr uint32_t TMC2209_IHOLD_IRUN_IRUN_Msk           = 0x1F << TMC2209_IHOLD_IRUN_IRUN_Pos; // min.: 0, max.: 31, default: 0
static constexpr uint32_t TMC2209_IHOLD_IRUN_IHOLDDELAY_Pos     = 16;   // IHOLDDELAY (Reset default: OTP) Controls the number of clock cycles for motor
                                                                        // power down after standstill is detected (stst=1) and TPOWERDOWN has expired.
                                                                        // The smooth transition avoids a motor jerk upon power down.
                                                                        //  0:      Instant power down
                                                                        //  1..15:  Delay per current reduction step in multiple of 2^18 clocks
static constexpr uint32_t TMC2209_IHOLD_IRUN_IHOLDDELAY_Msk     = 0xF << TMC2209_IHOLD_IRUN_IHOLDDELAY_Pos; // min.: 0, max.: 15, default: 0

static constexpr uint32_t TMC2209_TPOWERDOWN_TPOWERDOWN_Pos     = 0;    // (Reset default=20) Sets the delay time from stand still (stst) detection to motor
                                                                        // current power down. Time range is about 0 to 5.6 seconds. 0...((2^8)-1) * 2^18 tclk
                                                                        // Attention: A minimum setting of 2 is required to allow automatic tuning
                                                                        //            of stealthChop PWM_OFFS_AUTO.
static constexpr uint32_t TMC2209_TPOWERDOWN_TPOWERDOWN_Msk     = 0xFF << TMC2209_TPOWERDOWN_TPOWERDOWN_Pos; // min.: 0, max.: 255, default: 0

static constexpr uint32_t TMC2209_TSTEP_TSTEP_Pos               = 0;    // Actual measured time between two 1/256 microsteps derived from the step input
                                                                        // frequency in units of 1/fCLK. Measured value is (2^20)-1 in case of overflow or
                                                                        // stand still. The TSTEP related threshold uses a hysteresis of 1/16 of the compare
                                                                        // value to compensate for jitter in the clock or the step frequency:
                                                                        // (Txxx*15/16)-1 is the lower compare value for each TSTEP based comparison.
                                                                        // This means, that the lower switching velocity equals the calculated setting,
                                                                        // but the upper switching velocity is higher as defined by the hysteresis setting.
static constexpr uint32_t TMC2209_TSTEP_TSTEP_Msk               = 0xFFFFF << TMC2209_TSTEP_TSTEP_Pos; // min.: 0, max.: 1048575, default: 0

static constexpr uint32_t TMC2209_TPWMTHRS_TPWMTHRS_Pos         = 0;    // Sets the upper velocity for stealthChop voltage PWM mode.
                                                                        // For TSTEP = TPWMTHRS, stealthChop PWM mode is enabled, if configured.
                                                                        // When the velocity exceeds the limit set by TPWMTHRS,
                                                                        // the driver switches to spreadCycle. 0 = Disabled
static constexpr uint32_t TMC2209_TPWMTHRS_TPWMTHRS_Msk         = 0xFFFFF << TMC2209_TPWMTHRS_TPWMTHRS_Pos; // min.: 0, max.: 1048575, default: 0

static constexpr uint32_t TMC2209_VACTUAL_VACTUAL_Pos           = 0;    // VACTUAL allows moving the motor by UART control.
                                                                        // It gives the motor velocity in +-(2^23)-1 [µsteps / t]
                                                                        //  0:  Normal operation. Driver reacts to STEP input.
                                                                        //  !0: Motor moves with the velocity given by VACTUAL.
                                                                        //      Step pulses can be monitored via INDEX output.
                                                                        //      The motor direction is controlled by the sign of VACTUAL.
static constexpr uint32_t TMC2209_VACTUAL_VACTUAL_Msk           = 0xFFFFFF << TMC2209_VACTUAL_VACTUAL_Pos; // min.: -8388608, max.: 8388607, default: 0

static constexpr uint32_t TMC2209_COOLCONF_SEMIN_Pos            = 0;
static constexpr uint32_t TMC2209_COOLCONF_SEMIN_Msk            = 0xF << TMC2209_COOLCONF_SEMIN_Pos;
static constexpr uint32_t TMC2209_COOLCONF_SEUP_Pos             = 5;
static constexpr uint32_t TMC2209_COOLCONF_SEUP_Msk             = 0x3 << TMC2209_COOLCONF_SEUP_Pos;
static constexpr uint32_t TMC2209_COOLCONF_SEMAX_Pos            = 8;
static constexpr uint32_t TMC2209_COOLCONF_SEMAX_Msk            = 0xF << TMC2209_COOLCONF_SEMAX_Pos;
static constexpr uint32_t TMC2209_COOLCONF_SEDN_Pos             = 13;
static constexpr uint32_t TMC2209_COOLCONF_SEDN_Msk             = 0x3 << TMC2209_COOLCONF_SEDN_Pos;
static constexpr uint32_t TMC2209_COOLCONF_SEIMIN_Pos           = 15;
static constexpr uint32_t TMC2209_COOLCONF_SEIMIN               = 0x1 << TMC2209_COOLCONF_SEIMIN_Pos;

static constexpr uint32_t TMC2209_MSCNT_MSCNT_Pos               = 0;    // Microstep counter. Indicates actual position in the microstep table for CUR_A.
                                                                        // CUR_B uses an offset of 256 into the table. Reading out MSCNT allows
                                                                        // determination of the motor position within the electrical wave.
static constexpr uint32_t TMC2209_MSCNT_MSCNT_Msk               = 0x3FF << TMC2209_MSCNT_MSCNT_Pos; // min.: 0, max.: 1023, default: 0

static constexpr uint32_t TMC2209_MSCURACT_CUR_A_Pos            = 0;    // (signed) Actual microstep current for motor phase A as read from
                                                                        // the internal sine wave table (not scaled by current setting)
static constexpr uint32_t TMC2209_MSCURACT_CUR_A_Msk            = 0x1FF << TMC2209_MSCURACT_CUR_A_Pos; // min.: -255, max.: 255, default: 0
static constexpr uint32_t TMC2209_MSCURACT_CUR_B_Pos            = 16;   // (signed) Actual microstep current for motor phase B as read from
                                                                        // the internal sine wave table (not scaled by current setting)
static constexpr uint32_t TMC2209_MSCURACT_CUR_B_Msk            = 0x1FF << TMC2209_MSCURACT_CUR_B_Pos; // min.: -255, max.: 255, default: 0

static constexpr uint32_t TMC2209_CHOPCONF_TOFF_Pos             = 0;    // chopper off time and driver enable, Off time setting controls duration of slow
                                                                        // decay phase (Nclk = 12 + 32*Toff),
                                                                        //  %0000:              Driver disable, all bridges off
                                                                        //  %0001:              1 - use only with TBL = 2
                                                                        //  %0010 ... %1111:    2 - 15 (Default: OTP, resp. 3 in stealthChop mode)
static constexpr uint32_t TMC2209_CHOPCONF_TOFF_Msk             = 0xF << TMC2209_CHOPCONF_TOFF_Pos; // min.: 0, max.: 7, default: 0
static constexpr uint32_t TMC2209_CHOPCONF_HSTRT_Pos            = 4;    // hysteresis start value added to HEND,
                                                                        //  %000 - %111:    Add 1, 2, ..., 8 to hysteresis low value HEND (1/512 of this
                                                                        //                  setting adds to current setting)
                                                                        // Attention: Effective HEND+HSTRT <= 16.
                                                                        // Hint: Hysteresis decrement is done each 16 clocks.
                                                                        // (Default: OTP, resp. 0 in stealthChop mode)
static constexpr uint32_t TMC2209_CHOPCONF_HSTRT_Msk            = 0x7 << TMC2209_CHOPCONF_HSTRT_Pos; // min.: 0, max.: 7, default: 0
static constexpr uint32_t TMC2209_CHOPCONF_HEND_Pos             = 7;    // hysteresis low value OFFSET sine wave offset,
                                                                        //  %0000 - %1111:  Hysteresis is -3, -2, -1, 0, 1, ..., 12 (1/512 of this setting
                                                                        //                  adds to current setting) This is the hysteresis value which
                                                                        //                  becomes used for the hysteresis chopper.
                                                                        // (Default: OTP, resp. 5 in stealthChop mode)
static constexpr uint32_t TMC2209_CHOPCONF_HEND_Msk             = 0xF << TMC2209_CHOPCONF_HEND_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_CHOPCONF_TBL_Pos              = 15;   // blank time select (Default: OTP)
                                                                        //  %00 - %11: Set comparator blank time to 16, 24, 32 or 40 clocks
                                                                        // Hint: %00 or %01 is recommended for most applications
static constexpr uint32_t TMC2209_CHOPCONF_TBL_Msk              = 0x3 << TMC2209_CHOPCONF_TBL_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_CHOPCONF_VSENSE_Pos           = 17;   // sense resistor voltage based current scaling
static constexpr uint32_t TMC2209_CHOPCONF_VSENSE               = 0x1 << TMC2209_CHOPCONF_VSENSE_Pos;
static constexpr uint32_t TMC2209_CHOPCONF_MRES_Pos             = 24;   // MRES micro step resolution,
                                                                        //  %0000: Native 256 microstep setting.
                                                                        //  %0001 - %1000: 128, 64, 32, 16, 8, 4, 2, FULLSTEP: Reduced microstep resolution.
                                                                        // The resolution gives the number of microstep entries per sine quarter wave.
                                                                        // When choosing a lower microstep resolution, the driver automatically uses
                                                                        // microstep positions which result in a symmetrical wave. Number of microsteps per
                                                                        // step pulse = 2^MRES (Selection by pins unless disabled by GCONF. mstep_reg_select)
static constexpr uint32_t TMC2209_CHOPCONF_MRES_Msk             = 0xF << TMC2209_CHOPCONF_MRES_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_CHOPCONF_INTPOL_Pos           = 28;   // interpolation to 256 microsteps
static constexpr uint32_t TMC2209_CHOPCONF_INTPOL               = 0x1 << TMC2209_CHOPCONF_INTPOL_Pos;
static constexpr uint32_t TMC2209_CHOPCONF_DEDGE_Pos            = 29;   // enable double edge step pulses
static constexpr uint32_t TMC2209_CHOPCONF_DEDGE                = 0x1 << TMC2209_CHOPCONF_DEDGE_Pos;
static constexpr uint32_t TMC2209_CHOPCONF_DISS2G_Pos           = 30;   // short to GND protection disable
static constexpr uint32_t TMC2209_CHOPCONF_DISS2G               = 0x1 << TMC2209_CHOPCONF_DISS2G_Pos;
static constexpr uint32_t TMC2209_CHOPCONF_DISS2VS_Pos          = 31;   // Low side short protection disable
static constexpr uint32_t TMC2209_CHOPCONF_DISS2VS              = 0x1 << TMC2209_CHOPCONF_DISS2VS_Pos;

static constexpr uint32_t TMC2209_DRV_STATUS_OTPW_Pos           = 0;    // overtemperature prewarning flag
static constexpr uint32_t TMC2209_DRV_STATUS_OTPW               = 0x1 << TMC2209_DRV_STATUS_OTPW_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_OT_Pos             = 1;    // overtemperature flag
static constexpr uint32_t TMC2209_DRV_STATUS_OT                 = 0x1 << TMC2209_DRV_STATUS_OT_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_S2GA_Pos           = 2;    // short to ground indicator phase A
static constexpr uint32_t TMC2209_DRV_STATUS_S2GA               = 0x1 << TMC2209_DRV_STATUS_S2GA_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_S2GB_Pos           = 3;    // short to ground indicator phase B
static constexpr uint32_t TMC2209_DRV_STATUS_S2GB               = 0x1 << TMC2209_DRV_STATUS_S2GB_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_S2VSA_Pos          = 4;    // low side short indicator phase A
static constexpr uint32_t TMC2209_DRV_STATUS_S2VSA              = 0x1 << TMC2209_DRV_STATUS_S2VSA_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_S2VSB_Pos          = 5;    // low side short indicator phase B
static constexpr uint32_t TMC2209_DRV_STATUS_S2VSB              = 0x1 << TMC2209_DRV_STATUS_S2VSB_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_OLA_Pos            = 6;    // open load indicator phase A
static constexpr uint32_t TMC2209_DRV_STATUS_OLA                = 0x1 << TMC2209_DRV_STATUS_OLA_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_OLB_Pos            = 7;    // open load indicator phase B
static constexpr uint32_t TMC2209_DRV_STATUS_OLB                = 0x1 << TMC2209_DRV_STATUS_OLB_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_T120_Pos           = 8;    // 120°C comparator
static constexpr uint32_t TMC2209_DRV_STATUS_T120               = 0x1 << TMC2209_DRV_STATUS_T120_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_T143_Pos           = 9;    // 143°C comparator
static constexpr uint32_t TMC2209_DRV_STATUS_T143               = 0x1 << TMC2209_DRV_STATUS_T143_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_T150_Pos           = 10;   // 150°C comparator
static constexpr uint32_t TMC2209_DRV_STATUS_T150               = 0x1 << TMC2209_DRV_STATUS_T150_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_T157_Pos           = 11;   // 157°C comparator
static constexpr uint32_t TMC2209_DRV_STATUS_T157               = 0x1 << TMC2209_DRV_STATUS_T157_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_CS_ACTUAL_Pos      = 16;   // actual motor current
static constexpr uint32_t TMC2209_DRV_STATUS_CS_ACTUAL_Msk      = 0x1F << TMC2209_DRV_STATUS_CS_ACTUAL_Pos; // min.: 0, max.: 31, default: 0
static constexpr uint32_t TMC2209_DRV_STATUS_STEALTH_Pos        = 30;   // stealthChop indicator
static constexpr uint32_t TMC2209_DRV_STATUS_STEALTH            = 0x1 << TMC2209_DRV_STATUS_STEALTH_Pos;
static constexpr uint32_t TMC2209_DRV_STATUS_STST_Pos           = 31;   // standstill indicator
static constexpr uint32_t TMC2209_DRV_STATUS_STST               = 0x1 << TMC2209_DRV_STATUS_STST_Pos;

static constexpr uint32_t TMC2209_PWMCONF_PWM_OFS_Pos           = 0;    // User defined PWM amplitude offset (0-255) related to full motor
                                                                        // current (CS_ACTUAL=31) in stand still. (Reset default=36)
                                                                        // When using automatic scaling (pwm_autoscale=1) the value is used for initialization,
                                                                        // only. The autoscale function starts with PWM_SCALE_AUTO=PWM_OFS and finds the
                                                                        // required offset to yield the target current automatically. PWM_OFS = 0 will disable
                                                                        // scaling down motor current below a motor specific lower measurement threshold.
                                                                        // This setting should only be used under certain conditions, i.e. when the power
                                                                        // supply voltage can vary up and down by a factor of two or more. It prevents the
                                                                        // motor going out of regulation, but it also prevents power down below the regulation
                                                                        // limit. PWM_OFS > 0 allows automatic scaling to low PWM duty cycles even below the
                                                                        // lower regulation threshold. This allows low (standstill) current settings based on
                                                                        // the actual (hold) current scale (register IHOLD_IRUN).
static constexpr uint32_t TMC2209_PWMCONF_PWM_OFS_Msk           = 0xFF << TMC2209_PWMCONF_PWM_OFS_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_PWMCONF_PWM_GRAD_Pos          = 8;    // Velocity dependent gradient for PWM amplitude: PWM_GRAD * 256 / TSTEP This value is
                                                                        // added to PWM_AMPL to compensate for the velocity-dependent motor back-EMF. With
                                                                        // automatic scaling (pwm_autoscale=1) the value is used for first initialization,
                                                                        // only. Set PWM_GRAD to the application specific value (it can be read out from
                                                                        // PWM_GRAD_AUTO) to speed up the automatic tuning process. An approximate value
                                                                        // can be stored to OTP by programming OTP_PWM_GRAD.
static constexpr uint32_t TMC2209_PWMCONF_PWM_GRAD_Msk          = 0xFF << TMC2209_PWMCONF_PWM_GRAD_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_PWMCONF_PWM_FREQ_Pos          = 16;   // %00:   fPWM=2/1024 fCLK
                                                                        // %01:   fPWM=2/683 fCLK
                                                                        // %10:   fPWM=2/512 fCLK
                                                                        // %11:   fPWM=2/410 fCLK
static constexpr uint32_t TMC2209_PWMCONF_PWM_FREQ_Msk          = 0x3 << TMC2209_PWMCONF_PWM_FREQ_Pos; // min.: 0, max.: 3, default: 0
static constexpr uint32_t TMC2209_PWMCONF_PWM_AUTOSCALE_Pos     = 18;   // 0: User defined feed forward PWM amplitude. The current settings IRUN and IHOLD are
                                                                        //    not enforced by regulation but scale the PWM amplitude, only! The resulting PWM
                                                                        //    amplitude (limited to 0…255) is: PWM_OFS * ((CS_ACTUAL+1) / 32)  + PWM_GRAD * 256 / TSTEP 
                                                                        // 1: Enable automatic current control (Reset default) 
static constexpr uint32_t TMC2209_PWMCONF_PWM_AUTOSCALE         = 0x1 << TMC2209_PWMCONF_PWM_AUTOSCALE_Pos; 
static constexpr uint32_t TMC2209_PWMCONF_PWM_AUTOGRAD_Pos      = 19;
static constexpr uint32_t TMC2209_PWMCONF_PWM_AUTOGRAD          = 0x1 << TMC2209_PWMCONF_PWM_AUTOGRAD_Pos;
static constexpr uint32_t TMC2209_PWMCONF_FREEWHEEL_Pos         = 20;   // Stand still option when motor current setting is zero (I_HOLD=0).
                                                                        //  %00: Normal operation
                                                                        //  %01: Freewheeling 
                                                                        //  %10: Coil shorted using LS drivers 
                                                                        //  %11: Coil shorted using HS drivers
static constexpr uint32_t TMC2209_PWMCONF_FREEWHEEL_Msk         = 0x3 << TMC2209_PWMCONF_FREEWHEEL_Pos; // min.: 0, max.: 3, default: 0
static constexpr uint32_t TMC2209_PWMCONF_PWM_REG_Pos           = 24;   // User defined  maximum  PWM amplitude  change per  half  wave when using pwm_autoscale=1. (1...15):
                                                                        //  1:  0.5 increments (slowest regulation)
                                                                        //  2:  1 increment (default with OTP2.1=1)
                                                                        //  3:  1.5 increments
                                                                        //  4:  2 increments ...
                                                                        //  8:  4 increments (default with OTP2.1=0) ...
                                                                        //  15: 7.5 increments (fastest regulation)
static constexpr uint32_t TMC2209_PWMCONF_PWM_REG_Msk           = 0xF << TMC2209_PWMCONF_PWM_REG_Pos; // min.: 0, max.: 25, default: 0
static constexpr uint32_t TMC2209_PWMCONF_PWM_LIM_Pos           = 28;   // Limit for PWM_SCALE_AUTO when switching back from spreadCycle to stealthChop.
                                                                        // This value defines the upper limit for bits 7 to 4 of the automatic
                                                                        // current control when switching back. It can be set to reduce the current jerk
                                                                        // during mode change back to stealthChop. It does not limit PWM_GRAD or PWM_GRAD_AUTO
                                                                        // offset. (Default = 12)
static constexpr uint32_t TMC2209_PWMCONF_PWM_LIM_Msk           = 0xF << TMC2209_PWMCONF_PWM_LIM_Pos; // min.: 0, max.: 15, default: 0

static constexpr uint32_t TMC2209_PWM_SCALE_SUM_Pos             = 0;    // Actual PWM duty cycle. This value is used for scaling the
                                                                        // values CUR_A and CUR_B read from the sine wave table.
static constexpr uint32_t TMC2209_PWM_SCALE_SUM_Msk             = 0xFF << TMC2209_PWM_SCALE_SUM_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_PWM_SCALE_AUTO_Pos            = 16;   // 9 Bit signed offset added to the calculated PWM duty cycle. This is the
                                                                        // result of the automatic amplitude regulation based on current measurement.
static constexpr uint32_t TMC2209_PWM_SCALE_AUTO_Msk            = 0x1FF << TMC2209_PWM_SCALE_AUTO_Pos; // min.: -255, max.: 255, default: 0

static constexpr uint32_t TMC2209_PWM_AUTO_PWM_OFS_AUTO_Pos     = 0;    // Automatically determined offset value
static constexpr uint32_t TMC2209_PWM_AUTO_PWM_OFS_AUTO_Msk     = 0xFF << TMC2209_PWM_AUTO_PWM_OFS_AUTO_Pos; // min.: 0, max.: 255, default: 0
static constexpr uint32_t TMC2209_PWM_AUTO_PWM_GRAD_AUTO_Pos    = 16;   // Automatically determined gradient value
static constexpr uint32_t TMC2209_PWM_AUTO_PWM_GRAD_AUTO_Msk    = 0xFF << TMC2209_PWM_AUTO_PWM_GRAD_AUTO_Pos; // min.: 0, max.: 255, default: 0


static constexpr uint32_t TMC2209_MICROSTEPS_256 = 0;
static constexpr uint32_t TMC2209_MICROSTEPS_128 = 1;
static constexpr uint32_t TMC2209_MICROSTEPS_64  = 2;
static constexpr uint32_t TMC2209_MICROSTEPS_32  = 3;
static constexpr uint32_t TMC2209_MICROSTEPS_16  = 4;
static constexpr uint32_t TMC2209_MICROSTEPS_8   = 5;
static constexpr uint32_t TMC2209_MICROSTEPS_4   = 6;
static constexpr uint32_t TMC2209_MICROSTEPS_2   = 7;
static constexpr uint32_t TMC2209_MICROSTEPS_1   = 8;
