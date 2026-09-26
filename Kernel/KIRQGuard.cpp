// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

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
