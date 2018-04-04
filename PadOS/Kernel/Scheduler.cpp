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

#include "sam.h"
#include <sys/errno.h>

#include <string.h>
#include <vector>
#include <map>

#include "Scheduler.h"
#include "KProcess.h"
#include "KSemaphore.h"
#include "Kernel.h"
#include "KHandleArray.h"
#include "SystemSetup.h"

#include "Kernel/HAL/SAME70System.h"

using namespace kernel;

static KProcess gk_FirstProcess;
KProcess*  volatile kernel::gk_CurrentProcess = &gk_FirstProcess;
KThreadCB* volatile kernel::gk_CurrentThread = nullptr;

static KThreadCB*                gk_IdleThread = nullptr;
static KThreadCB*                gk_InitThread = nullptr;

static KThreadList               gk_ReadyThreadLists[KTHREAD_PRIORITY_LEVELS];
static KThreadWaitList           gk_SleepingThreads;
static KThreadList               gk_ZombieThreadLists;
static KHandleArray<KThreadCB>   gk_ThreadTable;

static void wakeup_sleeping_threads();

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kernel::get_remaining_stack()
{
    return __get_PSP() - intptr_t(gk_CurrentThread->GetStackBottom());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::check_stack_overflow()
{
    if (get_remaining_stack() < 100) panic("Stackoverflow!\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SysTick_Handler()
{
    disable_interrupts();
    Kernel::SystemTick();
    wakeup_sleeping_threads();
    KSWITCH_CONTEXT();
    restore_interrupts(0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void add_thread_to_ready_list(KThreadCB* thread)
{
    thread->m_State = KThreadState::Ready;
    gk_ReadyThreadLists[thread->m_PriorityLevel].Append(thread);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

namespace
{
extern "C" uint32_t* select_thread(uint32_t* currentStack)
{
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        KThreadCB* prevThread = gk_CurrentThread;
        if (!prevThread->DebugValidate()) {
            return currentStack;
        }
        prevThread->m_CurrentStack = currentStack;
        if (intptr_t(prevThread->m_CurrentStack) <= intptr_t(prevThread->GetStackBottom())) {
            panic("Stack overflow!\n");
            return currentStack;
        }
        for (int i = KTHREAD_PRIORITY_LEVELS - 1; i >= 0; --i)
        {
            KThreadCB* nextThread = gk_ReadyThreadLists[i].m_First;
            if (nextThread != nullptr)
            {
                if (prevThread->m_State != KThreadState::Running || i >= prevThread->m_PriorityLevel)
                {
                    gk_ReadyThreadLists[i].Remove(nextThread);
                    if (prevThread->m_State == KThreadState::Running) {
                        add_thread_to_ready_list(prevThread);
                    }
                    nextThread->m_State = KThreadState::Running;
                    _impure_ptr = &nextThread->m_NewLibreent;
                    gk_CurrentThread = nextThread;
                    nextThread->DebugValidate();
                    break;
                }
            }
        }
        if (prevThread->m_State == KThreadState::Zombie)
        {
            if (!prevThread->m_IsJoinable)
            {
                gk_ZombieThreadLists.Append(prevThread);
                if (gk_InitThread->m_State == KThreadState::Waiting) {
                    add_thread_to_ready_list(gk_InitThread);
                }
            }
            else
            {
                wakeup_wait_queue(&prevThread->m_WaitingThreads, prevThread->m_NewLibreent._errno, 0);
            }
        }
    } CRITICAL_END;

    if (intptr_t(gk_CurrentThread->m_CurrentStack) <= intptr_t(gk_CurrentThread->GetStackBottom())) {
        panic("Stack overflow!\n");
    }
    return gk_CurrentThread->m_CurrentStack;
}

extern "C" void switch_context()
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

void SVCall_Handler( void )
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void start_first_thread()
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

void PendSV_Handler( void )
{
    __asm volatile
    (
    "    mrs r0, psp\n"
    "    isb\n"
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
    "    isb\n"
    "    bx lr\n"
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool kernel::wakeup_wait_queue(KThreadWaitList* queue, int returnCode, int maxCount)
{
    int ourPriLevel = gk_CurrentThread->m_PriorityLevel;
    bool needSchedule = false;
    
    if (maxCount == 0) maxCount = std::numeric_limits<int>::max();
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        for (KThreadWaitNode* waitNode = queue->m_First; waitNode != nullptr && maxCount != 0; waitNode = queue->m_First, --maxCount)
        {
            KThreadCB* thread = waitNode->m_Thread;
            if (thread != nullptr && (thread->m_State == KThreadState::Sleeping || thread->m_State == KThreadState::Waiting)) {
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

static void wakeup_sleeping_threads()
{
    bigtime_t curTime = Kernel::GetTime();

    for (KThreadWaitNode* waitNode = gk_SleepingThreads.m_First; waitNode != nullptr && waitNode->m_ResumeTime <= curTime; waitNode = gk_SleepingThreads.m_First)
    {
        KThreadCB* thread = waitNode->m_Thread;
        if (thread != nullptr && thread->m_State == KThreadState::Sleeping) {
            add_thread_to_ready_list(thread);
        }
        gk_SleepingThreads.Remove(waitNode);
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KThreadCB> kernel::get_thread(thread_id handle)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);
    return (thread != nullptr && thread->m_State != KThreadState::Deleted) ? thread : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

thread_id spawn_thread(const char* name, ThreadEntryPoint_t entryPoint, int priority, void* arguments, bool joinable, int stackSize)
{
    Ptr<KThreadCB> thread;
    int handle;

    try
    {
        thread = ptr_new<KThreadCB>(name, priority, joinable, stackSize);
    }
    catch (const std::bad_alloc& error)
    {
        set_last_error(ENOMEM);
        return -1;
    }
    thread->InitializeStack(entryPoint, arguments);

    handle = gk_ThreadTable.AllocHandle();
    if (handle != -1)
    {
        thread->SetHandle(handle);
        gk_ThreadTable.Set(handle, thread);
    }
    else
    {
        return -1;
    }
    CRITICAL_BEGIN(CRITICAL_IRQ)
    {
        add_thread_to_ready_list(ptr_raw_pointer_cast(thread));
    } CRITICAL_END;
    return handle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int exit_thread(int returnCode)
{
    KProcess*  process = gk_CurrentProcess;
    KThreadCB* thread = gk_CurrentThread;

    process->ThreadQuit(thread);

    thread->m_State = KThreadState::Zombie;
    thread->m_NewLibreent._errno = returnCode;

    KSWITCH_CONTEXT();

    panic("exit_thread() survived a context switch!\n");
    for(;;);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int wait_thread(thread_id handle)
{
    KThreadCB* thread = gk_CurrentThread;

    for (;;)
    {
        Ptr<KThreadCB> child = gk_ThreadTable.Get(handle);

        if (child == nullptr) {
            set_last_error(EINVAL);
            return -1;
        }

        KThreadWaitNode waitNode;
        waitNode.m_Thread = thread;

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (child->m_State == KThreadState::Deleted)
            {
                set_last_error(EINVAL); // Someone just beat us
                return -1;
            }
            if (child->m_State != KThreadState::Zombie)
            {
                thread->m_State = KThreadState::Waiting;
                child->m_WaitingThreads.Append(&waitNode);
                KSWITCH_CONTEXT();
            }
        } CRITICAL_END;
        // If we ran KSWITCH_CONTEXT() we should be suspended here.
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
            
            if (child->m_State == KThreadState::Deleted) {
                set_last_error(EINVAL); // Someone beat us
                return -1;
            }
            
            if (child->m_State != KThreadState::Zombie) // We got interrupted
            {
                if (thread->m_RestartSyscalls) {
                    continue;
                } else {
                    set_last_error(EINTR);
                    return -1;
                }
            }
        } CRITICAL_END;
        int returnCode = child->m_NewLibreent._errno;
        gk_ThreadTable.FreeHandle(handle);
        return returnCode;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t wakeup_thread(thread_id handle)
{
    CRITICAL_SCOPE(CRITICAL_IRQ);

    Ptr<KThreadCB> thread = get_thread(handle);
    if (thread == nullptr || thread->m_State == KThreadState::Zombie) {
        set_last_error(EINVAL);
        return -1;
    }
    if (thread->m_State == KThreadState::Sleeping || thread->m_State == KThreadState::Waiting) {
        add_thread_to_ready_list(ptr_raw_pointer_cast(thread));
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

thread_id get_thread_id()
{
    return gk_CurrentThread->GetHandle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int thread_yield()
{
    KSWITCH_CONTEXT();
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::add_to_sleep_list(KThreadWaitNode* waitNode)
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

void kernel::remove_from_sleep_list(KThreadWaitNode* waitNode)
{
    gk_SleepingThreads.Remove(waitNode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t snooze(bigtime_t micros)
{
    return snooze_until(get_system_time() + micros);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t snooze_until(bigtime_t resumeTime)
{
    KThreadCB* thread = gk_CurrentThread;

    KThreadWaitNode waitNode;

    waitNode.m_Thread = thread;
    waitNode.m_ResumeTime = resumeTime;

    for (;;)
    {
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            add_to_sleep_list(&waitNode);
            thread->m_State = KThreadState::Sleeping;
        } CRITICAL_END;

        KSWITCH_CONTEXT();

        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            waitNode.Detatch();
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

uint32_t kernel::disable_interrupts()
{
    uint32_t oldState = __get_BASEPRI();
    __disable_irq();
    __set_BASEPRI(KIRQ_PRI_NORMAL_LATENCY_MAX << (8-__NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    __enable_irq();
    return oldState;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t kernel::KDisableLowLatenctInterrupts()
{
    uint32_t oldState = __get_BASEPRI();
    __disable_irq();
    __set_BASEPRI(KIRQ_PRI_LOW_LATENCY_MAX << (8-__NVIC_PRIO_BITS));
    __DSB();
    __ISB();
    __enable_irq();
    return oldState;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::restore_interrupts(uint32_t state)
{
    __disable_irq();
    __set_BASEPRI(state);
    __enable_irq();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void idle_thread_entry(void* arguments)
{
    for(;;)
    {
        __WFI();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void init_thread_entry(void* arguments)
{
    KThreadCB* thread = gk_CurrentThread;

    // To avoid any special cases in the first context switch we allow the
    // context switch routine to dump the initial context on the idle-thread's
    // stack. To make the idle-thread do it's job when all other threads go
    // to sleep, we must initialize it's stack properly before that happens.
    
    gk_IdleThread->InitializeStack(idle_thread_entry, nullptr);
    spawn_thread("main_thread", InitThreadMain, 0, nullptr, false);

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
            gk_ThreadTable.FreeHandle(zombie->GetHandle());
        }
        CRITICAL_BEGIN(CRITICAL_IRQ)
        {
            if (threadsToDelete.m_First == nullptr)
            {
                thread->m_State = KThreadState::Waiting;
                KSWITCH_CONTEXT();
            }
        } CRITICAL_END;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kernel::start_scheduler()
{
    NVIC_SetPriority(PendSV_IRQn, KIRQ_PRI_KERNEL);
    NVIC_SetPriority(SysTick_IRQn, KIRQ_PRI_KERNEL);

    //gk_CurrentProcess = new KProcess();

    Ptr<KThreadCB> idleThread = ptr_new<KThreadCB>("idle", KTHREAD_PRIORITY_MIN, false, 256);
    idleThread->SetHandle(0);
    idleThread->m_State = KThreadState::Running;

    gk_IdleThread = ptr_raw_pointer_cast(idleThread);

    // Set the idle thread as the current thread, so that when the init thread is
    // scheduled the initial context is dumped on the idle thread's task. The init
    // thread will then overwrite that context with the real context needed to enter
    // idle_thread_entry() when no other threads want's the CPU.

    gk_CurrentThread = gk_IdleThread;
    gk_ThreadTable.Set(gk_ThreadTable.AllocHandle(), idleThread);

    thread_id initThreadHandle = spawn_thread("init", init_thread_entry, 0, nullptr, false, 0);

    gk_InitThread = ptr_raw_pointer_cast(get_thread(initThreadHandle));


    __disable_irq();
    __set_BASEPRI(0);
    __DSB();
    __ISB();

    SysTick->LOAD = SAME70System::GetFrequencyCore() / 1000 - 1;
    SysTick->VAL  = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    start_first_thread();

    // Should never get here!
    panic("Failed to launch first thread!\n");
}

