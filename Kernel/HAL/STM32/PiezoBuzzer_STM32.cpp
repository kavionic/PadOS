// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.05.2022 16:30


#include <algorithm>

#include <Kernel/HAL/PeripheralMapping.h>
#include <Kernel/HAL/STM32/PiezoBuzzer_STM32.h>
#include <Kernel/IRQDispatcher.h>
#include <Utils/Utils.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PiezoBuzzer_STM32::Setup(HWTimerID timerID, uint32_t timerClkFrequency, PinMuxTarget beeperPin)
{
    if (beeperPin.MUX == DigitalPinPeripheralID::None)
    {
        return false;
    }
    m_Timer = get_timer_from_id(timerID);
    IRQn_Type irq = get_timer_irq(timerID, HWTimerIRQType::Update);

    if (m_Timer == nullptr || irq == IRQ_COUNT)
    {
        return false;
    }

    m_IsInitialized = true;

    m_BeeperPinMux = beeperPin;
    m_BeeperPin = m_BeeperPinMux.PINID;

    m_BeeperPin.SetDriveStrength(DigitalPinDriveStrength_e::VeryHigh);
    m_BeeperPin.SetDirection(DigitalPinDirection_e::Out);
    m_BeeperPin.SetPeripheralMux(m_BeeperPinMux.MUX);

    const uint32_t prescale = 8;

    m_TimerFrequency = timerClkFrequency / prescale;

    m_Timer->CR1    = TIM_CR1_ARPE;
    m_Timer->CCMR1  = (6 << TIM_CCMR1_OC1M_Pos); // PWM-mode1
    m_Timer->CCER   = TIM_CCER_CC1E; // Enable compare 1 output
    m_Timer->BDTR   = TIM_BDTR_MOE;
    m_Timer->PSC    = prescale - 1;
    m_Timer->ARR    = m_TimerFrequency / m_BeepFrequency;
    m_Timer->CCR1   = (m_TimerFrequency / m_BeepFrequency) / 2;
    m_Timer->DIER  |= TIM_DIER_UIE;

    uint32_t dbgFlagMask = 0;
    volatile uint32_t* dbgReg = get_timer_dbg_clk_flag(timerID, dbgFlagMask);
    if (dbgReg != nullptr)
    {
        *dbgReg |= dbgFlagMask;
    }

    NVIC_ClearPendingIRQ(irq);
    register_irq_handler(irq, IRQCallback, this);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PiezoBuzzer_STM32::Beep(float time)
{
    if (m_IsInitialized)
    {
        m_Timer->CR1 &= ~TIM_CR1_CEN;

        m_RemainingCycles = uint32_t(time * float(m_BeepFrequency));
        if (m_RemainingCycles > 0)
        {
            uint32_t repetitions = std::min(256L, m_RemainingCycles);
            m_RemainingCycles -= repetitions;
            set_bit_group(m_Timer->RCR, TIM_RCR_REP_Msk, repetitions - 1);

            m_Timer->EGR = TIM_EGR_UG;
            m_Timer->SR &= ~TIM_SR_UIF;
            m_Timer->CR1 |= TIM_CR1_CEN;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult PiezoBuzzer_STM32::IRQCallback(IRQn_Type irq, void* userData)
{
    return static_cast<PiezoBuzzer_STM32*>(userData)->HandleIRQ();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IRQResult PiezoBuzzer_STM32::HandleIRQ()
{
    if (m_Timer->SR & TIM_SR_UIF)
    {
        m_Timer->SR &= ~TIM_SR_UIF;

        if (m_RemainingCycles > 0)
        {
            uint32_t repetitions = std::min(256L, m_RemainingCycles);
            set_bit_group(m_Timer->RCR, TIM_RCR_REP_Msk, repetitions - 1);
            m_RemainingCycles -= repetitions;
        }
        else
        {
            m_Timer->CR1 &= ~TIM_CR1_CEN;
        }
        return IRQResult::HANDLED;
    }
    return IRQResult::UNHANDLED;
}

} //namespace kernel
