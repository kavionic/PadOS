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
// Created: 25.02.2018 12:57:08

#include "sam.h"

#include "EventTimer.h"
#include "System/System.h"
#include "System/Threads/Looper.h"

std::multimap< bigtime_t, EventTimer* > EventTimer::s_TimerMap;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

EventTimer::EventTimer(bigtime_t timeout, bool singleshot, int32_t id) : m_ID(id), m_Timeout(timeout)
{
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

EventTimer::~EventTimer()
{
    //Stop();
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void EventTimer::Set(bigtime_t timeout)
{
//    Stop();

    if ( timeout < 1 ) {
        timeout = 1;
    }
    m_Timeout = timeout;
    
    if (m_Looper != nullptr)
    {
        assert(m_Looper->GetThreadID() == get_thread_id());
        m_Looper->AddTimer(this);
    }
//    m_IsSingleshot = singleshot;
//    bigtime_t expireTime = timeout + get_system_time();
//    m_TimerMapIterator = s_TimerMap.insert( std::make_pair(expireTime,this) );
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

/*void EventTimer::Stop()
{
    if (m_Looper != nullptr) {
        m_Looper->RemoveTimer(this);
    }
    if ( m_Timeout >= 0 ) {
        s_TimerMap.erase(m_TimerMapIterator);
    }
    m_Timeout = -1;
}*/

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool EventTimer::IsRunning() const
{
    return m_Looper != nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void EventTimer::SetID(int32_t id)
{
    m_ID = id;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

int32_t EventTimer::GetID() const
{
    return m_ID;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bigtime_t EventTimer::GetRemainingTime() const
{  
    if (m_Looper == nullptr) {
        return 0;
    } else {
        return m_TimerMapIterator->first - get_system_time();
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
void EventTimer::Tick()
{
    bigtime_t curTime = get_system_time();

    while (!s_TimerMap.empty())
    {
        std::multimap< bigtime_t, EventTimer* >::iterator i = s_TimerMap.begin();

        bigtime_t timeout = (*i).first;

        if ( timeout > curTime ) {
            break;
        }

        EventTimer* timer = (*i).second;
      
        s_TimerMap.erase(i);

        if ( timer->m_IsSingleshot )
        {
            timer->m_Timeout = -1;
        }
        else
        {
            timeout += timer->m_Timeout;
            timer->m_TimerMapIterator = s_TimerMap.insert(std::make_pair(timeout, timer));
        }
        timer->SignalTrigged(timer);
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void EventTimer::Shutdown()
{
  while( !s_TimerMap.empty() )
  {
    s_TimerMap.begin()->second->Stop();
  }
}
*/