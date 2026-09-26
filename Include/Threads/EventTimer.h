// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 25.02.2018 12:57:09

#pragma once

#include <map>

#include "Kernel/Kernel.h"
#include "Signals/Signal.h"


class PLooper;

class PEventTimer
{
public:
    PEventTimer(TimeValNanos timeout = TimeValNanos::zero, bool singleshot = false, int32_t id = 0);
    ~PEventTimer();
    
    void      Set(TimeValNanos timeout, bool singleshot = false);

    bool      Start(bool singleShot = false, PLooper* looper = nullptr);
    void      Stop();
    bool      IsRunning() const;
    bool      IsSingleshot() const { return m_IsSingleshot; }
              
    void      SetID(int32_t ID);
    int32_t   GetID() const;

    TimeValNanos GetTimeout() const { return m_Timeout; }
    TimeValNanos GetRemainingTime() const;

    Signal<void, PEventTimer*> SignalTrigged;

private:
    friend class PLooper;

    PLooper* m_Looper = nullptr;
    int32_t                                             m_ID;
    TimeValNanos                                        m_Timeout;
    bool                                                m_IsSingleshot = false;
    std::multimap<TimeValNanos, PEventTimer*>::iterator  m_TimerMapIterator;

    PEventTimer( const PEventTimer &c );
    PEventTimer& operator=( const PEventTimer &c );

};
