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
///////////////////////////////////////////////////////////////////////////////
// Created: 09.01.2026 23:00

#include <Ptr/NoPtr.h>

#include <System/AppDefinition.h>

#include <Kernel/KThreadCB.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/KProcess.h>
#include <Kernel/KThread.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KBlockCache.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <Kernel/DebugConsole/KDebugConsole.h>

extern "C" void __libc_init_array(void);

extern uint8_t _idle_tls_start;
extern uint8_t _idle_tls_end;

namespace kernel
{

alignas(KHandleArray<KThreadCB>) std::byte              gk_ThreadTableBuffer[sizeof(KHandleArray<KThreadCB>)];

static uint8_t gk_InitialBlocksBuffer[2][sizeof(KHandleArrayBlock)] __attribute__((aligned(8)));
static Ptr<KHandleArrayBlock> gk_InitialBlocks[2] __attribute__((aligned(8)));


static uint8_t gk_FirstProcessBuffer[sizeof(KProcess)] __attribute__((aligned(8)));
static KProcess& gk_FirstProcess = *reinterpret_cast<KProcess*>(gk_FirstProcessBuffer);

static uint8_t gk_IdleThreadBuffer[sizeof(NoPtr<KThreadCB>)] __attribute__((aligned(8)));
static NoPtr<KThreadCB>& gk_IdleThreadInstance = *reinterpret_cast<NoPtr<KThreadCB>*>(gk_IdleThreadBuffer);
static uint8_t gk_InitThreadBuffer[sizeof(KThreadCB)] __attribute__((aligned(8)));

static uint8_t gk_IdleThreadStack[256] __attribute__((aligned(8)));
static uint8_t gk_InitThreadStack[32768] __attribute__((aligned(8)));


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void initialize_scheduler_statics()
{
    try
    {
        PThreadAttribs idleAttrs("idle", KTHREAD_PRIORITY_MIN, PThreadDetachState_Detached, sizeof(gk_IdleThreadStack));
        idleAttrs.StackAddress = gk_IdleThreadStack;

        gk_CurrentProcess = &gk_FirstProcess;

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

    kchdir_trw("/");

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

    KDebugConsole::Get().Setup();

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
