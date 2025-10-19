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

#pragma once
#include <stdint.h>
#include <Kernel/HAL/DigitalPort.h>

namespace kernel
{

enum class HWTimerID : int32_t;
enum class IRQResult : int;

class PiezoBuzzer_STM32
{
public:
    bool Setup(HWTimerID timerID, uint32_t timerClkFrequency, PinMuxTarget beeperPin);

    void Beep(float time);

private:
    static IRQResult IRQCallback(IRQn_Type irq, void* userData);
    IRQResult HandleIRQ();

    MCU_Timer16_t* m_Timer = nullptr;
    DigitalPin      m_BeeperPin;
    PinMuxTarget    m_BeeperPinMux;
    uint32_t        m_TimerFrequency = 0;
    uint32_t        m_BeepFrequency = 2000;
    int32_t         m_RemainingCycles = 0;
    bool            m_IsInitialized = false;
};

} //namespace kernel
