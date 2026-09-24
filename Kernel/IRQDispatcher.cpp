// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 20.07.2020 23:40

#include <sys/errno.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <iterator>
#include <memory>
#include <new>
#include <utility>

#include <Kernel/IRQDispatcher.h>
#include <Kernel/KIRQGuard.h>
#ifdef PADOS_GPROF_HW_TIMER
#include <Kernel/Profiler/KGProfSampler.h>
#endif // PADOS_GPROF_HW_TIMER
#include <Kernel/Scheduler.h>
#include <Kernel/Syscalls.h>
#include <System/Platform.h>
#include <System/System.h>
#include <Threads/Threads.h>

#if !defined(STM32H7)
#error IRQ cycle accounting must be implemented for this platform.
#endif

namespace kernel
{

struct KIRQAction
{
    KIRQHandler*            Handler = nullptr;
    KIRQDeferredHandler*    DeferredHandler = nullptr;
    void*                   UserData = nullptr;
    int32_t                 Handle = 0;
    KIRQPriorityLevels      Priority = KIRQ_PRI_NORMAL_LATENCY2;
    KIRQAction*             Next = nullptr;
    std::atomic<uint32_t>   DeferredPending{0};
};

static KIRQAction*              gk_IRQHandlers[IRQ_COUNT];
static std::atomic<uint32_t>    gk_DeferredIRQPendingFlags[(IRQ_COUNT + 31) / 32];
static TimeValNanos             gk_TotalIRQTime;

static_assert(std::atomic<uint32_t>::is_always_lock_free);
static std::atomic<uint32_t>    gk_TotalIRQCycles{0};
static uint32_t                 gk_PreviousIRQCycles = 0;
static double                   gk_IRQNanosecondRemainder = 0.0;

static KIRQPriorityLevels get_irq_priority(const KIRQAction* firstAction, const KIRQAction* excludedAction = nullptr)
{
    KIRQPriorityLevels priority = KIRQ_PRI_KERNEL;
    for (const KIRQAction* action = firstAction; action != nullptr; action = action->Next)
    {
        if (action != excludedAction) {
            priority = std::min(priority, action->Priority);
        }
    }
    return priority;
}

class KIRQTimeScope
{
public:
    KIRQTimeScope();
    ~KIRQTimeScope();

