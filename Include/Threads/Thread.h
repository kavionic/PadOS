// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.03.2018 13:10:28

#pragma once


#include <sys/pados_types.h>
#include "Signals/VFConnector.h"
#include "System/System.h"
#include "Utils/String.h"


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
