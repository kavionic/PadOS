// This file is part of PadOS.
//
// Copyright (C) 2018-2024 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include <PadOS/Time.h>
#include <Threads/Looper.h>
#include <Threads/EventHandler.h>
#include <Threads/EventTimer.h>
#include <System/SystemMessageIDs.h>
#include <Kernel/Scheduler.h>

using namespace os;

#if DEBUG_LOOPER_LIST
std::vector<Looper*> Looper::s_LooperList;

static Mutex& GetLooperListMutex()
{
    static Mutex mutex("LooperList", PEMutexRecursionMode_RaiseError);
    return mutex;
}
#endif // DEBUG_LOOPER_LIST

int32_t Looper::s_NextReplyToken;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Looper::Looper(const String& name, int portSize, size_t receiveBufferSize) : Thread(name), m_Mutex("looper", PEMutexRecursionMode_RaiseError), m_Port("looper", portSize), m_DoRun(true)
{
#if DEBUG_LOOPER_LIST
    CRITICAL_BEGIN(GetLooperListMutex()) {
        s_LooperList.push_back(this);
    } CRITICAL_END;
#endif
    SetReceiveBufferSize(receiveBufferSize);

    m_WaitGroup.AddObject(m_Port);
    m_WaitGroup.AddObject(m_TimerMapCondition);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Looper::~Looper()
{
    for (auto& i : m_TimerMap)
    {
        i.second->m_Looper = nullptr;
    }
#if DEBUG_LOOPER_LIST
    CRITICAL_BEGIN(GetLooperListMutex()) {
        s_LooperList.erase(std::find(s_LooperList.begin(), s_LooperList.end(), this));
    } CRITICAL_END;
#endif
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t Looper::SetReceiveBufferSize(size_t size)
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    try {
        ssize_t oldSize = m_ReceiveBuffer.size();
        m_ReceiveBuffer.resize(size);
        return oldSize;
    } catch(const std::bad_alloc&) {
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t   Looper::GetReceiveBufferSize() const
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    return m_ReceiveBuffer.size();
}

/*int32_t Looper::AllocReplyToken()
{
    return m_NextReplyToken++;
}*/

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::WaitForReply(handler_id replyHandler, int32_t replyCode)
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    int32_t replyToken = s_NextReplyToken++;
    m_WaitingCodes.push_back(std::make_pair(replyCode, replyToken));
    bool stillWaiting = true;
    do
    {
        handler_id targetHandler;
        int32_t    code;
        
        ssize_t msgLength = m_Port.ReceiveMessage(&targetHandler, &code, m_ReceiveBuffer.data(), m_ReceiveBuffer.size());
        if (msgLength >= 0)
        {
            if (!m_WaitingCodes.empty() && code == m_WaitingCodes[0].first)
            {
//                int32_t removedToken = m_WaitingCodes[0].second;
                m_WaitingCodes.erase(m_WaitingCodes.begin());
//                if (removedToken == replyToken) {
//                    return true;
//                }
            }
            ProcessMessage(targetHandler, code, msgLength);
            stillWaiting = false;
            for (auto i = m_WaitingCodes.begin(); i != m_WaitingCodes.end(); ++i)
            {
                if (i->first == replyCode && i->second == replyToken)
                {
                    stillWaiting = true;
                    break;
                }
            }                    
        }            
    } while(stillWaiting);
    return true;
//    return ProcessEvents(replyHandler, replyCode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::AddHandler(Ptr<EventHandler> handler)
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    try
    {
        if (handler->m_Looper != nullptr)
        {
            printf("ERROR: Looper::AddHandler() attempt to add handler %s(%d) already owned by looper %s(%d)\n", handler->GetName().c_str(), handler->GetHandle(), handler->m_Looper->GetName().c_str(), handler->m_Looper->GetThreadID());
            set_last_error(EBUSY);
            return false;
        }
        m_HandlerMap[handler->m_Handle] = handler;
        handler->m_Looper = this;
        return true;
    }
    catch (const std::bad_alloc&) {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::RemoveHandler(Ptr<EventHandler> handler)
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    if (handler->m_Looper != this) {
        printf("ERROR: Looper::RemoveHandler() attempt to remove handler %s(%d) from unrelated looper %s(%d)\n", handler->GetName().c_str(), handler->GetHandle(), GetName().c_str(), GetThreadID());
        return false;
    }
    handler->m_Looper = nullptr;
    auto i = m_HandlerMap.find(handler->m_Handle);
    if (i != m_HandlerMap.end()) {
        m_HandlerMap.erase(i);
        return true;
    } else {
        printf("ERROR: Looper::RemoveHandler() failed to find handler %s(%d) in looper %s(%d)\n", handler->GetName().c_str(), handler->GetHandle(), GetName().c_str(), GetThreadID());        
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::AddTimer(EventTimer* timer, bool singleshot)
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    try
    {
        TimeValNanos expireTime = get_monotonic_time();

        if (timer->m_Looper != nullptr)
        {
            if (timer->m_Looper == this) {
                m_TimerMap.erase(timer->m_TimerMapIterator);
            } else {
                set_last_error(EBUSY);
                return false;
            }
        }
        expireTime += timer->m_Timeout;

        timer->m_Looper = this;
        timer->m_IsSingleshot = singleshot;
        timer->m_TimerMapIterator = m_TimerMap.insert(std::make_pair(expireTime, timer));

        if (m_NextEventTime.IsInfinit() || expireTime < m_NextEventTime)
        {
            m_NextEventTime = expireTime;
            m_TimerMapCondition.WakeupAll();
        }
        return true;
    }
    catch (const std::bad_alloc&) {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::RemoveTimer(EventTimer* timer)
{
    assert(!IsRunning() || m_Mutex.IsLocked());

    if (timer->m_Looper == this)
    {
        timer->m_Looper = nullptr;
        m_TimerMap.erase(timer->m_TimerMapIterator);
        m_NextEventTime = (m_TimerMap.empty()) ? TimeValNanos::infinit : m_TimerMap.begin()->first;
        return true;
    }
    else
    {
        printf("ERROR: Looper::RemoveTimer() attempt to remove timer not belonging to us\n");
        set_last_error(EINVAL);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<EventHandler> Looper::FindHandler(handler_id handle) const
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    auto i = m_HandlerMap.find(handle);
    if (i != m_HandlerMap.end()) {
        return i->second;
    }
    return nullptr;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* Looper::Run()
{
    CRITICAL_SCOPE(m_Mutex);
    ThreadStarted();

    while (m_DoRun)
    {
        ProcessEvents();
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::ProcessEvents()
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    while (Tick());
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::Tick()
{
    if (m_DoRun)
    {
        assert(m_Mutex.IsLocked());
        m_WaitGroup.WaitDeadline(m_Mutex, m_NextEventTime);
        RunTimers();
        for (;;)
        {
            handler_id targetHandler;
            int32_t    code;

            ssize_t msgLength = m_Port.ReceiveMessageTimeout(&targetHandler, &code, m_ReceiveBuffer.data(), m_ReceiveBuffer.size(), TimeValNanos::zero);
            if (msgLength >= 0)
            {
                ProcessMessage(targetHandler, code, msgLength);
            }
            else
            {
                Idle();
                break;
            }
        }
    }
    return m_DoRun;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Looper::ProcessMessage(handler_id targetHandler, int32_t code, ssize_t msgLength)
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    if (MessageID(code) == MessageID::QUIT) {
        Stop();
    }
    if (!HandleMessage(targetHandler, code, m_ReceiveBuffer.data(), msgLength))
    {
        if (targetHandler != INVALID_HANDLE)
        {
            auto i = m_HandlerMap.find(targetHandler);
            if (i != m_HandlerMap.end()) {
                i->second->HandleMessage(code, m_ReceiveBuffer.data(), msgLength);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Looper::RunTimers()
{
    assert(!IsRunning() || m_Mutex.IsLocked());
    TimeValNanos curTime = get_monotonic_time();

    while (!m_TimerMap.empty())
    {
        auto i = m_TimerMap.begin();

        TimeValNanos timeout = (*i).first;

        if ( timeout > curTime ) {
            m_NextEventTime = timeout;
            return;
        }

        EventTimer* timer = (*i).second;
        
        m_TimerMap.erase(i);

        if ( timer->m_IsSingleshot )
        {
            timer->m_Looper = nullptr;
        }
        else
        {
            timeout += timer->m_Timeout;
            timer->m_TimerMapIterator = m_TimerMap.insert(std::make_pair(std::max(curTime + TimeValNanos::FromNanoseconds(1),timeout), timer));
        }
        timer->SignalTrigged(timer);
    }
    m_NextEventTime = TimeValNanos::infinit;
}
