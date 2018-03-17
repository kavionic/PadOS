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
// Created: 11.03.2018 13:10:28

#include "Thread.h"
#include "System/Threads.h"
#include "System/System.h"


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Thread::Thread()
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

thread_id Thread::Start(const char* name, bool joinable, int priority, int stackSize)
{
    if (m_ThreadHandle == -1) {
        m_IsJoinable = joinable;
        m_ThreadHandle = spawn_thread(name, ThreadEntry, priority, this, joinable, stackSize);
    }
    return m_ThreadHandle;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Thread::Wait(bigtime_t timeout)
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
    static_cast<Thread*>(data)->Exit(static_cast<Thread*>(data)->Run());
}
