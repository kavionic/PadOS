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
// Created: 14.05.2022 23:00

#pragma once

#include <stdint.h>
#include <Kernel/HAL/DigitalPort.h>

enum class HWTimerID : int32_t;

namespace os
{

enum class BeepLength : uint32_t
{
    Short,
    Medium,
    Long,
    VeryLong
};

bool setup_beeper(HWTimerID timerID, uint32_t timerClkFrequency, PinMuxTarget beeperPin);

void beep(BeepLength length);
void beep(float duration);


} // namespace os
