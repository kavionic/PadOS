// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"
#include <sys/errno.h>

#include <string.h>
#include <vector>
#include <map>

#include <Kernel/Scheduler.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/KProcess.h>
#include <Kernel/KSemaphore.h>
#include <Kernel/Kernel.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/VFS/KBlockCache.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/ThreadSyncDebugTracker.h>
#include "Ptr/NoPtr.h"

using namespace kernel;
using namespace os;

int main();


static uint8_t gk_FirstProcessBuffer[sizeof(KProcess)];
static KProcess& gk_FirstProcess = *reinterpret_cast<KProcess*>(gk_FirstProcessBuffer);

static uint8_t gk_IdleThreadBuffer[sizeof(NoPtr<KThreadCB>)];
static NoPtr<KThreadCB>& gk_IdleThreadInstance = *reinterpret_cast<NoPtr<KThreadCB>*>(gk_IdleThreadBuffer);

KProcess*  volatile kernel::gk_CurrentProcess = &gk_FirstProcess;

// Set the idle thread as the current thread, so that when the init thread is
// scheduled the initial context is dumped on the idle thread's task. The init
// thread will then overwrite that context with the real context needed to enter
// idle_thread_entry() when no other threads want's the CPU.

KThreadCB* volatile kernel::gk_CurrentThread;
KThreadCB*          kernel::gk_IdleThread;
thread_id           kernel::gk_MainThreadID;

static KThreadCB*                gk_InitThread = nullptr;

static KThreadList               gk_ReadyThreadLists[KTHREAD_PRIORITY_LEVELS];
static KThreadWaitList           gk_SleepingThreads;
static KThreadList               gk_ZombieThreadLists;
static uint8_t                   gk_ThreadTableBuffer[sizeof(KHandleArray<KThreadCB>)];
KHandleArray<KThreadCB>& kernel::gk_ThreadTable = *reinterpret_cast<KHandleArray<KThreadCB>*>(gk_ThreadTableBuffer);;
static volatile thread_id         gk_DebugWakeupThread = 0;


void* gk_CurrentTLS = nullptr;


static void wakeup_sleeping_threads();


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static uint8_t gk_IdleThreadStack[256] __attribute__((aligned(8)));
extern uint8_t _idle_tls_start;
extern uint8_t _idle_tls_end;

