// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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

namespace kernel
{

class KProcess;
class KIOContext;

extern void InitThreadMain(void* argument);


#define KSTACK_ALIGNMENT 8

#define KSWITCH_CONTEXT() do {SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;} while(false)
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
	KIRQP_PRI_unused1,KIRQP_PRI_unused2,KIRQP_PRI_unused3,KIRQP_PRI_unused4,KIRQP_PRI_unused5,KIRQP_PRI_unused6,KIRQP_PRI_unused7,KIRQP_PRI_unused8,
#else
#endif
    KIRQ_PRI_NORMAL_LATENCY1,
    KIRQ_PRI_KERNEL = KIRQ_PRI_NORMAL_LATENCY1
};



extern KProcess*  volatile gk_CurrentProcess;
extern KThreadCB* volatile gk_CurrentThread;
extern KThreadCB* gk_IdleThread;

Ptr<KThreadCB> get_thread(thread_id handle);

KProcess*      get_current_process();
KThreadCB*     get_current_thread();
KIOContext*    get_current_iocxt(bool forKernel);

void add_to_sleep_list(KThreadWaitNode* waitNode);
void remove_from_sleep_list(KThreadWaitNode* waitNode);

void add_thread_to_ready_list(KThreadCB* thread);

bool wakeup_wait_queue(KThreadWaitList* queue, int returnCode, int maxCount);


void start_scheduler(uint32_t coreFrequency, size_t mainThreadStackSize);

enum class IRQEnableState
{
    Enabled,
    NormalLatencyDisabled,
    LowLatencyDisabled,
};

IRQEnableState get_interrupt_enabled_state();
void           set_interrupt_enabled_state(IRQEnableState state);

uint32_t disable_interrupts();
uint32_t KDisableLowLatenctInterrupts();

void restore_interrupts(uint32_t state);

int  get_remaining_stack();
void check_stack_overflow();

class IRQDisabler
{
public:
    IRQDisabler() { Acquire(); }
    ~IRQDisabler() { Release(); }

    void Acquire() { if (m_LockCount++ == 0) m_PrevState = disable_interrupts(); }
    void Release() { if (--m_LockCount == 0) restore_interrupts(m_PrevState); }

    IRQDisabler(IRQDisabler&& other) : m_PrevState(other.m_PrevState), m_LockCount(other.m_LockCount) {
        other.m_LockCount = -1;
    }
private:
    uint32_t m_PrevState;
    int32_t  m_LockCount = 0;

    IRQDisabler( const IRQDisabler &c ) = delete;
    IRQDisabler& operator=( const IRQDisabler &c ) = delete;

};

#define CRITICAL_IRQ kernel::IRQDisabler()
inline IRQDisabler&& critical_create_guard(IRQDisabler&& lock) { return std::move(lock); }


struct IRQDisablerWithIterator : public IRQDisabler
{
    IRQDisablerWithIterator(int i) : iterator(i) {}
    int iterator = 0;
};

//#define KCRITICAL_SECTION() for (IRQDisablerWithIterator i(1); i.iterator != 0; --i.iterator)

//#define KNOIRQ_BEGIN() { IRQDisabler irqDisableGuard##__LINE();
//#define KNOIRQ_END }
#define DEBUG_DISABLE_IRQ() //IRQDisabler debugIRQGuard__


struct KCtxSwitchStackFrame
{
    // Kernel frame:
    uint32_t m_R11;
    uint32_t m_R10;
    uint32_t m_R9;
    uint32_t m_R8;
    uint32_t m_R7;
    uint32_t m_R6;
    uint32_t m_R5;
    uint32_t m_R4;
    uint32_t m_EXEC_RETURN;
    // Default frame:
    uint32_t m_R0;
    uint32_t m_R1;
    uint32_t m_R2;
    uint32_t m_R3;
    uint32_t m_R12;
    uint32_t m_LR;
    uint32_t m_PC;
    uint32_t m_xPSR;
//    uint32_t m_padding; // Ensure 8-byte alingnment
};

struct KCtxSwitchStackFrameFPU
{
    // Kernel frame:
    uint32_t m_R11;
    uint32_t m_R10;
    uint32_t m_R9;
    uint32_t m_R8;
    uint32_t m_R7;
    uint32_t m_R6;
    uint32_t m_R5;
    uint32_t m_R4;
    uint32_t m_EXEC_RETURN;
    uint32_t m_S31;
    uint32_t m_S30;
    uint32_t m_S29;
    uint32_t m_S28;
    uint32_t m_S27;
    uint32_t m_S26;
    uint32_t m_S25;
    uint32_t m_S24;
    uint32_t m_S23;
    uint32_t m_S22;
    uint32_t m_S21;
    uint32_t m_S20;
    uint32_t m_S19;
    uint32_t m_S18;
    uint32_t m_S17;
    uint32_t m_S16;
    // Default frame:
    uint32_t m_R0;
    uint32_t m_R1;
    uint32_t m_R2;
    uint32_t m_R3;
    uint32_t m_R12;
    uint32_t m_LR;
    uint32_t m_PC;
    uint32_t m_xPSR;
    uint32_t m_S0;
    uint32_t m_S1;
    uint32_t m_S2;
    uint32_t m_S3;
    uint32_t m_S4;
    uint32_t m_S5;
    uint32_t m_S6;
    uint32_t m_S7;
    uint32_t m_S8;
    uint32_t m_S9;
    uint32_t m_S10;
    uint32_t m_S11;
    uint32_t m_S12;
    uint32_t m_S13;
    uint32_t m_S14;
    uint32_t m_S15;
    uint32_t m_FPSCR;
    uint32_t m_padding; // Ensure 8-byte alingnment
};

} // namespace
