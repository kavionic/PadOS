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

#include "Threads/Thread.h"
#include "Threads/Threads.h"
#include "System/System.h"

using namespace os;
using namespace kernel;

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
    return static_cast<Thread*>(get_thread_local(GetThreadObjTLSSlot()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

thread_id Thread::Start(bool joinable, int priority, int stackSize)
{
    if (m_ThreadHandle == -1) {
        m_IsJoinable = joinable;
        m_ThreadHandle = spawn_thread(m_Name.c_str(), ThreadEntry, priority, this, joinable, stackSize);
    }
    return m_ThreadHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Thread::Wait(TimeValMicros timeout)
{
    if (m_IsJoinable)
    {
        int result = wait_thread(m_ThreadHandle);
        if (result >= 0 && m_DeleteOnExit) {
            delete this;
        }
        return result;
    }
    set_last_error(EINVAL);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Thread::Run()
{
    if (!VFRun.Empty()) {
        return VFRun(this);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////
    
void Thread::Exit(int returnCode)
{
    if (!m_IsJoinable && m_DeleteOnExit) {
        delete this;
    }
    exit_thread(returnCode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Thread::ThreadEntry(void* data)
{
    Thread* self = static_cast<Thread*>(data);
    try
    {
        set_thread_local(GetThreadObjTLSSlot(), data);
        self->Exit(self->Run());
    }
    catch(const std::exception& e)
    {
        kernel_log(LogCatKernel_Scheduler, KLogSeverity::FATAL, "Uncaught exception in thread %s: %s", self->GetName().c_str(), e.what());
        self->Exit(ENOTRECOVERABLE);
    }
    catch (...)
    {
        kernel_log(LogCatKernel_Scheduler, KLogSeverity::FATAL, "Uncaught exception in thread %s: unknown", self->GetName().c_str());
        self->Exit(ENOTRECOVERABLE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Thread::GetThreadObjTLSSlot()
{
    static int slot = alloc_thread_local_storage(nullptr);
    return slot;
}
