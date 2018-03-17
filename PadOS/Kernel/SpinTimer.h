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

#include "SystemSetup.h"

namespace kernel
{


class SpinTimer
{
public:
    static void Initialize();

    static void SleepuS(int32_t delay)
    {
        delay = delay * (CLOCK_PERIF_FREQUENCY / 1000000);
        uint16_t startTime = SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CV;
        for (;;)
        {
            if (delay > 3000) {
                while(int16_t(SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CV - startTime) < 3000);
                delay -= 3000;
                startTime += 3000;
            } else {
                while(int16_t(SYSTEM_TIMER->TC_CHANNEL[SYSTEM_TIMER_SPIN_TIMER_L].TC_CV - startTime) < delay);
                break;
            }
        }        
    }

    static void SleepMS(uint32_t delay)
    {
        SleepuS(delay * 1000);
    }
};

} // namespace
