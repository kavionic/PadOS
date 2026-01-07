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

#include "System/Platform.h"
#include <sys/errno.h>

#include <string.h>
#include <vector>
#include <map>

#include <Kernel/KTime.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KPosixSignals.h>
#include <Kernel/HAL/DigitalPort.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <Kernel/KThread.h>
#include <Kernel/KProcess.h>
#include <Kernel/KSemaphore.h>
#include <Kernel/Kernel.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/VFS/KBlockCache.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/ThreadSyncDebugTracker.h>
#include <Kernel/Syscalls.h>
#include <Kernel/KLogging.h>
#include <System/AppDefinition.h>
#include <Ptr/NoPtr.h>

using namespace os;

extern "C" void __libc_init_array(void);

static uint8_t gk_IdleThreadStack[256] __attribute__((aligned(8)));
static uint8_t gk_InitThreadStack[32768] __attribute__((aligned(8)));
extern uint8_t _idle_tls_start;
extern uint8_t _idle_tls_end;



namespace kernel
{
static uint8_t gk_InitialBlocksBuffer[2][sizeof(KHandleArrayBlock)] __attribute__((aligned(8)));
static Ptr<KHandleArrayBlock> gk_InitialBlocks[2] __attribute__((aligned(8)));

static uint8_t gk_FirstProcessBuffer[sizeof(KProcess)] __attribute__((aligned(8)));
static KProcess& gk_FirstProcess = *reinterpret_cast<KProcess*>(gk_FirstProcessBuffer);

static uint8_t gk_IdleThreadBuffer[sizeof(NoPtr<KThreadCB>)] __attribute__((aligned(8)));
static NoPtr<KThreadCB>& gk_IdleThreadInstance = *reinterpret_cast<NoPtr<KThreadCB>*>(gk_IdleThreadBuffer);
static uint8_t gk_InitThreadBuffer[sizeof(KThreadCB)] __attribute__((aligned(8)));

KProcess* volatile gk_CurrentProcess = &gk_FirstProcess;
KThreadCB* volatile gk_CurrentThread;

KThreadCB* gk_IdleThread;
thread_id  gk_MainThreadID;

static KThreadCB* gk_InitThread = nullptr;

static KThreadList          gk_ReadyThreadLists[KTHREAD_PRIORITY_LEVELS];
static KThreadWaitList      gk_SleepingThreads;
static KThreadList          gk_ZombieThreadLists;
static uint8_t              gk_ThreadTableBuffer[sizeof(KHandleArray<KThreadCB>)] __attribute__((aligned(8)));
KHandleArray<KThreadCB>&    gk_ThreadTable = *reinterpret_cast<KHandleArray<KThreadCB>*>(gk_ThreadTableBuffer);;
static volatile thread_id   gk_DebugWakeupThread = 0;


static void wakeup_sleeping_threads();


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void initialize_scheduler_statics()
{
    try
    {
        PThreadAttribs idleAttrs("idle", KTHREAD_PRIORITY_MIN, PThreadDetachState_Detached, sizeof(gk_IdleThreadStack));
        idleAttrs.StackAddress = gk_IdleThreadStack;
//        idleAttrs.ThreadLocalStorageAddress = &_idle_tls_start;
//        idleAttrs.ThreadLocalStorageSize = &_idle_tls_end - &_idle_tls_start;

        new((void*)gk_FirstProcessBuffer) KProcess();
        new((void*)gk_ThreadTableBuffer) KHandleArray<KThreadCB>();
        new((void*)gk_IdleThreadBuffer) NoPtr<KThreadCB>(&idleAttrs, nullptr, &_idle_tls_start);

        // Set the idle thread as the current thread, so that when the init thread is
        // scheduled the initial context is dumped on the idle thread's task. The init
        // thread will then overwrite that context with the real context needed to enter
        // idle_thread_entry() when no other threads want's the CPU.

        gk_IdleThread = &gk_IdleThreadInstance;
        gk_CurrentThread = gk_IdleThread;

        __kernel_thread_data = gk_CurrentThread->m_KernelTLS;

        for (size_t i = 0; i < ARRAY_COUNT(gk_InitialBlocksBuffer); ++i)
        {
            gk_InitialBlocks[i] = ptr_new_cast(new ((void*)&gk_InitialBlocksBuffer[i]) KHandleArrayBlock());
            gk_ThreadTable.CacheBlock(gk_InitialBlocks[i]);
        }
    }
    catch (std::exception& exc)
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
        "ldr   r0, =__kernel_thread_data \n"
        "ldr   r0, [r0]           \n"
        "bx    lr                 \n"
        );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const KHandleArray<KThreadCB>& get_thread_table()
{
    return gk_ThreadTable;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int get_remaining_stack()
{
    return __get_PSP() - intptr_t(gk_CurrentThread->GetStackTop());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void check_stack_overflow()
{
    if (get_remaining_stack() < 100) panic("Stackoverflow!\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" void SysTick_Handler()
{
    CRITICAL_SCOPE(CRITICAL_IRQ);
    Kernel::s_SystemTimeNS += 1000000;
    Kernel::s_SystemTicks += SysTick->LOAD + 1;
    wakeup_sleeping_threads();
    KSWITCH_CONTEXT();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void add_thread_to_ready_list(KThreadCB* thread)
{
    thread->m_State = ThreadState_Ready;
    gk_ReadyThreadLists[thread->m_PriorityLevel].Append(thread);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void add_thread_to_zombie_list(KThreadCB* thread)
{
    gk_ZombieThreadLists.Append(thread);
    if (gk_InitThread->m_State == ThreadState_Waiting) {
        add_thread_to_ready_list(gk_InitThread);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void stop_thread(bool notifyParent)
{
    KThreadCB* const thread = gk_CurrentThread;

    KSchedulerLock slock;

//    if (notifyParent && thread->m_Parent != INVALID_HANDLE) {
//        sys_kill(thread->m_Parent, SIGCHLD);
//    }

    thread->m_State = ThreadState_Stopped;

    KSWITCH_CONTEXT();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode wakeup_thread(thread_id handle, bool wakeupSuspended)
{
    KSchedulerLock slock;

    Ptr<KThreadCB> thread = get_thread(handle);
    if (thread == nullptr || thread->m_State == ThreadState_Zombie) {
        return PErrorCode::InvalidArg;
    }
    if (thread->m_State == ThreadState_Sleeping || thread->m_State == ThreadState_Waiting || (wakeupSuspended && thread->m_State == ThreadState_Stopped))
    {
        add_thread_to_ready_list(ptr_raw_pointer_cast(thread));
        return PErrorCode::Success;
    }
    return PErrorCode::InvalidArg;
}

namespace
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" uint32_t select_thread(uint32_t * currentStack, uint32_t controlReg)
{
    {
        KSchedulerLock slock;

        KThreadCB* const prevThread = gk_CurrentThread;
        const uint32_t stackAddrInt = intptr_t(currentStack);
        prevThread->m_CurrentStackAndPrivilege = stackAddrInt | (controlReg & 0x01); // Store nPRIV in bit 0 of stack address.
        if (stackAddrInt <= intptr_t(prevThread->GetStackTop())) [[unlikely]]
        {
            panic("Stack overflow!\n");
            return prevThread->m_CurrentStackAndPrivilege;
        }
        for (int i = KTHREAD_PRIORITY_LEVELS - 1; i >= 0; --i)
        {
            KThreadCB* const nextThread = gk_ReadyThreadLists[i].m_First;
            if (nextThread != nullptr)
            {
                if (prevThread->m_State != ThreadState_Running || i >= prevThread->m_PriorityLevel)
                {
                    gk_ReadyThreadLists[i].Remove(nextThread);
                    if (prevThread->m_State == ThreadState_Running) {
                        add_thread_to_ready_list(prevThread);
                    }
                    nextThread->m_State = ThreadState_Running;
                    gk_CurrentThread = nextThread;
                    __kernel_thread_data = gk_CurrentThread->m_KernelTLS;
                    __app_thread_data    = gk_CurrentThread->m_UserspaceTLS;

                    nextThread->DebugValidate();
                    break;
                }
            }
        }
        if (prevThread->m_State == ThreadState_Zombie) [[unlikely]]
        {
            if (prevThread->m_DetachState == PThreadDetachState_Detached) {
                add_thread_to_zombie_list(prevThread);
            }
            else {
                wakeup_wait_queue(&prevThread->GetWaitQueue(), prevThread->m_ReturnValue, 0);
            }
        }
        const TimeValNanos curTime = kget_monotonic_time_hires();
        prevThread->m_RunTime += curTime - prevThread->m_StartTime;
        gk_CurrentThread->m_StartTime = curTime;
    }

    if (gk_DebugWakeupThread != 0 && gk_CurrentThread->GetHandle() == gk_DebugWakeupThread) [[unlikely]]
    {
        gk_DebugWakeupThread = 0;
        __BKPT(0);
    }
    if ((gk_CurrentThread->m_CurrentStackAndPrivilege & 0x01) && gk_CurrentThread->HasUnblockedPendingSignals())
    {
        const uintptr_t newStackPtr = kprocess_pending_signals(gk_CurrentThread->m_CurrentStackAndPrivilege & ~0x01, /*userMode*/ true);
        gk_CurrentThread->m_CurrentStackAndPrivilege = (gk_CurrentThread->m_CurrentStackAndPrivilege & 0x01) | newStackPtr;
    }
    return gk_CurrentThread->m_CurrentStackAndPrivilege;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

extern "C" void switch_context()
{
    KSWITCH_CONTEXT();
}

} // End of anonymous namespace


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void start_first_thread()
{
    static uint32_t* SCB_VTOR_addr = (uint32_t*)&SCB->VTOR;
    const uint32_t dummyStack = gk_CurrentThread->m_CurrentStackAndPrivilege & ~0x01;
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
extern "C" __attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile
    (
        "   mrs     r0, psp\n"
        "   mrs     r1, CONTROL\n"
        ""
            ASM_STORE_SCHED_CONTEXT(r0)
        "   bl      select_thread\n"        // Ask the scheduler to find the next thread to run and update gk_CurrentThread.
        ""
        "   mrs     r1, CONTROL\n"
        "   bfi     r1, r0, #0, #1\n"       // Set nPRIV to bit 0 from the stack address returned by select_thread().
        "   msr     CONTROL, r1\n"
        "   isb\n"                          // Flush instruction pipeline.
        "   bic     r0, r0, #1\n"           // Clear bit 0 (nPRIV) from the stack address.
        ""
            ASM_LOAD_SCHED_CONTEXT(r0)
        ""
        "   msr     psp, r0\n"
        "   bx      lr\n"
        );
}
#elif defined(STM32G030xx)

extern "C" __attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile
    (
        "    mrs r0, psp\n"
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

bool wakeup_wait_queue(KThreadWaitList* queue, void* returnValue, int maxCount)
{
    int ourPriLevel = gk_CurrentThread->m_PriorityLevel;
    bool needSchedule = false;

    if (maxCount == 0) maxCount = std::numeric_limits<int>::max();

    KSchedulerLock slock;

    for (KThreadWaitNode* waitNode = queue->m_First; waitNode != nullptr && maxCount != 0; waitNode = queue->m_First, --maxCount)
    {
        KThreadCB* thread = waitNode->m_Thread;
        if (thread != nullptr && (thread->m_State == ThreadState_Sleeping || thread->m_State == ThreadState_Waiting)) {
            if (thread->m_PriorityLevel > ourPriLevel) needSchedule = true;
            add_thread_to_ready_list(thread);
        }
        queue->Remove(waitNode);
    }

    return needSchedule;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void wakeup_sleeping_threads()
{
    TimeValNanos curTime = TimeValNanos::FromNanoseconds(Kernel::s_SystemTimeNS);

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

Ptr<KThreadCB> get_thread(thread_id handle)
{
    Ptr<KThreadCB> thread = gk_ThreadTable.Get(handle);
    return (thread != nullptr && thread->m_State != ThreadState_Deleted) ? thread : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KProcess* get_current_process()
{
    return gk_CurrentProcess;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThreadCB* get_current_thread()
{
    return gk_CurrentThread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KIOContext* get_current_iocxt(bool forKernel)
{
    return gk_CurrentProcess->GetIOContext();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void add_to_sleep_list(KThreadWaitNode* waitNode)
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

void remove_from_sleep_list(KThreadWaitNode* waitNode)
{
    gk_SleepingThreads.Remove(waitNode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void* idle_thread_entry(void* arguments)
{
    for (;;)
    {
        //        __WFI();
        if (gk_DebugWakeupThread != 0)
        {
            wakeup_thread(gk_DebugWakeupThread, true);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* main_thread_entry(void* argument)
{
    kset_real_time(RealtimeClock::GetClock(), false);

    KBlockCache::Initialize();
    initialize_device_drivers();

    uint32_t control;
    __asm volatile ("mrs %0, CONTROL" : "=r"(control));
    control |= 1; // set nPRIV
    __asm volatile ("msr CONTROL, %0\n isb" :: "r"(control) : "memory");

    __app_definition.entry();

    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void* init_thread_entry(void* arguments)
{
    // Run global constructors.
    __libc_init_array();

    KThreadCB* thread = gk_CurrentThread;

    ksetup_rootfs_trw();

    // To avoid any special cases in the first context switch we allow the
    // context switch routine to dump the initial context on the idle-thread's
    // stack. To make the idle-thread do it's job when all other threads go
    // to sleep, we must initialize it's stack properly before that happens.

    size_t mainThreadStackSize = size_t(arguments);
    gk_IdleThread->InitializeStack(idle_thread_entry, /*privileged*/ true, /*skipEntryTrampoline*/ true, nullptr);

    PThreadAttribs attrs("main_thread", 0, PThreadDetachState_Detached, mainThreadStackSize);
    gk_MainThreadID = kthread_spawn_trw(&attrs, __app_definition.create_main_thread_tls_block(), /*privileged*/ true, main_thread_entry, nullptr);

    for (;;)
    {
        KThreadList threadsToDelete;
        {
            KSchedulerLock slock;

            for (KThreadCB* zombie = gk_ZombieThreadLists.m_First; zombie != nullptr; zombie = gk_ZombieThreadLists.m_First)
            {
                gk_ZombieThreadLists.Remove(zombie);
                threadsToDelete.Append(zombie);
            }
        }
        for (KThreadCB* zombie = threadsToDelete.m_First; zombie != nullptr; zombie = threadsToDelete.m_First)
        {
            threadsToDelete.Remove(zombie);
            zombie->m_State = ThreadState_Deleted;
            try
            {
                gk_ThreadTable.FreeHandle_trw(zombie->GetHandle());
            }
            catch(const std::exception& exc)
            {
                kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_Scheduler, "{}: failed to free zombie thread handle: {}", __PRETTY_FUNCTION__, exc.what());
            }
        }
        {
            KSchedulerLock slock;

            if (gk_ZombieThreadLists.m_First == nullptr)
            {
                thread->m_State = ThreadState_Waiting;

                KSWITCH_CONTEXT();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void start_scheduler(uint32_t coreFrequency, size_t mainThreadStackSize)
{
    NVIC_SetPriority(PendSV_IRQn, KIRQ_PRI_KERNEL);
    NVIC_SetPriority(SysTick_IRQn, KIRQ_PRI_KERNEL);
    NVIC_SetPriority(SVCall_IRQn, KIRQ_PRI_LOW_LATENCY_MAX);

    gk_IdleThread->SetHandle(0);
    gk_IdleThread->m_State = ThreadState_Running;

    thread_id idleThreadID = INVALID_HANDLE;
    gk_ThreadTable.AllocHandle(idleThreadID);
    gk_ThreadTable.Set(idleThreadID, gk_IdleThreadInstance);

    PThreadAttribs attrs("init", 0, PThreadDetachState_Detached, sizeof(gk_InitThreadStack));
    attrs.StackAddress = gk_InitThreadStack;

    thread_id initThreadHandle = INVALID_HANDLE;

    gk_InitThread = new((void*)gk_InitThreadBuffer)KThreadCB(&attrs, nullptr, &_idle_tls_start);
    Ptr<KThreadCB> initThread = ptr_new_cast(gk_InitThread);

    gk_InitThread->InitializeStack(init_thread_entry, /*privileged*/ true, /*skipEntryTrampoline*/ false, (void*)mainThreadStackSize);

    gk_ThreadTable.AllocHandle(initThreadHandle);
    gk_InitThread->SetHandle(initThreadHandle);
    gk_ThreadTable.Set(initThreadHandle, initThread);
    add_thread_to_ready_list(gk_InitThread);

#if defined(STM32H7)
    __set_BASEPRI(0);
#elif defined(STM32G030xx)
    __set_PRIMASK(0);
#else
#error Unknown platform.
#endif
    __ISB();

    SysTick->LOAD = coreFrequency / 1000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    start_first_thread();

    panic("Failed to launch first thread!\n");
    for (;;);
}

} // namespace kernel
