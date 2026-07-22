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
