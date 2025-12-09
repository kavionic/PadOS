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
// Created: 11.03.2018 13:10:28

#include <cstdlib>
#include <sys/reent.h>
#include <sys/pados_syscalls.h>
#include <PadOS/Threads.h>
#include <Threads/Thread.h>
#include <Threads/Threads.h>
#include <System/AppDefinition.h>
#include <System/System.h>
#include <Utils/Logging.h>
#include <Utils/Utils.h>
#include <Kernel/KThreadCB.h>

using namespace os;

thread_local Thread* Thread::st_CurrentThread = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Thread::Thread(const String& name) : m_Name(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Thread::~Thread()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Thread* Thread::GetCurrentThread()
{
    return st_CurrentThread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode Thread::Start(PThreadDetachState detachState, int priority, int stackSize)
{
    if (m_ThreadHandle == INVALID_HANDLE)
    {
        m_DetachState = detachState;
        PThreadAttribs attrs(m_Name.c_str(), priority, detachState, stackSize);
        return thread_spawn(&m_ThreadHandle, &attrs, ThreadEntry, this);
    }
    return PErrorCode::Busy;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode Thread::Join(void** outReturnValue, TimeValNanos deadline)
{
    PErrorCode result = PErrorCode::InvalidArg;
    if (m_DetachState == PThreadDetachState_Joinable)
    {
        result = thread_join(m_ThreadHandle, outReturnValue);
        if (result == PErrorCode::Success && m_DeleteOnExit) {
            delete this;
        }
        return result;
    }
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* Thread::Run()
{
    if (!VFRun.Empty()) {
        return VFRun(this);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void Thread::Exit(void* returnValue)
{
    if (m_DetachState == PThreadDetachState_Detached && m_DeleteOnExit) {
        delete this;
    }
    thread_exit(returnValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* Thread::ThreadEntry(void* data)
{
    Thread* self = static_cast<Thread*>(data);

    try
    {
        st_CurrentThread = self;
        self->Exit(self->Run());
    }
    catch(const std::exception& e)
    {
        p_system_log<PLogSeverity::FATAL>(LogCat_Threads, "Uncaught exception in thread {}: {}", self->GetName(), e.what());
        self->Exit(nullptr);
    }
    catch (...)
    {
        p_system_log<PLogSeverity::FATAL>(LogCat_Threads, "Uncaught exception in thread {}: unknown", self->GetName());
        self->Exit(nullptr);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* spawn_thread_entry_function(void* arguments)
{
    __app_thread_data = reinterpret_cast<PThreadControlBlock*>(arguments);

    void** controlBlock = reinterpret_cast<void**>(arguments);

    ThreadEntryPoint_t  threadEntry     = reinterpret_cast<ThreadEntryPoint_t>(controlBlock[0]);
    void*               threadArguments = controlBlock[1];

    memcpy(__app_thread_data + 1, __app_definition.TLSDefinition.TLSData, __app_definition.TLSDefinition.TLSDataSize);

    void* result = threadEntry(threadArguments);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void __thread_terminated(thread_id threadID, void* stackBuffer, PThreadControlBlock* threadLocalBuffer)
{
    _reclaim_reent(nullptr);

    ThreadLocalSlotManager::Get().ThreadTerminated();

    if (threadLocalBuffer != nullptr) {
        free(threadLocalBuffer);
    }
}

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode thread_spawn(thread_id* outHandle, const PThreadAttribs* attribs, ThreadEntryPoint_t entryPoint, void* arguments)
{
    const size_t controlBlockSize = sizeof(PThreadControlBlock) + __app_definition.TLSDefinition.TLSDataSize + __app_definition.TLSDefinition.TLSBSSSize;
    assert(__app_definition.TLSDefinition.TLSAlign <= sizeof(PThreadControlBlock));
    
    void** controlBlock = reinterpret_cast<void**>(aligned_alloc(__app_definition.TLSDefinition.TLSAlign, align_up(controlBlockSize, __app_definition.TLSDefinition.TLSAlign)));
    if (controlBlock == nullptr) {
        return PErrorCode::NoMemory;
    }
    memset(controlBlock, 0, controlBlockSize);
    controlBlock[0] = reinterpret_cast<void*>(entryPoint);
    controlBlock[1] = arguments;
    return __thread_spawn(outHandle, attribs, spawn_thread_entry_function, controlBlock);
}


#ifdef __cplusplus
}
#endif

