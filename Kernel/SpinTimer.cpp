// This file is part of PadOS.
//
// Copyright (c) 2016-2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include "Kernel/SpinTimer.h"
#include <Kernel/HAL/STM32/ResetAndClockControl.h>
#include <System/TimeValue.h>

namespace kernel
{

uint32_t   SpinTimer::s_TicksPerMicroSec;

void SpinTimer::Initialize()
{
    s_TicksPerMicroSec = ResetAndClockControl::GetSysClockFrequency() / TimeValMicros::TicksPerSecond;
}

void SpinTimer::SleepuS(uint32_t delay)
{
    int32_t delayCycles = delay * s_TicksPerMicroSec;

    uint32_t prev = SysTick->VAL;
    uint32_t range = SysTick->LOAD;

    while (delayCycles > 0)
    {
        uint32_t current = SysTick->VAL;
        if (current <= prev) {
            delayCycles -= prev - current;
        }
        else {
            delayCycles -= prev + (range - current);
        }
        prev = current;
    }
}

void SpinTimer::SleepMS(uint32_t delay)
{
    SleepuS(delay * 1000);
}

} // namespace kernel
