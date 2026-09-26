// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
#include <System/ExceptionHandling.h>
#include <Utils/Logging.h>
#include <Utils/Utils.h>
#include <Kernel/KThreadCB.h>


thread_local PThread* PThread::st_CurrentThread = nullptr;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThread::PThread(const PString& name) : m_Name(name)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThread::~PThread()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PThread* PThread::GetCurrentThread()
{
    return st_CurrentThread;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode PThread::Start(PThreadDetachState detachState, int priority, int stackSize)
{
    if (m_ThreadHandle == INVALID_HANDLE)
    {
        m_DetachState = detachState;
        PThreadAttribs attrs(m_Name.c_str(), priority, detachState, stackSize);
        return thread_spawn(&m_ThreadHandle, &attrs, ThreadEntry, this);
    }
    return PErrorCode::BUSY;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode PThread::Join(void** outReturnValue, TimeValNanos deadline)
{
    PErrorCode result = PErrorCode::INVAL;
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

void* PThread::Run()
{
    if (!VFRun.Empty()) {
        return VFRun(this);
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void PThread::Exit(void* returnValue)
{
    if (m_DetachState == PThreadDetachState_Detached && m_DeleteOnExit) {
        delete this;
    }
    thread_exit(returnValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* PThread::ThreadEntry(void* data)
{
    PThread* self = static_cast<PThread*>(data);

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
    PRETHROW_CANCELLATION
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

PErrorCode PThread::Adopt()
{
    if (m_ThreadHandle == INVALID_HANDLE)
    {
        m_ThreadHandle = get_thread_id();
//        m_DetachState = detachState;
        ThreadEntry(this);

        return PErrorCode::Success;
    }
    return PErrorCode::BUSY;

}

