// This file is part of PadOS.
//
// Copyright (C) 2016-2018 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include "sam.h"

//#include "SystemSetup.h"


namespace kernel
{


class SpinTimer
{
public:
    static void Initialize(TcChannel* timerChannel); // SYSTEM_TIMER->TcChannel[SYSTEM_TIMER_SPIN_TIMER_L]

    static void SleepuS(int32_t delay)
    {
        delay = delay * s_TicksPerMicroSec;
        uint16_t startTime = s_TimerChannel->TC_CV;
        for (;;)
        {
            if (delay > 3000) {
                while(int16_t(s_TimerChannel->TC_CV - startTime) < 3000);
                delay -= 3000;
                startTime += 3000;
            } else {
                while(int16_t(s_TimerChannel->TC_CV - startTime) < delay);
                break;
            }
        }        
    }

    static void SleepMS(uint32_t delay)
    {
        SleepuS(delay * 1000);
    }
private:
    static uint32_t   s_TicksPerMicroSec;
    static TcChannel* s_TimerChannel;
};

} // namespace
