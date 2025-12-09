// This file is part of PadOS.
//
// Copyright (C) 2021-2025 Kurt Skauen <http://kavionic.com/>
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

#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/Drivers/MultiMotorController/StepperDriver.h>
#include <Kernel/Drivers/MultiMotorController/TMC2209Registers.h>

struct TMC2209IOSetup;

namespace kernel
{

class TMC2209IODriver;


enum class TMCC2209_VRefSource : uint8_t
{
    Internal,
    External
};

enum class TMCC2209_SensMethod : uint8_t
{
    ExternalRSens,
    InternalRSens
};

enum class TMCC2209_PWMMethod : uint8_t
{
    StealthChop,
    SpreadCycle
};

enum class TMCC2209_Direction : uint8_t
{
    Forward,
    Reverse
};

enum class TMCC2209_IndexOutput : uint8_t
{
    FullStep,
    MicroStep,
    OvertempPreWarning
};

enum class TMCC2209_PDNUARTMode : uint8_t
{
    Powerdown,
    UART
};

enum class TMCC2209_MSTEPSelect : uint8_t
{
    MSxPins,
    MSTEPReg
};

enum class TMCC2209_MultiStepFilter : uint8_t
{
    Off,
    On
};

struct TMC2209Config
{
    TMCC2209_VRefSource         VRefSource = TMCC2209_VRefSource::Internal;
    TMCC2209_SensMethod         SensMethod = TMCC2209_SensMethod::ExternalRSens;
    TMCC2209_PWMMethod          PWMMethod = TMCC2209_PWMMethod::StealthChop;
    TMCC2209_Direction          Direction = TMCC2209_Direction::Forward;
    TMCC2209_IndexOutput        IndexOutput = TMCC2209_IndexOutput::FullStep;
    TMCC2209_PDNUARTMode        PDNUARTMode = TMCC2209_PDNUARTMode::Powerdown;
    TMCC2209_MSTEPSelect        MSTEPSelect = TMCC2209_MSTEPSelect::MSxPins;
    TMCC2209_MultiStepFilter    MultiStepFilter = TMCC2209_MultiStepFilter::On;
};

enum class TMCC2209_CoolStep_MinimumCurrent : uint8_t
{
    HalfIRUN,
    QuarterIRUN
};

enum class TMCC2209_CoolStep_StepDownSpeed : uint8_t
{
    Speed32 = 0, // For each 32 StallGuard4 values decrease by one
    Speed8  = 1, // For each 8 StallGuard4 values decrease by one
    Speed2  = 2, // For each 2 StallGuard4 values decrease by one
    Speed1  = 3  // For each StallGuard4 values decrease by one
};


class TMC2209Driver : public StepperDriver
{
public:
    TMC2209Driver();
    virtual ~TMC2209Driver() override;

    void Setup_trw(
        Ptr<TMC2209IODriver>    controlPort,
        DigitalPinID            diagPinID,
        uint32_t                chipAddress,
        HWTimerID               timerID,
        PinMuxTarget            pinStep,
        DigitalPinID            pinEnable,
        DigitalPinID            pinDirection
    );


    void Setup_trw(Ptr<TMC2209IODriver> controlPort, const TMC2209IOSetup& setup);

    void        SetTMCClock(uint32_t frequency) { m_TMCClock = frequency; }
    uint32_t    GetTMCClock() const { return m_TMCClock; }

    void    SetConfig_trw(const TMC2209Config& config);

    void    SetCurrent_trw(float holdCurrent, float runCurrent, TimeValMillis fadeTime);

    float   GetRunCurrent() const { return m_RunCurrent; }
    void    SetRunCurrent_trw(float current) { return SetCurrent_trw(m_HoldCurrent, current, m_CurrentFadeTime); }

    void    SetHoldCurrent_trw(float current) { return SetCurrent_trw(current, m_RunCurrent, m_CurrentFadeTime); }
    float   GetHoldCurrent() const { return m_HoldCurrent; }


    TimeValMillis   GetCurrentFadeTime() const { return m_CurrentFadeTime; }
    void            SetCurrentFadeTime_trw(TimeValMillis fadeTime) { return SetCurrent_trw(m_RunCurrent, m_HoldCurrent, fadeTime); }

    void SetPowerDownTime_trw(TimeValMillis time);
    void SetMicrosteps_trw(int32_t steps);

    uint32_t    GetStepTicks_trw() const; // Time per microstep in GetTMCClock() cycles.
    float       GetStepTime_trw() const; // Time per microstep in seconds.
    void        SetMaxStealthChopSpeed_trw(float maxSpeed);
    void        SetMinStallGuardSpeed_trw(float minSpeed);

    void        SetStallGuardThreshold_trw(float threshold);
    float       GetStallGuardResult_trw() const;


    void SetHaltOnStall(bool doHalt);
    void ClearHaltedFlag() { m_HasHalted = false; }
    bool HasHalted() const { return m_HasHalted; }
private:
    uint32_t    ReadRegister_trw(uint8_t registerAddress) const;
    void        WriteRegister_trw(uint8_t registerAddress, uint32_t data) const;

    static IRQResult MotorStallIRQCallback(IRQn_Type irq, void* userData) { return static_cast<TMC2209Driver*>(userData)->HandleMotorStallIRQ(); }
    IRQResult HandleMotorStallIRQ();

    Ptr<TMC2209IODriver>    m_ControlPort;
    DigitalPin              m_DiagPin;
    uint8_t                 m_ChipAddress;
    mutable uint8_t         m_CurrentUpdateCount = 0;
    bool                    m_HaltOnStall = false;
    volatile bool           m_HasHalted = false;
    float                   m_RSens = 100.0e-3f;
    uint32_t                m_TMCClock = 12000000;
    int                     m_MicroStepsPerStep = 256;
    uint32_t                m_GCONF = 0;
    uint32_t                m_CHOPCONF = (3 << TMC2209_CHOPCONF_TOFF_Pos) | (5 << TMC2209_CHOPCONF_HSTRT_Pos) | TMC2209_CHOPCONF_INTPOL;
    float                   m_RunCurrent = 0.0f;
    float                   m_HoldCurrent = 0.0f;
    TimeValMillis           m_CurrentFadeTime;
};

} // namespace kernel
