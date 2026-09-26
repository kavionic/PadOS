// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
