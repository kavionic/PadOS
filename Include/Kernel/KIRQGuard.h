// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <System/Platform.h>

#if !defined(STM32H7)
#error KIRQGuard must be implemented for this platform.
#endif

namespace kernel
{

// Masks one external IRQ and holds the same BASEPRI threshold as KSchedulerLock.
// Normal-latency IRQs, deferred callbacks, and thread scheduling are postponed until the mask is released.
// Other low-latency IRQs can still run. An already stronger BASEPRI mask is preserved.
// Nested guards must be released in reverse order. Do not block while holding the guard except through IRQWait().
// The guard does not clear pending hardware interrupts or discard deferred work.
class KIRQGuard
{
public:
    explicit KIRQGuard(IRQn_Type irqNum);
    ~KIRQGuard();

    void Lock();
    void Unlock();

    // Override the saved NVIC enable state, for IRQ registration and removal.
    void SetRestoreEnabled(bool enabled);

    bool IsLocked() const { return m_IsLocked; }
    bool CanWait() const { return m_IsLocked && m_RestoreEnabled && m_PreviousBasePriority == 0; }

    KIRQGuard(const KIRQGuard&) = delete;
    KIRQGuard& operator=(const KIRQGuard&) = delete;

private:
    const IRQn_Type m_IRQ;
    bool m_IsLocked = false;
    bool m_RestoreEnabled = false;
    uint32_t m_PreviousBasePriority = 0;
};

} // namespace kernel
