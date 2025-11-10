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

#include <Threads/Thread.h>
#include <Threads/Threads.h>
#include <System/System.h>
#include <Utils/Logging.h>

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
        p_system_log(kernel::LogCatKernel_Scheduler, PLogSeverity::FATAL, "Uncaught exception in thread {}: {}", self->GetName(), e.what());
        self->Exit(nullptr);
    }
    catch (...)
    {
        p_system_log(kernel::LogCatKernel_Scheduler, PLogSeverity::FATAL, "Uncaught exception in thread {}: unknown", self->GetName());
        self->Exit(nullptr);
    }
    return nullptr;
}
