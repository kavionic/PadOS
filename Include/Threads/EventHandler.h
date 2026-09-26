// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 11.03.2018 16:01:50

#pragma once

#include "Signals/RemoteSignal.h"
#include "Ptr/PtrTarget.h"
#include "Utils/String.h"


class PLooper;


class PEventHandler : public PtrTarget
{
public:
    PEventHandler(const PString& name);
    virtual ~PEventHandler();

    const PString& GetName() const { return m_Name; }
    void SetName(const PString& name) { m_Name = name; }

    handler_id GetHandle() const { return m_Handle; }

    PLooper* GetLooper() const { return m_Looper; }

    virtual bool HandleMessage(int32_t code, const void* data, size_t length);

    template<typename SIGNAL, typename CALLBACK>
    void RegisterRemoteSignal(SIGNAL* signal, CALLBACK callback)
    {
        m_RemoteSignalRegistry.Register(signal, this, callback);
    }

    template<typename SIGNAL, typename CALLBACK>
    void UnregisterRemoteSignal(CALLBACK callback)
    {
        m_RemoteSignalRegistry.Unregister<SIGNAL>(this, callback);
    }

    PRemoteSignalRXBase* GetSignalForMessage(int32_t code)
    {
        return m_RemoteSignalRegistry.GetSignal(code);
    }

private:
    friend class PLooper;

    PString    m_Name;

    PLooper*    m_Looper = nullptr;
    handler_id m_Handle;

    PRemoteSignalRegistry m_RemoteSignalRegistry;
    
    PEventHandler(const PEventHandler &) = delete;
    PEventHandler& operator=(const PEventHandler &) = delete;
};
