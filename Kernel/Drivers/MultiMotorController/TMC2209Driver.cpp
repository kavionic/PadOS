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

#include <algorithm>

#include <System/ExceptionHandling.h>
#include <DeviceControl/MultiMotor.h>

#include <Kernel/KLogging.h>
#include <Kernel/HAL/STM32/Peripherals_STM32H7.h>
#include <Kernel/Drivers/MultiMotorController/TMC2209Driver.h>
#include <Kernel/Drivers/MultiMotorController/TMC2209IODriver.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TMC2209Driver::TMC2209Driver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TMC2209Driver::~TMC2209Driver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::Setup_trw(
    Ptr<TMC2209IODriver>    controlPort,
    DigitalPinID            diagPinID,
    uint32_t                chipAddress,
    HWTimerID               timerID,
    PinMuxTarget            pinStep,
    DigitalPinID            pinEnable,
    DigitalPinID            pinDirection
)
{
    if (controlPort == nullptr)
    {
        p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "TMC2209Driver::Setup(): missing control port.");
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    StepperDriver::Setup_trw(timerID, pinStep, pinEnable, pinDirection);

    m_ControlPort   = controlPort;
    m_DiagPin       = diagPinID;
    m_ChipAddress   = uint8_t(chipAddress);

    m_GCONF = ReadRegister_trw(TMC2209_REG_GCONF);
    const uint32_t prevCnt = ReadRegister_trw(TMC2209_REG_IFCNT);
    m_CurrentUpdateCount = uint8_t(prevCnt);

    const uint32_t curIO = ReadRegister_trw(TMC2209_REG_IOIN);
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "TMC2209Driver::Setup(): IO{}={:02x}", chipAddress, curIO);

    SetCurrent_trw(0.5f, 1.2f, 1.0);
    SetPowerDownTime_trw(2.0);
    SetMicrosteps_trw(TMC2209_MICROSTEPS_16);

    WriteRegister_trw(TMC2209_REG_GCONF, /*TMC2209_I_SCALE_ANALOG |*/ TMC2209_GCONF_PDN_DISABLE | TMC2209_GCONF_MSTEP_REG_SELECT | TMC2209_GCONF_MULTISTEP_FILT);
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "{}: Wrote config.", __PRETTY_FUNCTION__);

    if (diagPinID != DigitalPinID::None)
    {
        m_DiagPin.SetDirection(DigitalPinDirection_e::In);
        m_DiagPin.SetInterruptMode(PinInterruptMode_e::RisingEdge);
        register_irq_handler(get_peripheral_irq(diagPinID), MotorStallIRQCallback, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::Setup_trw(Ptr<TMC2209IODriver> controlPort, const TMC2209IOSetup& setup)
{
    Setup_trw(controlPort, setup.DiagPinID, setup.ChipAddress, setup.TimerID, setup.StepPin, setup.EnablePin, setup.DirectionPin);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetConfig_trw(const TMC2209Config& config)
{
    uint32_t value = 0;

    if (config.VRefSource       == TMCC2209_VRefSource::External)       value |= TMC2209_GCONF_I_SCALE_ANALOG;
    if (config.SensMethod       == TMCC2209_SensMethod::InternalRSens)  value |= TMC2209_GCONF_INTERNAL_RSENSE;
    if (config.PWMMethod        == TMCC2209_PWMMethod::SpreadCycle)     value |= TMC2209_GCONF_EN_SPREADCYCLE;
    if (config.Direction        == TMCC2209_Direction::Reverse)         value |= TMC2209_GCONF_SHAFT;
    if (config.PDNUARTMode      == TMCC2209_PDNUARTMode::UART)          value |= TMC2209_GCONF_PDN_DISABLE;
    if (config.MSTEPSelect      == TMCC2209_MSTEPSelect::MSTEPReg)      value |= TMC2209_GCONF_MSTEP_REG_SELECT;
    if (config.MultiStepFilter  == TMCC2209_MultiStepFilter::On)        value |= TMC2209_GCONF_MULTISTEP_FILT;

    switch (config.IndexOutput)
    {
        case TMCC2209_IndexOutput::FullStep:
            break;
        case TMCC2209_IndexOutput::MicroStep:
            value |= TMC2209_GCONF_INDEX_STEP;
            break;
        case TMCC2209_IndexOutput::OvertempPreWarning:
            value |= TMC2209_GCONF_INDEX_OTPW;
            break;
    }
    WriteRegister_trw(TMC2209_REG_GCONF, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetCurrent_trw(float holdCurrent, float runCurrent, TimeValMillis fadeTime)
{
    m_HoldCurrent       = holdCurrent;
    m_RunCurrent        = runCurrent;
    m_CurrentFadeTime   = fadeTime;

    const float maxCurrent = 325.0e-3f / (m_RSens + 20.0e-3f) * 0.7071067811865475244f; // 325.0e-3 / (m_RSens + 20e-3) * (1/sqrt(2))
    const int32_t holdScale = int32_t(32.0f * holdCurrent / maxCurrent) - 1;
    const int32_t runScale = int32_t(32.0f * runCurrent / maxCurrent) - 1;
    const int32_t holdTicks = (runScale > holdScale) ? uint32_t(double(m_TMCClock) * fadeTime.AsSeconds() / double((runScale - holdScale) * (2 << 18))) : 0;

    const uint32_t value = (std::clamp<int32_t>(holdTicks, 0, 15) << 16) | (std::clamp<int32_t>(runScale, 0, 31) << 8) | std::clamp<int32_t>(holdScale, 0, 31);

    WriteRegister_trw(TMC2209_REG_IHOLD_IRUN, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetPowerDownTime_trw(TimeValMillis time)
{
    int32_t ticks = int32_t(double(m_TMCClock) * time.AsSeconds() / double(2 << 18));
    WriteRegister_trw(TMC2209_REG_TPOWERDOWN, std::clamp<int32_t>(ticks, 2, 255));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetMicrosteps_trw(int32_t steps)
{
    int32_t stepMode;

    if (steps <= 1) {
        stepMode = TMC2209_MICROSTEPS_1;
    } else if (steps <= 2) {
        stepMode = TMC2209_MICROSTEPS_2;
    } else if (steps <= 4) {
        stepMode = TMC2209_MICROSTEPS_4;
    } else if (steps <= 8) {
        stepMode = TMC2209_MICROSTEPS_8;
    } else if (steps <= 16) {
        stepMode = TMC2209_MICROSTEPS_16;
    } else if (steps <= 32) {
        stepMode = TMC2209_MICROSTEPS_32;
    } else if (steps <= 64) {
        stepMode = TMC2209_MICROSTEPS_64;
    } else if (steps <= 128) {
        stepMode = TMC2209_MICROSTEPS_128;
    } else {
        stepMode = TMC2209_MICROSTEPS_256;
}    

    uint32_t CHOPCONF = (m_CHOPCONF & ~TMC2209_CHOPCONF_MRES_Msk) | ((stepMode << TMC2209_CHOPCONF_MRES_Pos) & TMC2209_CHOPCONF_MRES_Msk);
    WriteRegister_trw(TMC2209_REG_CHOPCONF, CHOPCONF);

    m_CHOPCONF = CHOPCONF;
    m_MicroStepsPerStep = 1 << stepMode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t TMC2209Driver::GetStepTicks_trw() const
{
    return ReadRegister_trw(TMC2209_REG_TSTEP);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float TMC2209Driver::GetStepTime_trw() const
{
    const uint32_t tics = GetStepTicks_trw();
    return float(tics) / float(m_TMCClock);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetMaxStealthChopSpeed_trw(float maxSpeed)
{
    uint32_t ticks = 0;
    if (maxSpeed > 1.0)
    {
        static constexpr uint32_t maxTicks = (1 << 20) - 1;
        ticks = std::min(uint32_t(float(m_TMCClock) / (maxSpeed * GetStepsPerMillimeter() * float(m_MicroStepsPerStep))), maxTicks);
    }
    WriteRegister_trw(TMC2209_REG_TPWMTHRS, ticks);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetMinStallGuardSpeed_trw(float minSpeed)
{
    uint32_t ticks = 0;
    if (minSpeed > 1.0)
    {
        static constexpr uint32_t maxTicks = (1 << 20) - 1;
        ticks = std::min(uint32_t(float(m_TMCClock) / (minSpeed * GetStepsPerMillimeter() * float(m_MicroStepsPerStep))), maxTicks);
    }
    WriteRegister_trw(TMC2209_REG_TCOOLTHRS, ticks);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetStallGuardThreshold_trw(float threshold)
{
    uint32_t value = std::min<uint32_t>(255, uint32_t(threshold * 255.0f));

    WriteRegister_trw(TMC2209_REG_SGTHRS, value);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float TMC2209Driver::GetStallGuardResult_trw() const
{
    const uint32_t value = ReadRegister_trw(TMC2209_REG_SG_RESULT);
    return float(value) * (1.0f / 511.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::SetHaltOnStall(bool doHalt)
{
    if (doHalt != m_HaltOnStall)
    {
        m_HaltOnStall = doHalt;
        if (m_DiagPin.GetID() != DigitalPinID::None)
        {
            if (m_HaltOnStall) {
                m_DiagPin.GetAndClearInterruptStatus();
                m_DiagPin.EnableInterrupts();
            } else {
                m_DiagPin.DisableInterrupts();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t TMC2209Driver::ReadRegister_trw(uint8_t registerAddress) const
{
    if (!IsInitialized()) PERROR_THROW_CODE(PErrorCode::InvalidArg);
    
    for (int i = 0; i < 8; ++i)
    {
        try
        {
            return m_ControlPort->ReadRegister(m_ChipAddress, registerAddress);
        }
        PERROR_CATCH([](PErrorCode) {});
    }
    p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "TMC2209Driver::ReadRegister({}, {}) To many retries.", m_ChipAddress, registerAddress);
    PERROR_THROW_CODE(PErrorCode::IOError);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TMC2209Driver::WriteRegister_trw(uint8_t registerAddress, uint32_t data) const
{
    if (!IsInitialized()) PERROR_THROW_CODE(PErrorCode::InvalidArg);

    for (int i = 0; i < 5; ++i)
    {
        try
        {
            m_CurrentUpdateCount++;
            m_ControlPort->WriteRegister(m_ChipAddress, registerAddress, data);

            const uint32_t updateCount = ReadRegister_trw(TMC2209_REG_IFCNT);
            if (m_CurrentUpdateCount == uint8_t(updateCount)) {
                return;
            }
            p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "TMC2209Driver::WriteRegister({}, {}) failed to update register ({} != {}). Retrying...", m_ChipAddress, registerAddress, uint8_t(updateCount), m_CurrentUpdateCount);
            m_CurrentUpdateCount = uint8_t(updateCount);
        }
        PERROR_CATCH([](PErrorCode) {});
    }
    p_system_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "TMC2209Driver::WriteRegister({}, {}) failed to update register. To many retries.", m_ChipAddress, registerAddress);
    PERROR_THROW_CODE(PErrorCode::IOError);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult TMC2209Driver::HandleMotorStallIRQ()
{
    if (m_DiagPin.GetAndClearInterruptStatus())
    {
        m_HasHalted = true;
        StartStopTimer(false);
        ClearMotion();
        return IRQResult::HANDLED;
    }
    return IRQResult::UNHANDLED;
}

} // namespace kernel