void kernel::initialize_scheduler_statics()
{
    try
    {
        PThreadAttribs idleAttrs("idle", KTHREAD_PRIORITY_MIN, PThreadDetachState_Detached, sizeof(gk_IdleThreadStack));
        idleAttrs.StackAddress = gk_IdleThreadStack;
        idleAttrs.ThreadLocalStorageAddress = &_idle_tls_start;
        idleAttrs.ThreadLocalStorageSize = &_idle_tls_end - &_idle_tls_start;

        new((void*)gk_FirstProcessBuffer) KProcess();
        new((void*)gk_ThreadTableBuffer) KHandleArray<KThreadCB>();
        new((void*)gk_IdleThreadBuffer) NoPtr<KThreadCB>(&idleAttrs);

        gk_IdleThread = &gk_IdleThreadInstance;
        gk_CurrentThread = gk_IdleThread;

        gk_CurrentTLS = gk_CurrentThread->m_ThreadLocalBuffer;
    }
    catch(std::exception& exc)
    {
        panic(exc.what());
    }
    catch (...)
    {
        panic("Unknown C++ exception");
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" __attribute__((naked)) void* __aeabi_read_tp(void)
{
    __asm__ volatile(
        "ldr   r0, =gk_CurrentTLS \n"
        "ldr   r0, [r0]           \n"
        "bx    lr                 \n"
        );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const KHandleArray<KThreadCB>& kernel::get_thread_table()
{
    return gk_ThreadTable;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC int kernel::get_remaining_stack()
{
    return __get_PSP() - intptr_t(gk_CurrentThread->GetStackTop());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void kernel::check_stack_overflow()
{
    if (get_remaining_stack() < 100) panic("Stackoverflow!\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" IFLASHC void SysTick_Handler()
{
    disable_interrupts();
	Kernel::s_SystemTime++;
    wakeup_sleeping_threads();
    KSWITCH_CONTEXT();
    restore_interrupts(0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::add_thread_to_ready_list(KThreadCB* thread)
{
    thread->m_State = ThreadState_Ready;
    gk_ReadyThreadLists[thread->m_PriorityLevel].Append(thread);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::add_thread_to_zombie_list(KThreadCB* thread)
{
    gk_ZombieThreadLists.Append(thread);
    if (gk_InitThread->m_State == ThreadState_Waiting) {
        add_thread_to_ready_list(gk_InitThread);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t wakeup_thread(thread_id handle)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);

    Ptr<KThreadCB> thread = get_thread(handle);
    if (thread == nullptr || thread->m_State == ThreadState_Zombie)
    {
        set_last_error(EINVAL);
        return -1;
    }
    if (thread->m_State == ThreadState_Sleeping || thread->m_State == ThreadState_Waiting) {
        add_thread_to_ready_list(ptr_raw_pointer_cast(thread));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

namespace
{

extern "C" IFLASHC uint32_t* select_thread(uint32_t* currentStack)
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        KThreadCB* prevThread = gk_CurrentThread;
        if (!prevThread->DebugValidate()) {
            return currentStack;
        }
        prevThread->m_CurrentStack = currentStack;
        if (intptr_t(prevThread->m_CurrentStack) <= intptr_t(prevThread->GetStackTop()))
        {
            panic("Stack overflow!\n");
            return currentStack;
        }
        for (int i = KTHREAD_PRIORITY_LEVELS - 1; i >= 0; --i)
        {
            KThreadCB* nextThread = gk_ReadyThreadLists[i].m_First;
            if (nextThread != nullptr)
            {
                if (prevThread->m_State != ThreadState_Running || i >= prevThread->m_PriorityLevel)
                {
                    gk_ReadyThreadLists[i].Remove(nextThread);
                    if (prevThread->m_State == ThreadState_Running) {
                        add_thread_to_ready_list(prevThread);
                    }
                    nextThread->m_State = ThreadState_Running;
//                    _impure_ptr = &nextThread->m_NewLibreent;
                    gk_CurrentThread = nextThread;
                    gk_CurrentTLS = nextThread->m_ThreadLocalBuffer;
                    nextThread->DebugValidate();
                    break;
                }
            }
        }
        if (prevThread->m_State == ThreadState_Zombie)
        {
            if (prevThread->m_DetachState == PThreadDetachState_Detached) {
                add_thread_to_zombie_list(prevThread);
            } else {
                wakeup_wait_queue(&prevThread->GetWaitQueue(), prevThread->m_ReturnValue, 0);
            }
        }
        TimeValNanos curTime = get_system_time_hires();
        prevThread->m_RunTime += curTime - prevThread->m_StartTime;
        gk_CurrentThread->m_StartTime = curTime;
    } CRITICAL_END;

    if (intptr_t(gk_CurrentThread->m_CurrentStack) <= intptr_t(gk_CurrentThread->GetStackTop())) {
        panic("Stack overflow!\n");
    }
    if (gk_DebugWakeupThread != 0 && gk_CurrentThread->GetHandle() == gk_DebugWakeupThread)
    {
        gk_DebugWakeupThread = 0;
        __BKPT(0);
    }

    return gk_CurrentThread->m_CurrentStack;
}

extern "C" IFLASHC void switch_context()
{
    KSWITCH_CONTEXT();
}

} // End of anonymous namespace

#if 0
__asm void SVCHandler(void)
{
    IMPORT SVCHandler_main
    TST lr, #4
    MRSEQ r0, MSP
    MRSNE r0, PSP
    B SVCHandler_main}
    
void SVCHandler_main(unsigned int * svc_args)
{
    unsigned int svc_number;
    /*    * Stack contains:    * r0, r1, r2, r3, r12, r14, the return address and xPSR    * First argument (r0) is svc_args[0]    */
    svc_number = ((char *)svc_args[6])[-2];
    switch(svc_number)
    {
        case SVC_00:            /* Handle SVC 00 */            break;
        case SVC_01:            /* Handle SVC 01 */            break;
        default:            /* Unknown SVC */            break;
    }
}
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" IFLASHC void SVCall_Handler(void)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static IFLASHC void start_first_thread()
{
    static uint32_t* SCB_VTOR_addr = (uint32_t*)&SCB->VTOR;
    void* dummyStack = gk_CurrentThread->m_CurrentStack;
    __asm volatile(
        "    ldr r0, %0\n"   // SCB->VTOR
        "    ldr r1, %1\n"
        "    ldr r0, [r0]\n" // Lookup the vector table
        "    ldr r0, [r0]\n" // Lookup the original stack.
        "    msr msp, r0\n"  // Restore msp to the start of the stack.
        "    msr psp, r1\n"
        "    dsb\n"
        "    isb\n"
        "    cpsie f\n"
        "    cpsie i\n"      // Globally enable interrupts.
        "    b switch_context\n"
        :: "m"(SCB_VTOR_addr), "m"(dummyStack)
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#if defined(STM32H7)
extern "C" IFLASHC void PendSV_Handler(void)
{
    __asm volatile
    (
    "    mrs r0, psp\n"
    "    isb\n"                     // Flush instruction pipeline.
    ""
    "    tst lr, #0x10\n"           // Test bit 4 in EXEC_RETURN to check if the thread use the FPU context.
    "    it eq\n"
    "    vstmdbeq r0!, {s16-s31}\n" // If bit 4 not set, push the high FPU registers.
    ""
    "    stmdb r0!, {r4-r11, lr}\n" // Push high core registers.
    ""
    "    bl select_thread\n"        // Ask the scheduler to find the next thread to run and update gk_CurrentThread.
    ""
    "    ldmia r0!, {r4-r11, lr}\n" // Pop high core registers.
    ""
    "    tst lr, #0x10\n"           // Test bit 4 in EXEC_RETURN to check if the thread use the FPU context.
    "    it eq\n"
    "    vldmiaeq r0!, {s16-s31}\n" // If bit 4 not set, pop the high FPU registers.
    ""
    "    msr psp, r0\n"
    "    isb\n"                     // Flush instruction pipeline.
    "    bx lr\n"
    );
}
#elif defined(STM32G030xx)

extern "C" IFLASHC void PendSV_Handler(void)
{
    __asm volatile
    (
        "    mrs r0, psp\n"
        "    isb\n"                     // Flush instruction pipeline.
        ""
        "    stmea r0!, {r4-r7}\n" // Push high core registers.
        ""
        "    bl select_thread\n"        // Ask the scheduler to find the next thread to run and update gk_CurrentThread.
        ""
        "    ldmia r0!, {r4-r7}\n" // Pop high core registers.
        ""
        "    msr psp, r0\n"
        "    isb\n"                     // Flush instruction pipeline.
        "    bx lr\n"
        );
}
#else
#error Unknown platform.
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool IFLASHC kernel::wakeup_wait_queue(KThreadWaitList* queue, void* returnValue, int maxCount)
{
    int ourPriLevel = gk_CurrentThread->m_PriorityLevel;
    bool needSchedule = false;
    
    if (maxCount == 0) maxCount = std::numeric_limits<int>::max();
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        for (KThreadWaitNode* waitNode = queue->m_First; waitNode != nullptr && maxCount != 0; waitNode = queue->m_First, --maxCount)
        {
            KThreadCB* thread = waitNode->m_Thread;
            if (thread != nullptr && (thread->m_State == ThreadState_Sleeping || thread->m_State == ThreadState_Waiting)) {
                if (thread->m_PriorityLevel > ourPriLevel) needSchedule = true;
                add_thread_to_ready_list(thread);
            }
            queue->Remove(waitNode);
        }
    } CRITICAL_END;
    return needSchedule;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static IFLASHC void wakeup_sleeping_threads()
{
    TimeValMicros curTime = TimeValMicros::FromNative(Kernel::s_SystemTime * SYS_TICKS_PER_SEC);

    for (KThreadWaitNode* waitNode = gk_SleepingThreads.m_First; waitNode != nullptr && waitNode->m_ResumeTime <= curTime; waitNode = gk_SleepingThreads.m_First)
    {
        KThreadCB* thread = waitNode->m_Thread;
        if (thread != nullptr && thread->m_State == ThreadState_Sleeping) {
            add_thread_to_ready_list(thread);
        }
        gk_SleepingThreads.Remove(waitNode);
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KThreadCB> IFLASHC kernel::get_thread(thread_id handle)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);
    return (thread != nullptr && thread->m_State != ThreadState_Deleted) ? thread : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess* IFLASHC kernel::get_current_process()
{
    return gk_CurrentProcess;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThreadCB* IFLASHC kernel::get_current_thread()
{
    return gk_CurrentThread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KIOContext* IFLASHC kernel::get_current_iocxt(bool forKernel)
{
    return gk_CurrentProcess->GetIOContext();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void kernel::add_to_sleep_list(KThreadWaitNode* waitNode)
{
    for (KThreadWaitNode* i = gk_SleepingThreads.m_First; i != nullptr; i = i->m_Next)
    {
        if (waitNode->m_ResumeTime <= i->m_ResumeTime) {
            gk_SleepingThreads.Insert(i, waitNode);
            break;
        }
    }
    if (waitNode->m_List == nullptr) {
        gk_SleepingThreads.Append(waitNode);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void kernel::remove_from_sleep_list(KThreadWaitNode* waitNode)
{
    gk_SleepingThreads.Remove(waitNode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC status_t snooze_until(TimeValMicros resumeTime)
{
    KThreadCB* thread = gk_CurrentThread;

    KThreadWaitNode waitNode;

    waitNode.m_Thread = thread;
    waitNode.m_ResumeTime = resumeTime + TimeValMicros::FromMilliseconds(1); // Add 1 tick-time to ensure we always round up.

    for (;;)
    {
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            add_to_sleep_list(&waitNode);
            thread->m_State = ThreadState_Sleeping;
            ThreadSyncDebugTracker::GetInstance().AddThread(thread, nullptr);
        } CRITICAL_END;

        KSWITCH_CONTEXT();

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
            ThreadSyncDebugTracker::GetInstance().RemoveThread(thread);
        } CRITICAL_END;
        if (get_system_time() >= waitNode.m_ResumeTime)
        {
            return 0;
        }
        else
        {
            set_last_error(EINTR);
            return -1;
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC status_t snooze(TimeValMicros delay)
{
    return snooze_until(get_system_time() + delay);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC status_t snooze_us(bigtime_t micros)
{
	return snooze_until(get_system_time() + TimeValMicros::FromMicroseconds(micros));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC status_t snooze_ms(bigtime_t millis)
{
    return snooze_until(get_system_time() + TimeValMicros::FromMilliseconds(millis));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC status_t snooze_s(bigtime_t seconds)
{
    return snooze_until(get_system_time() + TimeValMicros::FromSeconds(seconds));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC IRQEnableState kernel::get_interrupt_enabled_state()
{
#if defined(STM32H7)
    uint32_t basePri = __get_BASEPRI();
    if (basePri == 0) {
        return IRQEnableState::Enabled;
    } else if (basePri >= (KIRQ_PRI_NORMAL_LATENCY_MAX << (8-__NVIC_PRIO_BITS))) {
        return IRQEnableState::NormalLatencyDisabled;
    } else {
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

IFLASHC void kernel::set_interrupt_enabled_state(IRQEnableState state)
{
#if defined(STM32H7)
    switch(state)
    {
        case IRQEnableState::Enabled:               __set_BASEPRI(0); break;
        case IRQEnableState::NormalLatencyDisabled: __set_BASEPRI(KIRQ_PRI_NORMAL_LATENCY_MAX << (8-__NVIC_PRIO_BITS)); break;
        case IRQEnableState::LowLatencyDisabled:    __set_BASEPRI(KIRQ_PRI_LOW_LATENCY_MAX << (8-__NVIC_PRIO_BITS)); break;
    }
#elif defined(STM32G030xx)
    __set_PRIMASK((state == IRQEnableState::Enabled) ? 0 : 1);
#else
#error Unknown platform.
#endif
    __DSB();
    __ISB();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC uint32_t kernel::disable_interrupts()
{
#if defined(STM32H7)
    const uint32_t oldState = __get_BASEPRI();
    __set_BASEPRI(KIRQ_PRI_NORMAL_LATENCY_MAX << (8-__NVIC_PRIO_BITS));
#elif defined(STM32G030xx)
    const uint32_t oldState = __get_PRIMASK();
    __disable_irq();
#else
#error Unknown platform.
#endif
    __DSB();
    __ISB();
    return oldState;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

#if defined(STM32H7)
IFLASHC uint32_t kernel::KDisableLowLatenctInterrupts()
{
    const uint32_t oldState = __get_BASEPRI();
    __set_BASEPRI(KIRQ_PRI_LOW_LATENCY_MAX << (8-__NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    return oldState;
}
#endif // defined(STM32H7)

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void kernel::restore_interrupts(uint32_t state)
{
    __DSB();
#if defined(STM32H7)
    __set_BASEPRI(state);
#elif defined(STM32G030xx)
    if ((state & 0x01) == 0)
    {
        __enable_irq();
    }
#else
#error Unknown platform.
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static IFLASHC void* idle_thread_entry(void* arguments)
{
    for(;;)
    {
//        __WFI();
        if (gk_DebugWakeupThread != 0)
        {
            wakeup_thread(gk_DebugWakeupThread);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* main_thread_entry(void* argument)
{
    KBlockCache::Initialize();
    main();
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static IFLASHC void* init_thread_entry(void* arguments)
{
    KThreadCB* thread = gk_CurrentThread;

    FileIO::Initialze();

    // To avoid any special cases in the first context switch we allow the
    // context switch routine to dump the initial context on the idle-thread's
    // stack. To make the idle-thread do it's job when all other threads go
    // to sleep, we must initialize it's stack properly before that happens.
    
    size_t mainThreadStackSize = size_t(arguments);
    gk_IdleThread->InitializeStack(idle_thread_entry, nullptr);

    PThreadAttribs attrs("main_thread", 0, PThreadDetachState_Detached, mainThreadStackSize);
    sys_thread_spawn(&gk_MainThreadID, &attrs, main_thread_entry, nullptr);

    for(;;)
    {
        KThreadList threadsToDelete;
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {        
            for (KThreadCB* zombie = gk_ZombieThreadLists.m_First; zombie != nullptr; zombie = gk_ZombieThreadLists.m_First)
            {
                gk_ZombieThreadLists.Remove(zombie);
                threadsToDelete.Append(zombie);
            }
        } CRITICAL_END;
        for (KThreadCB* zombie = threadsToDelete.m_First; zombie != nullptr; zombie = threadsToDelete.m_First)
        {
            threadsToDelete.Remove(zombie);
            zombie->m_State = ThreadState_Deleted;
            gk_ThreadTable.FreeHandle(zombie->GetHandle());
        }
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (gk_ZombieThreadLists.m_First == nullptr)
            {
                thread->m_State = ThreadState_Waiting;

                KSWITCH_CONTEXT();
            }
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

IFLASHC void kernel::start_scheduler(uint32_t coreFrequency, size_t mainThreadStackSize)
{
    NVIC_SetPriority(PendSV_IRQn, KIRQ_PRI_KERNEL);
    NVIC_SetPriority(SysTick_IRQn, KIRQ_PRI_KERNEL);

    gk_IdleThread->SetHandle(0);
    gk_IdleThread->m_State = ThreadState_Running;

    thread_id idleThreadID = INVALID_HANDLE;
    gk_ThreadTable.AllocHandle(idleThreadID);
    gk_ThreadTable.Set(idleThreadID, gk_IdleThreadInstance);
    PThreadAttribs attrs("init", 0, PThreadDetachState_Detached);
    thread_id initThreadHandle = INVALID_HANDLE;
    sys_thread_spawn(&initThreadHandle, &attrs, init_thread_entry, (void*)mainThreadStackSize);

    gk_InitThread = ptr_raw_pointer_cast(get_thread(initThreadHandle));


#if defined(STM32H7)
    __set_BASEPRI(0);
#elif defined(STM32G030xx)
    __set_PRIMASK(0);
#else
#error Unknown platform.
#endif
    __DSB();
    __ISB();

    SysTick->LOAD = coreFrequency / 1000 - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    start_first_thread();

    panic("Failed to launch first thread!\n");
    for (;;);
}
