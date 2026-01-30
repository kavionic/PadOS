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

class PEventHandler;
class PEventTimer;

class PLooper : public PThread
{
public:
    PLooper(const PString& name, int portSize, size_t receiveBufferSize = 512);
    ~PLooper();

    static PLooper* GetCurrentLooper() { return dynamic_cast<PLooper*>(GetCurrentThread()); }

    void Stop() { m_DoRun = false; }

    PMutex& GetMutex() const { return m_Mutex; }

    PObjectWaitGroup& GetWaitGroup() { return m_WaitGroup; }

    const PMessagePort& GetPort() const { return m_Port; }
    port_id GetPortID() const { return m_Port.GetHandle(); }

    status_t SetReceiveBufferSize(size_t size);
    size_t   GetReceiveBufferSize() const;

    bool    WaitForReply(handler_id replyHandler, int32_t replyCode);

    bool AddHandler(Ptr<PEventHandler> handler);
    bool RemoveHandler(Ptr<PEventHandler> handler);
    bool AddTimer(PEventTimer* timer, bool singleshot = false);
    bool RemoveTimer(PEventTimer* timer);

    Ptr<PEventHandler> FindHandler(handler_id handle) const;

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
    static std::vector<PLooper*> s_LooperList;
#endif
    mutable PMutex                           m_Mutex;
    PMessagePort                             m_Port;
    PConditionVariable                       m_TimerMapCondition;
    PObjectWaitGroup                         m_WaitGroup;

    std::vector<uint8_t>                        m_ReceiveBuffer;
    TimeValNanos                                m_NextEventTime = TimeValNanos::infinit;
    volatile std::atomic_bool                   m_DoRun;
    std::multimap<TimeValNanos, PEventTimer*>    m_TimerMap;
    std::map<handler_id, Ptr<PEventHandler>>     m_HandlerMap;
    std::vector<std::pair<int32_t,int32_t>>     m_WaitingCodes;
    static int32_t                              s_NextReplyToken;

    PLooper(const PLooper &) = delete;
    PLooper& operator=(const PLooper &) = delete;
};
