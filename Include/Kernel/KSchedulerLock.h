// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 06.01.2026 21:00

#pragma once

#include <Kernel/KInterrupts.h>

namespace kernel
{

class KSchedulerLock
{
public:
    KSchedulerLock() { m_PrevState = disable_interrupts(); }
    ~KSchedulerLock() { restore_interrupts(m_PrevState); }

    static bool IsLocked() { return get_interrupt_enabled_state() != IRQEnableState::Enabled; }

private:
    uint32_t    m_PrevState;

    KSchedulerLock(const KSchedulerLock&) = delete;
    KSchedulerLock& operator=(const KSchedulerLock&) = delete;
};

} // namespace
