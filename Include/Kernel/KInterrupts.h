// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.01.2026 21:00

#pragma once

#include <Kernel/KIRQPriorityLevels.h>

namespace kernel
{


enum class IRQEnableState
{
    Enabled,
#if defined(STM32H7)
    NormalLatencyDisabled,
    LowLatencyDisabled,
#elif defined(STM32G0)
    Disabled
#else
#error Unknown platform.
#endif
};


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline IRQEnableState get_interrupt_enabled_state()
{
#if defined(STM32H7)
    uint32_t basePri = __get_BASEPRI();
    if (basePri == 0) {
        return IRQEnableState::Enabled;
    }
    else if (basePri >= (KIRQ_PRI_NORMAL_LATENCY_MAX << (8 - __NVIC_PRIO_BITS))) {
        return IRQEnableState::NormalLatencyDisabled;
    }
    else {
        return IRQEnableState::LowLatencyDisabled;
    }
#elif defined(STM32G030xx)
    return (__get_PRIMASK() & 0x01) ? IRQEnableState::Disabled : IRQEnableState::Enabled;
#else
#error Unknown platform.
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline void set_interrupt_enabled_state(IRQEnableState state)
{
#if defined(STM32H7)
    switch (state)
    {
        case IRQEnableState::Enabled:               __set_BASEPRI(0); break;
        case IRQEnableState::NormalLatencyDisabled: __set_BASEPRI(KIRQ_PRI_NORMAL_LATENCY_MAX << (8 - __NVIC_PRIO_BITS)); break;
        case IRQEnableState::LowLatencyDisabled:    __set_BASEPRI(KIRQ_PRI_LOW_LATENCY_MAX << (8 - __NVIC_PRIO_BITS)); break;
    }
#elif defined(STM32G030xx)
    __set_PRIMASK((state == IRQEnableState::Enabled) ? 0 : 1);
#else
#error Unknown platform.
#endif
    __ISB();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline uint32_t disable_interrupts()
{
#if defined(STM32H7)
    const uint32_t oldState = __get_BASEPRI();
    __set_BASEPRI(KIRQ_PRI_NORMAL_LATENCY_MAX << (8 - __NVIC_PRIO_BITS));
    assert(__get_BASEPRI() == (KIRQ_PRI_NORMAL_LATENCY_MAX << (8 - __NVIC_PRIO_BITS)));
#elif defined(STM32G030xx)
    const uint32_t oldState = __get_PRIMASK();
    __disable_irq();
#else
#error Unknown platform.
#endif
    __ISB();
    return oldState;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#if defined(STM32H7)
inline uint32_t KDisableLowLatenctInterrupts()
{
    const uint32_t oldState = __get_BASEPRI();
    __set_BASEPRI(KIRQ_PRI_LOW_LATENCY_MAX << (8 - __NVIC_PRIO_BITS));
    __ISB();
    assert(__get_BASEPRI() == (KIRQ_PRI_LOW_LATENCY_MAX << (8 - __NVIC_PRIO_BITS)));
    return oldState;
}
#endif // defined(STM32H7)

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

inline void restore_interrupts(uint32_t state)
{
#if defined(STM32H7)
    __set_BASEPRI(state);
    assert(__get_BASEPRI() == state);
#elif defined(STM32G030xx)
    if ((state & 0x01) == 0)
    {
        __enable_irq();
    }
#else
#error Unknown platform.
#endif
}


} // namespace
