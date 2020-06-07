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

#include <map>

#include "Signals/Signal.h"
#include "Ptr/PtrTarget.h"
#include "Utils/String.h"

namespace os
{
    
class Looper;
class RemoteSignalRXBase;

class EventHandler : public PtrTarget
{
public:
    EventHandler(const String& name);
    virtual ~EventHandler();

    const String& GetName() const { return m_Name; }

    handler_id GetHandle() const { return m_Handle; }

    Looper* GetLooper() { return m_Looper; }

    virtual bool HandleMessage(int32_t code, const void* data, size_t length);

    template<typename SIGNAL, typename CALLBACK>
    void RegisterRemoteSignal(SIGNAL* signal, CALLBACK callback)
    {
        m_RemoteSignalMap[signal->GetID()] = signal;
        signal->Connect(this, callback);
    }
    RemoteSignalRXBase* GetSignalForMessage(int32_t code) {
        auto i = m_RemoteSignalMap.find(code);
        if (i != m_RemoteSignalMap.end()) {
            return i->second;
        }
        return nullptr;            
    }
//    Signal<void, int, const void*> SignalMessageReceived;
private:
    friend class Looper;

    String     m_Name;

    Looper*    m_Looper = nullptr;
    handler_id m_Handle;
    
    std::map<int, RemoteSignalRXBase*> m_RemoteSignalMap;
    
    EventHandler(const EventHandler &) = delete;
    EventHandler& operator=(const EventHandler &) = delete;
};

} // namespace