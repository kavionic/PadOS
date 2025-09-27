// This file is part of PadOS.
//
// Copyright (C) 2018-2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 27.04.2018 22:30:54

#pragma once

#include "KNamedObject.h"

namespace kernel
{

class KMutex;

class KConditionVariable : public KNamedObject
{
public:
    static IFLASHC constexpr KNamedObjectType ObjectType = KNamedObjectType::ConditionVariable;
    
    IFLASHC KConditionVariable(const char* name, clockid_t clockID = CLOCK_MONOTONIC);
    IFLASHC ~KConditionVariable();
    
    PErrorCode Wait() { return WaitInternal(nullptr); }
    PErrorCode WaitTimeout(TimeValMicros timeout) { return WaitTimeoutInternal(nullptr, m_ClockID, timeout); }
    PErrorCode WaitDeadline(TimeValMicros deadline) { return WaitDeadlineInternal(nullptr, m_ClockID, deadline); }
    PErrorCode WaitClock(clockid_t clockID, TimeValMicros deadline) { return WaitDeadlineInternal(nullptr, clockID, deadline); }

    PErrorCode Wait(KMutex& lock) { return WaitInternal(&lock); }
    PErrorCode WaitTimeout(KMutex& lock, TimeValMicros timeout) { return WaitTimeoutInternal(&lock, m_ClockID, timeout); }
    PErrorCode WaitDeadline(KMutex& lock, TimeValMicros deadline) { return WaitDeadlineInternal(&lock, m_ClockID, deadline); }
    PErrorCode WaitClock(KMutex& lock, clockid_t clockID, TimeValMicros deadline) { return WaitDeadlineInternal(&lock, clockID, deadline); }

    IFLASHC PErrorCode IRQWait();
    IFLASHC PErrorCode IRQWaitTimeout(TimeValMicros timeout);
    IFLASHC PErrorCode IRQWaitDeadline(TimeValMicros deadline);
    IFLASHC PErrorCode IRQWaitClock(clockid_t clockID, TimeValMicros deadline);

    IFLASHC PErrorCode Wakeup(int threadCount);
    inline PErrorCode WakeupAll() { return Wakeup(0); }

private:
    IFLASHC PErrorCode WaitInternal(KMutex* lock);
    IFLASHC PErrorCode WaitTimeoutInternal(KMutex* lock, clockid_t clockID, TimeValMicros timeout);
    IFLASHC PErrorCode WaitDeadlineInternal(KMutex* lock, clockid_t clockID, TimeValMicros deadline);

    clockid_t m_ClockID = CLOCK_MONOTONIC;

    KConditionVariable(const KConditionVariable&) = delete;
    KConditionVariable& operator=(const KConditionVariable&) = delete;
};

} // namespace
