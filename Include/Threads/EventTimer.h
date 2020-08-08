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
// Created: 25.02.2018 12:57:09

#pragma once

#include <map>

#include "Kernel/Kernel.h"
#include "Signals/Signal.h"

namespace os
{
    
class Looper;

class EventTimer
{
public:
    EventTimer(TimeValMicros timeout = TimeValMicros::zero, bool singleshot = false, int32_t id = 0);
    ~EventTimer();
    
    void      Set(TimeValMicros timeout, bool singleshot = false);

    bool      Start(bool singleShot = false, Looper* looper = nullptr);
    void      Stop();
    bool      IsRunning() const;
    bool      IsSingleshot() const { return m_IsSingleshot; }
              
    void      SetID(int32_t ID);
    int32_t   GetID() const;

    TimeValMicros GetTimeout() const { return m_Timeout; }
    TimeValMicros GetRemainingTime() const;

    Signal<void, EventTimer*> SignalTrigged;

private:
    friend class Looper;

    Looper* m_Looper = nullptr;
    int32_t                                             m_ID;
    TimeValMicros                                       m_Timeout;
    bool                                                m_IsSingleshot = false;
    std::multimap<TimeValMicros, EventTimer*>::iterator m_TimerMapIterator;

    EventTimer( const EventTimer &c );
    EventTimer& operator=( const EventTimer &c );

};

} // namespace
