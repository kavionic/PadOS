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

#include "Platform.h"

#include "Looper.h"
#include "EventHandler.h"
#include "System/Utils/EventTimer.h"
#include "System/SystemMessageIDs.h"

using namespace os;

int32_t Looper::s_NextReplyToken;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Looper::Looper(const String& name, int portSize, size_t receiveBufferSize) : Thread(name), m_Mutex("looper"), m_Port("looper", portSize), m_DoRun(true)
{
    SetReceiveBufferSize(receiveBufferSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Looper::~Looper()
{
/*  while( !m_TimerMap.empty() )
  {
      m_TimerMap.begin()->second->Stop();
  }*/
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t Looper::SetReceiveBufferSize(size_t size)
{
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
    CRITICAL_SCOPE(m_Mutex);
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::RemoveHandler(Ptr<EventHandler> handler)
{
    CRITICAL_SCOPE(m_Mutex);
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
    bigtime_t expireTime = get_system_time();
    CRITICAL_SCOPE(m_Mutex);

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
    timer->m_TimerMapIterator = m_TimerMap.insert(std::make_pair(expireTime,timer));

    if (expireTime < m_NextEventTime) {
        m_NextEventTime = expireTime;
        thread_id thread = GetThreadID();
        if (get_thread_id() != thread) {
            wakeup_thread(thread);
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::RemoveTimer(EventTimer* timer)
{
    CRITICAL_SCOPE(m_Mutex);
    
    if (timer->m_Looper == this)
    {
        timer->m_Looper = nullptr;
        m_TimerMap.erase(timer->m_TimerMapIterator);
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
    auto i = m_HandlerMap.find(handle);
    if (i != m_HandlerMap.end()) {
        return i->second;
    }
    return nullptr;    
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int Looper::Run()
{
    ThreadStarted();

    while (m_DoRun)
    {
        ProcessEvents(-1, -1);
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool Looper::ProcessEvents(handler_id waitTarget, int32_t waitCode)
{
    while (m_DoRun)
    {
        bigtime_t nextEventTime;
        if (waitCode == -1)
        {
            CRITICAL_BEGIN(m_Mutex)
            {
                RunTimers();
                if (m_TimerMap.empty()) {
                    nextEventTime = INFINIT_TIMEOUT;
                } else {
                    nextEventTime = m_TimerMap.begin()->first;
                }
            } CRITICAL_END;
        }
        else
        {
            nextEventTime = INFINIT_TIMEOUT;
        }
        // RACE!!! If another thread start a timer now, we might get stuck waiting for a message.
        for (;;)
        {
            handler_id targetHandler;
            int32_t    code;
        
            ssize_t msgLength = m_Port.ReceiveMessageTimeout(&targetHandler, &code, m_ReceiveBuffer.data(), m_ReceiveBuffer.size(), 0);
            if (msgLength < 0 && get_last_error() == ETIME) {
                Idle();
                msgLength = m_Port.ReceiveMessageDeadline(&targetHandler, &code, m_ReceiveBuffer.data(), m_ReceiveBuffer.size(), nextEventTime);
            }
            if (msgLength >= 0) {
                ProcessMessage(targetHandler, code, msgLength);
            }
            else {
                break;
            }
        }            
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void Looper::ProcessMessage(handler_id targetHandler, int32_t code, ssize_t msgLength)
{
    CRITICAL_SCOPE(m_Mutex);

    if (code == MessageID::QUIT) {
        Stop();
    }
    if (!HandleMessage(targetHandler, code, m_ReceiveBuffer.data(), msgLength))
    {
        if (targetHandler != -1)
        {
            auto i = m_HandlerMap.find(targetHandler);
            if (i != m_HandlerMap.end()) {
                i->second->HandleMessage(code, m_ReceiveBuffer.data(), msgLength);
            }
        }
/*                    for (auto i : m_HandlerMap)
        {
            if (i.second->HandleMessage(code, m_ReceiveBuffer.data(), msgLength)) {
                break;
            }
        }*/
    }
/*    if ((waitCode != -1 && code == waitCode) && (waitTarget == -1 || waitTarget == targetHandler))
    {
        return true;
    }*/
//            printf("Looper::ProcessEvents() '%s' handline event %d (%d)\n", GetName().c_str(), code, msgLength);
//           printf("Looper::Run() message received\n");
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bigtime_t Looper::RunTimers()
{
    CRITICAL_SCOPE(m_Mutex);
    bigtime_t curTime = get_system_time();

    while (!m_TimerMap.empty())
    {
        auto i = m_TimerMap.begin();

        bigtime_t timeout = (*i).first;

        if ( timeout > curTime ) {
            return timeout;
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
            timer->m_TimerMapIterator = m_TimerMap.insert(std::make_pair(std::max(curTime + 1,timeout), timer));
        }
        timer->SignalTrigged(timer);
    }
    return INFINIT_TIMEOUT;
}

