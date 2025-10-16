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

#pragma once


#include <sys/pados_types.h>
#include "Signals/VFConnector.h"
#include "System/System.h"
#include "Utils/String.h"

namespace os
{

class Thread
{
public:
    Thread(const String& name);
    virtual ~Thread();

    static Thread* GetCurrentThread();

    const String& GetName() const { return m_Name; }

    void SetDeleteOnExit(bool doDelete) { m_DeleteOnExit = doDelete; }
    bool GetDeleteOnExit() const { return m_DeleteOnExit; }

    PErrorCode  Start(PThreadDetachState detachState = PThreadDetachState_Detached, int priority = 0, int stackSize = 0);
    PErrorCode  Join(void** outReturnValue, TimeValNanos deadline = TimeValNanos::infinit);

    bool IsRunning() const { return m_ThreadHandle != INVALID_HANDLE; }
    thread_id GetThreadID() const { return m_ThreadHandle; }

    virtual void* Run();
    
    void Exit(void* returnValue);

    VFConnector<void*, Thread*> VFRun;
private:
    static void* ThreadEntry(void* data);

    static thread_local Thread* st_CurrentThread;

    String              m_Name;
    thread_id           m_ThreadHandle = INVALID_HANDLE;
    PThreadDetachState  m_DetachState = PThreadDetachState_Detached;
    bool                m_DeleteOnExit = true;

    Thread(const Thread &) = delete;
    Thread& operator=(const Thread &) = delete;
};

} // namespace
