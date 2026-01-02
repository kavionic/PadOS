// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 27.02.2018 21:06:38

#pragma once

#include "Kernel.h"
#include "KThreadCB.h"


extern "C" void initialize_device_drivers();

namespace kernel
{

class KProcess;
class KThreadCB;
class KIOContext;

template<typename T> class KHandleArray;

void initialize_scheduler_statics();


#define KSTACK_ALIGNMENT 8

#define KSWITCH_CONTEXT() do {SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; __DSB();} while(false)
enum KIRQPriorityLevels
{
    KIRQ_PRI_LOW_LATENCY_MAX = 1,
    KIRQ_PRI_LOW_LATENCY3 = KIRQ_PRI_LOW_LATENCY_MAX,
    KIRQ_PRI_LOW_LATENCY2,
    KIRQ_PRI_LOW_LATENCY1,
    KIRQ_PRI_NORMAL_LATENCY_MAX,
    KIRQ_PRI_NORMAL_LATENCY4 = KIRQ_PRI_NORMAL_LATENCY_MAX,
    KIRQ_PRI_NORMAL_LATENCY3,
    KIRQ_PRI_NORMAL_LATENCY2,
#if	__NVIC_PRIO_BITS == 3
#elif __NVIC_PRIO_BITS == 4
    KIRQP_PRI_unused1, KIRQP_PRI_unused2, KIRQP_PRI_unused3, KIRQP_PRI_unused4, KIRQP_PRI_unused5, KIRQP_PRI_unused6, KIRQP_PRI_unused7, KIRQP_PRI_unused8,
#else
#endif
    KIRQ_PRI_NORMAL_LATENCY1,
    KIRQ_PRI_KERNEL = KIRQ_PRI_NORMAL_LATENCY1
};



extern KProcess* volatile      gk_CurrentProcess;
extern "C" KThreadCB* volatile gk_CurrentThread;
extern KThreadCB* gk_IdleThread;
extern thread_id                gk_MainThreadID;
extern KHandleArray<KThreadCB>& gk_ThreadTable;

Ptr<KThreadCB> get_thread(thread_id handle);

KProcess* get_current_process();
KThreadCB* get_current_thread();
KIOContext* get_current_iocxt(bool forKernel);

void add_to_sleep_list(KThreadWaitNode* waitNode);
void remove_from_sleep_list(KThreadWaitNode* waitNode);

void add_thread_to_ready_list(KThreadCB* thread);
void add_thread_to_zombie_list(KThreadCB* thread);

bool wakeup_wait_queue(KThreadWaitList* queue, void* returnValue, int maxCount);

void        stop_thread(bool notifyParent);
PErrorCode  wakeup_thread(thread_id handle, bool wakeupSuspended);


void start_scheduler(uint32_t coreFrequency, size_t mainThreadStackSize);

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

IRQEnableState get_interrupt_enabled_state();
void           set_interrupt_enabled_state(IRQEnableState state);

uint32_t disable_interrupts();
#if defined(STM32H7)
uint32_t KDisableLowLatenctInterrupts();
#endif // defined(STM32H7)

void restore_interrupts(uint32_t state);


const KHandleArray<KThreadCB>& get_thread_table();
int  get_remaining_stack();
void check_stack_overflow();

class IRQDisabler
{
public:
    IRQDisabler() : m_IsLocked(true) { m_PrevState = disable_interrupts(); }
    ~IRQDisabler() { if (m_IsLocked) restore_interrupts(m_PrevState); }

    IRQDisabler(IRQDisabler&& other) : m_PrevState(other.m_PrevState), m_IsLocked(other.m_IsLocked) {
        other.m_IsLocked = false;
    }
private:
    uint32_t    m_PrevState;
    bool        m_IsLocked;

    IRQDisabler(const IRQDisabler& c) = delete;
    IRQDisabler& operator=(const IRQDisabler& c) = delete;
};

#define CRITICAL_IRQ IRQDisabler()
inline IRQDisabler&& critical_create_guard(IRQDisabler&& lock) { return std::move(lock); }



} // namespace
