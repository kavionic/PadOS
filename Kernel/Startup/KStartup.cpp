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
#include <Threads/ThreadUserspaceState.h>

#include <Kernel/KThreadCB.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/KProcess.h>
#include <Kernel/KThread.h>
#include <Kernel/KPIDNode.h>
#include <Kernel/KTime.h>
#include <Kernel/KLogging.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KBlockCache.h>
#include <Kernel/VFS/Kpty.h>
#include <Kernel/HAL/STM32/RealtimeClock.h>
#include <Kernel/HAL/STM32/ResetAndClockControl.h>
#include <Kernel/DebugConsole/KDebugConsole.h>
#include <Kernel/DebugConsole/KSerialPseudoTerminal.h>
#include <Kernel/FSDrivers/BinFS/BinFS.h>

extern "C" void __libc_init_array(void);

extern uint8_t _idle_tls_start;
extern uint8_t _idle_tls_end;

namespace kernel
{

static uint8_t gk_FirstProcessBuffer[sizeof(KProcess)] __attribute__((aligned(8)));
static KProcess& gk_FirstProcess = *reinterpret_cast<KProcess*>(gk_FirstProcessBuffer);

static uint8_t gk_IdleThreadBuffer[sizeof(NoPtr<KThreadCB>)] __attribute__((aligned(8)));
static NoPtr<KThreadCB>& gk_IdleThreadInstance = *reinterpret_cast<NoPtr<KThreadCB>*>(gk_IdleThreadBuffer);
static uint8_t gk_InitThreadBuffer[sizeof(KThreadCB)] __attribute__((aligned(8)));

static uint8_t gk_IdleThreadStack[256] __attribute__((aligned(8)));
static uint8_t gk_InitThreadStack[32768] __attribute__((aligned(8)));

//static KDebugConsole gk_DebugConsole1(STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO);
//static KDebugConsole gk_DebugConsole2("/dev/com/udp0");
static KSerialPseudoTerminal g_SerialTerminal1(STDIN_FILENO);
static KSerialPseudoTerminal g_SerialTerminal2("/dev/com/udp0");

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void initialize_scheduler_statics()
{
    try
    {
        PThreadAttribs idleAttrs("idle", KTHREAD_PRIORITY_MIN, PThreadDetachState_Detached, sizeof(gk_IdleThreadStack));
        idleAttrs.StackAddress = gk_IdleThreadStack;

        gk_KernelProcess = &gk_FirstProcess;

        g_PIDMapMutexOpt.emplace("pidmap", PEMutexRecursionMode_RaiseError);
        g_PIDMapOpt.emplace();

        KScopedLock lock(g_PIDMapMutex);

        g_PIDMap[KTHREAD_ID_IDLE] = ptr_new<KPIDNode>(KTHREAD_ID_IDLE);
        g_PIDMap[KTHREAD_ID_INIT] = ptr_new<KPIDNode>(KTHREAD_ID_INIT);

        new((void*)gk_FirstProcessBuffer) KProcess(*g_PIDMap[KTHREAD_ID_INIT], "kernel");
        new((void*)gk_IdleThreadBuffer) NoPtr<KThreadCB>(0, ptr_new_cast(gk_KernelProcess), &idleAttrs, /*kernelThread*/ true, nullptr, &_idle_tls_start);

        // Set the idle thread as the current thread, so that when the init thread is
        // scheduled the initial context is dumped on the idle thread's task. The init
        // thread will then overwrite that context with the real context needed to enter
        // idle_thread_entry() when no other threads want's the CPU.

        gk_IdleThread = &gk_IdleThreadInstance;
        gk_CurrentThread = gk_IdleThread;

        __kernel_thread_data = gk_CurrentThread->m_KernelTLS;
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
            Ptr<KThreadCB> thread = kget_thread(gk_DebugWakeupThread);
            if (thread != nullptr) {
                wakeup_thread(*thread, true);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* main_thread_entry(void* argument)
{
    PThreadUserData* const threadData = gk_CurrentThread->m_ThreadUserData;

    gk_CurrentThread->SetName("main_thread");

    uint32_t control;
    __asm volatile ("mrs %0, CONTROL" : "=r"(control));
    control |= 1; // set nPRIV
    __asm volatile ("msr CONTROL, %0\n isb" :: "r"(control) : "memory");

    __app_definition.entry(threadData);

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

    // To avoid any special cases in the first context switch we allow the
    // context switch routine to dump the initial context on the idle-thread's
    // stack. To make the idle-thread do it's job when all other threads go
    // to sleep, we must initialize it's stack properly before that happens.

    size_t mainThreadStackSize = size_t(arguments);
    gk_IdleThread->InitializeStack(nullptr, idle_thread_entry, /*skipEntryTrampoline*/ true, nullptr);


    kset_real_time(RealtimeClock::GetClock(), false);

    KBlockCache::Initialize();

    ksetup_rootfs_trw();

    kregister_filesystem_trw("binfs", ptr_new<KBinFilesystem>());
    kregister_filesystem_trw("ptyfs", ptr_new<KPTYFilesystem>());

    kchdir_trw(KLocateFlag::None, "/");

    initialize_device_drivers();

    mkdir("/dev/pty", S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    mount("", "/dev/pty", "ptyfs", 0, nullptr, 0);

    g_SerialTerminal1.SetDeleteOnExit(false);
    g_SerialTerminal2.SetDeleteOnExit(false);

    g_SerialTerminal1.Setup();
    g_SerialTerminal2.Setup();

    PThreadUserData* mainThreadUserData = __app_definition.create_main_thread_user_data();
    PThreadAttribs attrs("main", 0, PThreadDetachState_Detached, mainThreadStackSize);

    attrs.StackSize     = mainThreadUserData->StackSize;
    attrs.StackAddress  = mainThreadUserData->StackBuffer;
   
    gk_MainThreadID = kthread_spawn_trw(&attrs, nullptr, mainThreadUserData, { KSpawnThreadFlag::Privileged, KSpawnThreadFlag::SpawnProcess }, nullptr, main_thread_entry, nullptr);

    for (;;)
    {
        KThreadList threadsToDelete;
        {
            KSchedulerLock slock;

            for (KThreadCB* zombie = gk_ZombieThreadLists.GetFirst(); zombie != nullptr; zombie = gk_ZombieThreadLists.GetFirst())
            {
                gk_ZombieThreadLists.Remove(zombie);
                threadsToDelete.Append(zombie);
            }
        }

        for (KThreadCB* zombie = threadsToDelete.GetFirst(); zombie != nullptr; zombie = threadsToDelete.GetFirst())
        {
            threadsToDelete.Remove(zombie);
            zombie->SetState(ThreadState_Deleted);

            p_thread_reaper_schedule_cleanup(zombie->m_ThreadUserData);

            try
            {
                pid_t pid = zombie->GetHandle();
                const Ptr<KPIDNode> pidNode = kget_pid_node(pid);
                if (kensure(pidNode != nullptr))
                {
                    pidNode->Thread = nullptr;
                    kerase_pid_node_if_empty(pid);
                }
            }
            catch (const std::exception& exc)
            {
                kernel_log<PLogSeverity::CRITICAL>(LogCatKernel_Scheduler, "{}: failed to free zombie thread handle: {}", __PRETTY_FUNCTION__, exc.what());
            }
        }

        // Reap zombie child processes reparented to init.
        {
            siginfo_t processInfo = {};
            while (kwaitid(P_ALL, 0, &processInfo, WEXITED | WNOHANG) == PErrorCode::Success && processInfo.si_pid != 0) {
                processInfo = {};
            }
        }

        {
            KSchedulerLock slock;

            if (gk_ZombieThreadLists.GetFirst() == nullptr)
            {
                thread->SetState(ThreadState_Waiting);

                KSWITCH_CONTEXT();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void start_scheduler(size_t mainThreadStackSize)
{
    NVIC_SetPriority(PendSV_IRQn, KIRQ_PRI_KERNEL);
    NVIC_SetPriority(SysTick_IRQn, KIRQ_PRI_KERNEL);
    NVIC_SetPriority(SVCall_IRQn, KIRQ_PRI_LOW_LATENCY_MAX);
    
    {
        KScopedLock lock(g_PIDMapMutex);

        gk_IdleThread->SetState(ThreadState_Running);


        g_PIDMap[KTHREAD_ID_IDLE]->Thread = gk_IdleThreadInstance;

        PThreadAttribs attrs("init", 0, PThreadDetachState_Detached, sizeof(gk_InitThreadStack));
        attrs.StackAddress = gk_InitThreadStack;

        gk_InitThread = new((void*)gk_InitThreadBuffer)KThreadCB(KTHREAD_ID_INIT, ptr_tmp_cast(gk_KernelProcess), &attrs, /*kernelThread*/ true, nullptr, &_idle_tls_start);
        Ptr<KThreadCB> initThread = ptr_new_cast(gk_InitThread);

        gk_InitThread->InitializeStack(nullptr, init_thread_entry, /*skipEntryTrampoline*/ false, (void*)mainThreadStackSize);

        g_PIDMap[KTHREAD_ID_INIT]->Thread = initThread;
    }
    {
        KSchedulerLock lock;
        add_thread_to_ready_list(gk_InitThread);
    }
#if defined(STM32H7)
    __set_BASEPRI(0);
#elif defined(STM32G030xx)
    __set_PRIMASK(0);
#else
#error Unknown platform.
#endif
    __ISB();

    SysTick->LOAD = ResetAndClockControl::GetSysClockFrequency() / 1000 - 1;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;

    start_first_thread();

    panic("Failed to launch first thread!\n");
    for (;;);
}


} // namespace kernel
