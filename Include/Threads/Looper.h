// This file is part of PadOS.
//
// Copyright (C) 2018-2021 Kurt Skauen <http://kavionic.com/>
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
#include "Mutex.h"
#include "Ptr/Ptr.h"
#include "Utils/MessagePort.h"
#include "ConditionVariable.h"
#include "ObjectWaitGroup.h"

namespace os
{

class EventHandler;
class EventTimer;

class Looper : public Thread
{
public:
    Looper(const String& name, int portSize, size_t receiveBufferSize = 512);
    ~Looper();

    static Looper* GetCurrentLooper() { return dynamic_cast<Looper*>(GetCurrentThread()); }

    void Stop() { m_DoRun = false; }

    Mutex& GetMutex() const { return m_Mutex; }

    MessagePort GetPort() const { return m_Port; }
    port_id GetPortID() const { return m_Port.GetHandle(); }

    status_t SetReceiveBufferSize(size_t size);
    size_t   GetReceiveBufferSize() const;

    bool    WaitForReply(handler_id replyHandler, int32_t replyCode);

    bool AddHandler(Ptr<EventHandler> handler);
    bool RemoveHandler(Ptr<EventHandler> handler);
    bool AddTimer(EventTimer* timer, bool singleshot = false);
    bool RemoveTimer(EventTimer* timer);

    Ptr<EventHandler> FindHandler(handler_id handle) const;

    virtual void ThreadStarted() {}
    virtual bool HandleMessage(handler_id targetHandler, int32_t code, const void* data, size_t length) { return false; }
    virtual void Idle() {}
        
    virtual void* Run() override;
    
    bool ProcessEvents();
    bool Tick();

private:
    void ProcessMessage(handler_id targetHandler, int32_t code, ssize_t msgLength);
    void RunTimers();

#if DEBUG_LOOPER_LIST
    static std::vector<Looper*> s_LooperList;
#endif
    kernel::KThreadCB* m_Thread = nullptr;
    mutable Mutex                           m_Mutex;
    MessagePort                             m_Port;
    ConditionVariable                       m_TimerMapCondition;
    ObjectWaitGroup                         m_WaitGroup;

    std::vector<uint8_t>                        m_ReceiveBuffer;
    TimeValMicros                               m_NextEventTime = TimeValMicros::infinit;
    volatile std::atomic_bool                   m_DoRun;
    std::multimap<TimeValMicros, EventTimer*>   m_TimerMap;
    std::map<handler_id, Ptr<EventHandler>>     m_HandlerMap;
    std::vector<std::pair<int32_t,int32_t>>     m_WaitingCodes;
    static int32_t                              s_NextReplyToken;

    Looper(const Looper &) = delete;
    Looper& operator=(const Looper &) = delete;
};

} // namespace