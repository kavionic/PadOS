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

#include <Kernel/KIRQGuard.h>
#include <Kernel/Kernel.h>
#include <Kernel/KInterrupts.h>

namespace kernel
{

KIRQGuard::KIRQGuard(IRQn_Type irqNum) : m_IRQ(irqNum)
{
    kassert(irqNum >= 0 && irqNum < IRQ_COUNT);
    Lock();
}

KIRQGuard::~KIRQGuard()
{
    if (m_IsLocked) {
        Unlock();
    }
}

void KIRQGuard::Lock()
{
    kassert(!m_IsLocked);
    m_PreviousBasePriority = __get_BASEPRI();
    __set_BASEPRI_MAX(KIRQ_PRI_NORMAL_LATENCY_MAX << (8 - __NVIC_PRIO_BITS));
    __ISB();
    m_RestoreEnabled = NVIC_GetEnableIRQ(m_IRQ) != 0;
    NVIC_DisableIRQ(m_IRQ);
    m_IsLocked = true;
}

void KIRQGuard::Unlock()
{
    kassert(m_IsLocked);
    m_IsLocked = false;
    if (m_RestoreEnabled) {
        NVIC_EnableIRQ(m_IRQ);
    }
    restore_interrupts(m_PreviousBasePriority);
    __ISB(); // Allow a pending context switch before IRQWait() reacquires the guard.
}

void KIRQGuard::SetRestoreEnabled(bool enabled)
{
    kassert(m_IsLocked);
    m_RestoreEnabled = enabled;
}

} // namespace kernel
