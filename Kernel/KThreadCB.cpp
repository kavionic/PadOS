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
// Created: 04.03.2018 19:30:41

#include "System/Platform.h"

#include <algorithm>
#include <map>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <malloc.h>
#include <sys/pados_syscalls.h>

#include <Kernel/KProcess.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/KStackFrames.h>
#include <Kernel/Scheduler.h>
#include <Kernel/ThreadSyncDebugTracker.h>
#include <Kernel/Syscalls.h>
#include <System/AppDefinition.h>
#include <System/ModuleTLSDefinition.h>
#include <Threads/ThreadUserspaceState.h>
#include <Utils/Utils.h>


extern uint32_t __tdata_start;
extern uint32_t __tdata_end;
extern uint32_t __tbss_start;
extern uint32_t __tbss_end;

extern uint8_t __tdata_size;
extern uint8_t __tbss_size;
extern uint8_t __tls_align;

SECTION_KERNEL_IMAGE_DEFINITION PFirmwareImageDefinition _kerneldef =
{
    .entry                          = nullptr,
    .process_entry_trampoline       = nullptr,
    .thread_terminated              = nullptr,
    .signal_trampoline              = nullptr,
    .signal_terminate_thread        = nullptr,
    .create_main_thread_user_data   = nullptr,
    .create_thread_user_data        = nullptr,
    .alloc_memory                   = nullptr,
    .free_memory                    = nullptr,
    .FirstAppPointer = PAppDefinition::s_FirstApp,
    .ThreadReaperQueue = nullptr,
    .TLSDefinition =
    {
        .TLSData = &__tdata_start,
        .TLSBSS = &__tbss_start,
        .TLSDataSize = uint32_t(&__tdata_size),
        .TLSBSSSize = uint32_t(&__tbss_size),
        .TLSAlign = uint32_t(&__tls_align)
    }
};

__attribute__((section(".kerneltls")))
PThreadControlBlock* __kernel_thread_data;

namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void thread_entry_point(PThreadUserData* threadData, void* (*threadEntry)(void*), void* arguments)
{
    try
    {
        void* const result = threadEntry(arguments);
        ksys_thread_terminate(result);
    }
    catch (const std::exception& exc)
    {
        const PString error = PString("Uncaught exception in thread '") + gk_CurrentThread->GetName() + "': " + exc.what();
        panic(error.c_str());
        ksys_thread_terminate((void*)-1);
    }
    PRETHROW_CANCELLATION
    catch (...)
    {
        panic("Uncaught exception from thread.\n");
        ksys_thread_terminate((void*)-1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void invalid_return_handler()
{
    panic("Invalid return from thread\n");
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThreadCB::KThreadCB(thread_id handle, Ptr<KProcess> process, const PThreadAttribs* attribs, bool kernelThread, PThreadUserData* threadUserData, void* kernelTLSMemory)
    : KNamedObject((attribs != nullptr && attribs->Name != nullptr) ? attribs->Name : "", KNamedObjectType::Thread)
    , m_KernelThread(kernelThread)
{
    SetHandle(handle);

    const int priority = (attribs != nullptr) ? attribs->Priority : 0;
    m_DetachState = (attribs != nullptr) ? attribs->DetachState : PThreadDetachState_Detached;
    m_StackSize = (attribs != nullptr && attribs->StackSize != 0) ? (attribs->StackSize & ~(KSTACK_ALIGNMENT - 1)) : THREAD_DEFAULT_STACK_SIZE;
    void* stackAddress = (attribs != nullptr) ? attribs->StackAddress : nullptr;

    if (stackAddress == nullptr)
    {
        m_StackBuffer = new uint8_t[m_StackSize + KSTACK_ALIGNMENT];
        memset(m_StackBuffer, THREAD_STACK_FILLER, m_StackSize + KSTACK_ALIGNMENT);
        m_FreeStackOnExit = true;
    }
    else
    {
        m_StackBuffer = reinterpret_cast<uint8_t*>(stackAddress);
        memset(m_StackBuffer, THREAD_STACK_FILLER, m_StackSize);
        m_FreeStackOnExit = false;
    }

    m_CurrentStackAndPrivilege = (intptr_t(m_StackBuffer) - 4 + m_StackSize) & ~(KSTACK_ALIGNMENT - 1);
    m_ThreadState = ThreadState_Ready;
    m_PriorityLevel = PriToLevel(priority);
    m_ThreadUserData = threadUserData;

    if (m_ThreadUserData != nullptr) {
        m_UserspaceTLS = m_ThreadUserData->TLSData;
    }

    try
    {
        SetupTLS(attribs, kernelTLSMemory);
    }
    catch (...)
    {
        if (m_FreeStackOnExit) {
            delete[] m_StackBuffer;
        }
        throw;
    }
    process->AddThread(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KThreadCB::~KThreadCB()
{
    while (m_FirstQueuedSignal != nullptr)
    {
        KSignalQueueNode* node = m_FirstQueuedSignal;
        m_FirstQueuedSignal = node->Next;
        kfree_signal_queue_node(node);
    }

    kassert(m_Process == nullptr);
    if (m_Process != nullptr) {
        m_Process->RemoveThread(this);
    }
    if (m_FreeStackOnExit) {
        delete[] m_StackBuffer;
    }
    if (m_FreeTLSOnExit && m_KernelTLS != nullptr) {
        free(m_KernelTLS);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KThreadCB::IsMainThread() const noexcept
{
    return GetHandle() == m_Process->GetPID();
}

///////////////////////////////////////////////////////////////////////////////
/// Setup a stack frame identical to the one produced during a context switch.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KThreadCB::InitializeStack(ThreadEntryTrampoline_t entryTrampoline, ThreadEntryPoint_t entryPoint, bool skipEntryTrampoline, void* arguments)
{
    uint32_t currentStack = ((intptr_t(m_StackBuffer) - 4 + m_StackSize) & ~(KSTACK_ALIGNMENT - 1));

    KCtxSwitchStackFrame* stackFrame = reinterpret_cast<KCtxSwitchStackFrame*>(currentStack) - 1;

    memset(stackFrame, 0, sizeof(*stackFrame));
    stackFrame->KernelFrame.EXEC_RETURN = 0xfffffffd; // Return to Thread mode, exception return uses non-floating-point state from the PSP and execution uses PSP after return
    if (!skipEntryTrampoline) [[likely]]
    {
        stackFrame->ExceptionFrame.R0 = uintptr_t(m_ThreadUserData);
        stackFrame->ExceptionFrame.R1 = uintptr_t(entryPoint);
        stackFrame->ExceptionFrame.R2 = uintptr_t(arguments);
        if (entryTrampoline != nullptr) {
            stackFrame->ExceptionFrame.PC = uintptr_t(entryTrampoline) & ~1; // Clear the thumb flag from the function pointer.
        } else {
            stackFrame->ExceptionFrame.PC = uintptr_t(thread_entry_point) & ~1; // Clear the thumb flag from the function pointer.
        }
    }
    else
    {
        stackFrame->ExceptionFrame.R0 = uintptr_t(arguments);
        stackFrame->ExceptionFrame.PC = uintptr_t(entryPoint) & ~1; // Clear the thumb flag from the function pointer.
    }
    stackFrame->ExceptionFrame.LR = uint32_t(invalid_return_handler) &~1; // Clear the thumb flag from the function pointer.
    stackFrame->ExceptionFrame.xPSR = xPSR_T_Msk; // Always in Thumb state.

    m_CurrentStackAndPrivilege = reinterpret_cast<intptr_t>(stackFrame);

#ifndef PADOS_OPT_PRIVILEGED_USERSPACE_THREADS
    if (!m_KernelThread) {
        m_CurrentStackAndPrivilege |= 0x01;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KThreadCB::SetState(ThreadState state) noexcept
{
    KSchedulerLock lock;

    if (state != m_ThreadState)
    {
        const ThreadState prevState = m_ThreadState;
        m_ThreadState = state;

        if (prevState == ThreadState_Stopped) {
            m_Process->ThreadContinued();
        } else if (state == ThreadState_Stopped) {
            m_Process->ThreadStopped();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KThreadCB::PriToLevel(int priority) noexcept
{
    int level = priority - KTHREAD_PRIORITY_MIN;
    return std::clamp(level, 0, KTHREAD_PRIORITY_LEVELS - 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KThreadCB::LevelToPri(int level) noexcept
{
    return level + KTHREAD_PRIORITY_MIN;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KThreadCB::SetBlockingObject(const KNamedObject* waitObject) noexcept
{
    if (m_BlockingObject != nullptr)
    {
        ThreadSyncDebugTracker::GetInstance().RemoveThread(this);
    }
    m_BlockingObject = waitObject;
    if (m_BlockingObject != nullptr)
    {
        ThreadSyncDebugTracker::GetInstance().AddThread(this, m_BlockingObject);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KThreadCB::SetupTLS(const PThreadAttribs* attribs, void* kernelTLSMemory)
{
    m_KernelTLS = create_thread_tls_block(__kernel_definition, kernelTLSMemory);
    if (m_KernelTLS == nullptr) {
        throw std::bad_alloc();
    }
    m_FreeTLSOnExit = kernelTLSMemory == nullptr;
}


} // namespace kernel
