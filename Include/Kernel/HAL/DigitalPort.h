// This file is part of PadOS.
//
// Copyright (c) 2016-2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
