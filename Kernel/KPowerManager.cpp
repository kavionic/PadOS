// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 18/08/25 0:24:37

#include "System/Platform.h"

#if defined(__SAME70Q21__)
#include "component/supc.h"
#elif defined(STM32H743xx)
#else
#error Unknown platform
#endif



#include "KPowerManager.h"
#include "Kernel.h"
//#include "SystemSetup.h"
#include "Drivers/RA8875Driver/GfxDriver.h"
#include "SDRAM.h"
#include "HAL/SAME70System.h"
#include "SAME70TimerDefines.h"

static int POWER_BUTTON_PRESSED_THRESHOLD; // = CLOCK_PERIF_FREQUENCY / 65536 / 10; // ~100mS
static int POWER_BUTTON_RESET_THRESHOLD; //   = 2 * CLOCK_PERIF_FREQUENCY / 65536; // ~2S

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPowerManager::KPowerManager() : Thread("sys_power_manager")
{
    POWER_BUTTON_PRESSED_THRESHOLD = kernel::Kernel::GetFrequencyPeripheral() / 65536 / 10; // ~100mS
    POWER_BUTTON_RESET_THRESHOLD   = 2 * kernel::Kernel::GetFrequencyPeripheral() / 65536; // ~2S
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPowerManager::~KPowerManager()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPowerManager& KPowerManager::GetInstance()
{
    static KPowerManager instance;
    return instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KPowerManager::Initialize(MCU_Timer16_t* timerChannel, const DigitalPin& pinPowerSwitch)
{
    m_TimerChannel = timerChannel;
    m_PinPowerSwitch = pinPowerSwitch;
#if defined(__SAME70Q21__)
    kernel::Kernel::RegisterIRQHandler(PIOA_IRQn, IRQCallback, this);
    kernel::Kernel::RegisterIRQHandler(TC4_IRQn, TimerIRQCallback, this);

    m_PinPowerSwitch.SetPullMode(PinPullMode_e::Up);
    m_PinPowerSwitch.SetInterruptMode(PinInterruptMode_e::BothEdges);
    m_PinPowerSwitch.GetInterruptStatus(); // Clear any pending interrupts.
    m_PinPowerSwitch.EnableInterrupts();
    
    m_TimerChannel->TC_EMR = TC_EMR_NODIVCLK_Msk; // Run at undivided peripheral clock.
    m_TimerChannel->TC_CMR = TC_CMR_WAVE_Msk | TC_CMR_WAVESEL_UP; // | TC_CMR_TCCLKS_TIMER_CLOCK4; // MCK/128
    m_TimerChannel->TC_IER = TC_IER_COVFS_Msk;

//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_POWER_SW_TIMER].TC_CCR = TC_CCR_CLKEN_Msk;
//    SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_POWER_SW_TIMER].TC_CCR = TC_CCR_SWTRG_Msk;
#elif defined(STM32H743xx)
#else
#error Unknown platform
#endif

    SetState(sys_power_state::running);
    Start(false);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KPowerManager::Shutdown()
{
//    kernel::GfxDriver::Instance.Shutdown();
//    ShutdownSDRAM();
	snooze_ms(100);
//    QSPI_RESET_Pin   = false;
//    WIFI_RESET_Pin   = false;

#if defined(__SAME70Q21__)
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;
    SUPC->SUPC_CR = SUPC_CR_KEY_PASSWD | SUPC_CR_VROFF_Msk;
    __WFE();
#elif defined(STM32H743xx)
#else
#error Unknown platform
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KPowerManager::SetState(sys_power_state newState)
{
    if (newState != m_State)
    {
//        sys_power_state oldState = m_State;
        m_State = newState;

//        RGBLED_B = m_State == sys_power_state::shutdown;
        
        if (m_State == sys_power_state::shutdown)
        {
            Shutdown();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KPowerManager::Run()
{
    for (;;)
    {
/*        if (m_PinPowerSwitch) {
            RGBLED_B.Write(false);            
        } else {
            RGBLED_B.Write(true);            
        }            */
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#if defined(__SAME70Q21__)
kernel::IRQResult KPowerManager::HandleIRQ()
{
    if (m_PinPowerSwitch.GetInterruptStatus())
    {
//        bool buttonState = !m_PinPowerSwitch;
        
        if (!m_PinPowerSwitch)
        {
            m_PowerButtonPressedCycles = 0;
            m_TimerChannel->TC_CCR = TC_CCR_CLKEN_Msk;
            m_TimerChannel->TC_CCR = TC_CCR_SWTRG_Msk;
        }
        else
        {
            m_TimerChannel->TC_CCR = TC_CCR_CLKDIS_Msk;
//            RGBLED_G = false;
            if (m_PowerButtonPressedCycles >= POWER_BUTTON_PRESSED_THRESHOLD)
            {
                if (m_State == sys_power_state::running) {
                    SetState(sys_power_state::shutdown);
                } else {
                    SetState(sys_power_state::running);
                }
            }
            m_PowerButtonPressedCycles = 0;
        }
        
/*        SpinTimer::SleepMS(5);
        
        uint32_t delay = 2000000 * (CLOCK_PERIF_FREQUENCY / 1000000);
        uint16_t startTime = SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CV;
        for (;;)
        {
            uint16_t curTime = SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CV;
            
            if (delay > 3000) {
                while(int16_t(SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CV - startTime) < 3000);
                delay -= 3000;
                startTime += 3000;
            } else {
                while(int16_t(SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CV - startTime) < delay);
                break;
            }
        }        
        
        RGBLED_R.Write(false);
        RGBLED_G.Write(false);
        if (m_PinPowerSwitch) {
            RGBLED_B.Write(false);            
        } else {
            RGBLED_B.Write(true);            
        }*/
		return kernel::IRQResult::HANDLED;
    }
	return kernel::IRQResult::UNHANDLED;
}

IRQResult KPowerManager::HandleTimerIRQ()
{
    if (m_TimerChannel->TC_SR & TC_SR_COVFS_Msk)
    {
        m_PowerButtonPressedCycles++;
        if (m_PowerButtonPressedCycles >= POWER_BUTTON_RESET_THRESHOLD) {
//            RGBLED_G = true;
            RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST_Msk;
        }
    }    
	return IRQResult::HANDLED;
}

#elif defined(STM32H743xx)
#else
#error Unknown platform
#endif
