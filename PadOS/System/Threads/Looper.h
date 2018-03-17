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
// Created: 11.03.2018 16:00:25

#pragma once

#include <map>
#include <vector>
#include <atomic>

#include "Thread.h"
#include "Semaphore.h"
#include "System/Ptr/Ptr.h"
#include "System/Utils/MessagePort.h"

class EventHandler;
class EventTimer;

class Looper : public Thread
{
public:
    Looper(int portSize);
    ~Looper();

    void Stop() { m_DoRun = false; }

    MessagePort GetPort() const { return m_Port; }
    port_id GetPortID() const { return m_Port.GetPortID(); }
        
    bool AddHandler(Ptr<EventHandler> handler);
    bool AddTimer(EventTimer* timer, bool singleshot = false);

    virtual int Run() override;
private:
    bigtime_t RunTimers();

    Semaphore m_Mutex;
    MessagePort m_Port;
    bigtime_t                             m_NextEventTime = 0;
    volatile std::atomic_bool             m_DoRun;
    std::multimap<bigtime_t, EventTimer*> m_TimerMap;
    std::vector<Ptr<EventHandler>>        m_HandlerList;

    Looper(const Looper &) = delete;
    Looper& operator=(const Looper &) = delete;
};
