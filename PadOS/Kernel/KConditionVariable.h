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
// Created: 27.04.2018 22:30:54

#pragma once

#include "KNamedObject.h"

namespace kernel
{

class KMutex;

class KConditionVariable : public KNamedObject
{
public:
    static const KNamedObjectType ObjectType = KNamedObjectType::ConditionVariable;
    
    KConditionVariable(const char* name);
    ~KConditionVariable();
    
    bool Wait(KMutex& lock);
    bool WaitTimeout(KMutex& lock, bigtime_t timeout);
    bool WaitDeadline(KMutex& lock, bigtime_t deadline);

    bool IRQWait();
    bool IRQWaitTimeout(bigtime_t timeout);
    bool IRQWaitDeadline(bigtime_t deadline);
    
    void Wakeup(int threadCount = 1);
    
private:
    KConditionVariable(const KConditionVariable&) = delete;
    KConditionVariable& operator=(const KConditionVariable&) = delete;
};

} // namespace
