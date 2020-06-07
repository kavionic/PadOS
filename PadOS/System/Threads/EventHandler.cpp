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

#include "Threads/EventHandler.h"
#include "Signals/RemoteSignal.h"

using namespace os;

EventHandler::EventHandler(const String& name) : m_Name(name)
{
    static handler_id nextHandle = 0;
    m_Handle = ++nextHandle;
}

EventHandler::~EventHandler()
{
}

bool EventHandler::HandleMessage(int32_t code, const void* data, size_t length)
{
    auto i = m_RemoteSignalMap.find(code);
    if (i != m_RemoteSignalMap.end()) {
//        printf("EventHandler::HandleMessage() '%s' handline event %d (%d)\n", m_Name.c_str(), code, length);
        i->second->Dispatch(data, length);
        return true;
    }
    return false;
}
