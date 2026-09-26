// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.07.2020 23:40

#pragma once

#include "System/Platform.h"
#include <System/TimeValue.h>
#include <Kernel/KIRQPriorityLevels.h>

namespace kernel
{

enum class IRQResult : int
{
    UNHANDLED,
    HANDLED,
    HANDLED_DEFERRED
};

typedef IRQResult KIRQHandler(IRQn_Type irq, void* userData);
typedef void KIRQDeferredHandler(IRQn_Type irq, void* userData);

// Registration and removal require thread context.
// A shared IRQ uses the highest requested priority (lowest numerical value), recomputed on removal.
// All handlers sharing an IRQ must belong to the same latency class; mixing classes fails with EBUSY.
int register_irq_handler(IRQn_Type irqNum, KIRQHandler* handler, void* userData);

// HANDLED_DEFERRED requests a coalesced callback at the start of PendSV, under KSchedulerLock.
// The immediate handler must not call scheduler APIs at a low-latency priority.
// The deferred handler may wake threads, but must not block or register/unregister handlers.
// Driver-owned state must retain all work until consumed; callback invocations do not count IRQ events.
// The deferred handler can be interrupted by the immediate handler, so shared driver state still needs protection.
// KIRQGuard masks the source IRQ and holds the scheduler interrupt mask, postponing all deferred callbacks.
int register_irq_handler(
    IRQn_Type irqNum,
    KIRQHandler* handler,
    KIRQDeferredHandler* deferredHandler,
    void* userData,
    KIRQPriorityLevels priority);

// On return neither callback can run again for this registration; any queued notification is discarded.
int unregister_irq_handler(IRQn_Type irqNum, int handle);

// Scheduler entry point. Call only from PendSV with KSchedulerLock held.
void dispatch_deferred_irq_handlers();

// Counts dispatched device IRQs and deferred callbacks, with nested execution counted once.
// Requires a context allowed to acquire KSchedulerLock.
TimeValNanos kget_total_irq_time();

} // namespace

