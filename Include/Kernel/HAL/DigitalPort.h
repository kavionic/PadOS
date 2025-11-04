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

#include <stdint.h>

enum class DigitalPinDirection_e : uint32_t
{
    Analog,
    In,
    Out,
    OpenCollector
};

enum class PinPullMode_e : uint32_t
{
    None,
    Up,
    Down
};

enum class DigitalPinDriveStrength_e : uint32_t
{
    Low,
#if defined(STM32H7) || defined(STM32G0)
	Medium,
#endif
    High,
#if defined(STM32H7) || defined(STM32G0)
	VeryHigh
#endif
};

enum class PinInterruptMode_e
{
	  None
    , BothEdges
    , FallingEdge
    , RisingEdge
#if defined(__SAME70Q21__)
	, LowLevel,
    , HighLevel
#endif // defined(__SAME70Q21__)

};

#if defined(__SAME70Q21__)
#include "DigitalPort_ATSAM.h"
#elif defined(STM32H7) || defined(STM32G0)
#include "DigitalPort_STM32.h"
#else
#error Unknown platform
#endif
