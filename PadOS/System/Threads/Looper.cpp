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

#include "sam.h"

#include "Looper.h"
#include "EventHandler.h"
#include "System/Utils/EventTimer.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Looper::Looper(int portSize) : m_Mutex("looper"), m_Port("looper", portSize), m_DoRun(true)
{
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

bool Looper::AddHandler(Ptr<EventHandler> handler)
{
    CRITICAL_SCOPE(m_Mutex);
    m_HandlerList.push_back(handler);
    return true;
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

int  Looper::Run()
{
    while (m_DoRun)
    {
        bigtime_t nextEventTime;
        CRITICAL_BEGIN(m_Mutex)
        {
            RunTimers();
            if (m_TimerMap.empty()) {
                nextEventTime = INFINIT_TIMEOUT;
            } else {
                nextEventTime = m_TimerMap.begin()->first;
            }
        } CRITICAL_END;
        // RACE!!! If another thread start a timer now, we might get stuck waiting for a message.
        int32_t code;
        size_t  bufferSize = 500;
        void*   buffer = malloc(bufferSize);
        ssize_t msgLength = m_Port.ReceiveMessageDeadline(&code, buffer, bufferSize, nextEventTime);
        if (msgLength >= 0)
        {
            CRITICAL_BEGIN(m_Mutex)
            {
                if (code == 123) {
                    Stop();
                }
                for (auto i : m_HandlerList)
                {
                    if (i->HandleMessage(code, buffer, msgLength)) {
                        break;
                    }
                }
    //           printf("Looper::Run() message received\n");
            } CRITICAL_END;
        }            
        free(buffer);
    }
    return 0;
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
            timer->m_TimerMapIterator = m_TimerMap.insert(std::make_pair(timeout, timer));
        }
        timer->SignalTrigged(timer);
    }
    return INFINIT_TIMEOUT;
}

