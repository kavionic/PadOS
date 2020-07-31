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
    static constexpr KNamedObjectType ObjectType = KNamedObjectType::ConditionVariable;
    
    KConditionVariable(const char* name);
    ~KConditionVariable();
    
    bool Wait(KMutex& lock);
    bool WaitTimeout(KMutex& lock, TimeValMicros timeout);
    bool WaitDeadline(KMutex& lock, TimeValMicros deadline);

    bool IRQWait();
    bool IRQWaitTimeout(TimeValMicros timeout);
    bool IRQWaitDeadline(TimeValMicros deadline);
    
    void Wakeup(int threadCount);
    void WakeupAll() { Wakeup(0); }

private:
    KConditionVariable(const KConditionVariable&) = delete;
    KConditionVariable& operator=(const KConditionVariable&) = delete;
};

} // namespace
