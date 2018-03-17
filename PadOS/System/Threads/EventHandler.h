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

#include "System/Signals/Signal.h"
#include "System/Ptr/PtrTarget.h"

class Looper;

class EventHandler : public PtrTarget
{
public:
    EventHandler();
    virtual ~EventHandler();

    virtual bool HandleMessage(int32_t code, const void* data, size_t length) { SignalMessageReceived(code, data); return true; }

    Signal<void, int, const void*> SignalMessageReceived;
private:
    friend class Looper;

    Looper* m_Looper = nullptr;

    EventHandler(const EventHandler &) = delete;
    EventHandler& operator=(const EventHandler &) = delete;
};

