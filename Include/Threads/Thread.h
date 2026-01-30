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

struct PFirmwareImageDefinition;

class PThread
{
public:
    PThread(const PString& name);
    virtual ~PThread();

    static PThread* GetCurrentThread();

    const PString& GetName() const { return m_Name; }

    void SetDeleteOnExit(bool doDelete) { m_DeleteOnExit = doDelete; }
    bool GetDeleteOnExit() const { return m_DeleteOnExit; }

    PErrorCode  Start(PThreadDetachState detachState = PThreadDetachState_Detached, int priority = 0, int stackSize = 0);
    PErrorCode  Join(void** outReturnValue, TimeValNanos deadline = TimeValNanos::infinit);

    PErrorCode Adopt();

    bool IsRunning() const { return m_ThreadHandle != INVALID_HANDLE; }
    thread_id GetThreadID() const { return m_ThreadHandle; }

    virtual void* Run();
    
    void Exit(void* returnValue);

    VFConnector<void*, PThread*> VFRun;
private:
    static void* ThreadEntry(void* data);

    static thread_local PThread* st_CurrentThread;

    PString             m_Name;
    thread_id           m_ThreadHandle = INVALID_HANDLE;
    PThreadDetachState  m_DetachState = PThreadDetachState_Detached;
    bool                m_DeleteOnExit = true;

    PThread(const PThread &) = delete;
    PThread& operator=(const PThread &) = delete;
};

PThreadControlBlock* create_thread_tls_block(const PFirmwareImageDefinition& imageDefinition, void* buffer = nullptr);
void delete_thread_tls_block(PThreadControlBlock* tlsBlock);
