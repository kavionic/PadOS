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

#include "System/Platform.h"

#include "Threads/EventTimer.h"
#include "System/System.h"
#include "Threads/Looper.h"
#include "System/SysTime.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

EventTimer::EventTimer(TimeValMicros timeout, bool singleshot, int32_t id) : m_ID(id), m_Timeout(timeout)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

EventTimer::~EventTimer()
{
    Stop();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void EventTimer::Set(TimeValMicros timeout, bool singleshot)
{
    if ( timeout <= TimeValMicros::zero ) {
        timeout = TimeValMicros::FromMicroseconds(1);
    }
    m_Timeout = timeout;
    
    if (m_Looper != nullptr)
    {
        assert(m_Looper->GetThreadID() == get_thread_id());
        m_Looper->AddTimer(this, singleshot);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool EventTimer::Start(bool singleShot, Looper* looper)
{
    if (looper == nullptr) {
        looper = Looper::GetCurrentLooper();
    }
    if (looper != nullptr) {
        looper->AddTimer(this, singleShot);
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void EventTimer::Stop()
{
    if (m_Looper != nullptr) {
        m_Looper->RemoveTimer(this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool EventTimer::IsRunning() const
{
    return m_Looper != nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void EventTimer::SetID(int32_t id)
{
    m_ID = id;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t EventTimer::GetID() const
{
    return m_ID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TimeValMicros EventTimer::GetRemainingTime() const
{  
    if (m_Looper == nullptr) {
        return TimeValMicros::zero;
    } else {
        return m_TimerMapIterator->first - get_system_time();
    }
}

