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