    KIRQTimeScope(const KIRQTimeScope&) = delete;
    KIRQTimeScope& operator=(const KIRQTimeScope&) = delete;

private:
    uint32_t m_StartCycles;
    uint32_t m_PreviousIRQCycles;
};

KIRQTimeScope::KIRQTimeScope()
{
    // Retry only if a nested handler completed between the counter and clock samples.
    for (;;)
    {
        m_PreviousIRQCycles = gk_TotalIRQCycles.load(std::memory_order_relaxed);
        std::atomic_signal_fence(std::memory_order_seq_cst);
        m_StartCycles = DWT->CYCCNT;
        std::atomic_signal_fence(std::memory_order_seq_cst);
        if (gk_TotalIRQCycles.load(std::memory_order_relaxed) == m_PreviousIRQCycles) {
            break;
        }
    }
}

KIRQTimeScope::~KIRQTimeScope()
{
    uint32_t previousCycles = gk_TotalIRQCycles.load(std::memory_order_relaxed);
    for (;;)
    {
        // Replace the nested handlers' contributions with the inclusive duration of this scope.
        // A nested completion after the timestamp invalidates the CAS and requires a fresh timestamp.
        std::atomic_signal_fence(std::memory_order_seq_cst);
        const uint32_t elapsedCycles = DWT->CYCCNT - m_StartCycles;
        std::atomic_signal_fence(std::memory_order_seq_cst);
        if (gk_TotalIRQCycles.compare_exchange_weak(
            previousCycles,
            m_PreviousIRQCycles + elapsedCycles,
            std::memory_order_relaxed,
            std::memory_order_relaxed)) {
            break;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int register_irq_handler(IRQn_Type irqNum, KIRQHandler* handler, void* userData)
{
    return register_irq_handler(irqNum, handler, nullptr, userData, KIRQ_PRI_NORMAL_LATENCY2);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int register_irq_handler(
    IRQn_Type irqNum,
    KIRQHandler* handler,
    KIRQDeferredHandler* deferredHandler,
    void* userData,
    KIRQPriorityLevels priority)
{
    kassert(!is_in_isr());
    if (irqNum < 0 || irqNum >= IRQ_COUNT || handler == nullptr ||
        priority < KIRQ_PRI_LOW_LATENCY_MAX || priority > KIRQ_PRI_KERNEL)
    {
        set_last_error(EINVAL);
        return -1;
    }
#ifdef PADOS_GPROF_HW_TIMER
    if (irqNum == get_timer_irq(PADOS_GPROF_TIMER_ID, HWTimerIRQType::Update))
    {
        set_last_error(EBUSY);
        return -1;
    }
#endif // PADOS_GPROF_HW_TIMER
    std::unique_ptr<KIRQAction> action(new(std::nothrow) KIRQAction);
    if (action == nullptr)
    {
        set_last_error(ENOMEM);
        return -1;
    }
    action->Handler = handler;
    action->DeferredHandler = deferredHandler;
    action->UserData = userData;
    action->Priority = priority;

    KSchedulerLock lock;
    const bool firstHandler = gk_IRQHandlers[irqNum] == nullptr;
    const bool lowLatency = priority < KIRQ_PRI_NORMAL_LATENCY_MAX;
    if (!firstHandler && lowLatency != (gk_IRQHandlers[irqNum]->Priority < KIRQ_PRI_NORMAL_LATENCY_MAX))
    {
        // Promoting a normal-latency handler could let its scheduler calls interrupt scheduler mutations.
        set_last_error(EBUSY);
        return -1;
    }
    const KIRQPriorityLevels effectivePriority = std::min(priority, get_irq_priority(gk_IRQHandlers[irqNum]));
    static int32_t currentHandle = 0;
    if (currentHandle == INT32_MAX)
    {
        set_last_error(ENOSPC);
        return -1;
    }
    const int32_t handle = ++currentHandle;
    action->Handle = handle;

    // KSchedulerLock excludes PendSV and other threads, but not low-latency handlers.
    KIRQGuard irqGuard(irqNum);
    if (firstHandler) {
        irqGuard.SetRestoreEnabled(true);
    }
    action->Next = gk_IRQHandlers[irqNum];
    gk_IRQHandlers[irqNum] = action.release();
    NVIC_SetPriority(irqNum, std::to_underlying(effectivePriority));
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int unregister_irq_handler(IRQn_Type irqNum, int handle)
{
    kassert(!is_in_isr());
    if (irqNum < 0 || irqNum >= IRQ_COUNT)
    {
        set_last_error(EINVAL);
        return -1;
    }
    std::unique_ptr<KIRQAction> removedAction;
    {
        KSchedulerLock lock;
        KIRQAction* previousAction = nullptr;
        for (KIRQAction* action = gk_IRQHandlers[irqNum]; action != nullptr; action = action->Next)
        {
            if (action->Handle == handle)
            {
                const KIRQPriorityLevels remainingPriority = get_irq_priority(gk_IRQHandlers[irqNum], action);
                KIRQGuard irqGuard(irqNum);
                if (previousAction != nullptr) {
                    previousAction->Next = action->Next;
                } else {
                    gk_IRQHandlers[irqNum] = action->Next;
                }
                removedAction.reset(action);
                if (gk_IRQHandlers[irqNum] != nullptr) {
                    NVIC_SetPriority(irqNum, std::to_underlying(remainingPriority));
                }
                if (gk_IRQHandlers[irqNum] == nullptr) {
                    irqGuard.SetRestoreEnabled(false);
                }
                break;
            }
            previousAction = action;
        }
    }
    if (removedAction != nullptr) {
        return 0;
    }
    set_last_error(EINVAL);
    return -1;
}

void dispatch_deferred_irq_handlers()
{
    kassert(KSchedulerLock::IsLocked());
    kassert(kis_in_irq(PendSV_IRQn));

    // Take one snapshot per word. An IRQ arriving during a callback remains pending for another pass.
    for (size_t wordIndex = 0; wordIndex < std::size(gk_DeferredIRQPendingFlags); ++wordIndex)
    {
        // A producer also pends PendSV, so work arriving after this check is handled on a later pass.
        if (gk_DeferredIRQPendingFlags[wordIndex].load(std::memory_order_relaxed) == 0) {
            continue;
        }
        uint32_t pending = gk_DeferredIRQPendingFlags[wordIndex].exchange(0, std::memory_order_acquire);
        while (pending != 0)
        {
            const uint32_t bitIndex = std::countr_zero(pending);
            pending &= pending - 1;
            const IRQn_Type irqNum = static_cast<IRQn_Type>(wordIndex * 32 + bitIndex);
            for (KIRQAction* action = gk_IRQHandlers[irqNum]; action != nullptr; action = action->Next)
            {
                if (action->DeferredPending.exchange(0, std::memory_order_acquire) != 0)
                {
                    KIRQTimeScope irqTime;
                    action->DeferredHandler(irqNum, action->UserData);
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValNanos kget_total_irq_time()
{
    KSchedulerLock lock;
    // select_thread() folds this counter on scheduler entry, normally at least once per millisecond.
    const uint32_t currentCycles = gk_TotalIRQCycles.load(std::memory_order_relaxed);
    const uint32_t elapsedCycles = currentCycles - gk_PreviousIRQCycles;
    if (elapsedCycles != 0)
    {
        gk_PreviousIRQCycles = currentCycles;
        const double elapsedNanoseconds = elapsedCycles * Kernel::GetCoreFrequencyToNanosecondScale() + gk_IRQNanosecondRemainder;
        const int64_t wholeNanoseconds = static_cast<int64_t>(elapsedNanoseconds);
        gk_IRQNanosecondRemainder = elapsedNanoseconds - static_cast<double>(wholeNanoseconds);
        gk_TotalIRQTime += TimeValNanos::FromNanoseconds(wholeNanoseconds);
    }
    return gk_TotalIRQTime;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" void KernelHandleIRQ()
{
    KIRQTimeScope irqTime;
    const int vector = SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk;
    if (vector >= 16) [[likely]]
    {
        const IRQn_Type irqNum = static_cast<IRQn_Type>(vector - 16);
        if (irqNum < IRQ_COUNT) [[likely]]
        {
            for (KIRQAction* action = gk_IRQHandlers[irqNum]; action != nullptr; action = action->Next)
            {
                const IRQResult result = action->Handler(irqNum, action->UserData);
                if (result == IRQResult::HANDLED_DEFERRED)
                {
                    kassert(action->DeferredHandler != nullptr);
                    action->DeferredPending.store(1, std::memory_order_release);
                    gk_DeferredIRQPendingFlags[irqNum / 32].fetch_or(uint32_t(1) << (irqNum % 32), std::memory_order_release);
                    KSWITCH_CONTEXT();
                }
                if (result != IRQResult::UNHANDLED) {
                    break;
                }
            }
        }
    }
    else
    {
        panic("Unhandled exception.");
    }
}

} // namespace kernel
