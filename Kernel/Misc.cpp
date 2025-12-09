// This file is part of PadOS.
//
// Copyright (C) 2022-2025 Kurt Skauen <http://kavionic.com/>
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

#include <Kernel/Misc.h>
#include <Kernel/HAL/STM32/PiezoBuzzer_STM32.h>

namespace kernel
{

static PiezoBuzzer_STM32 g_BuzzerDriver;

bool setup_beeper(HWTimerID timerID, uint32_t timerClkFrequency, PinMuxTarget beeperPin)
{
    return g_BuzzerDriver.Setup(timerID, timerClkFrequency, beeperPin);
}

void kbeep_seconds(float duration)
{
    g_BuzzerDriver.Beep(duration);
}

} // namespace kernel

