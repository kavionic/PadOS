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

#pragma once

#include <assert.h>
#include "SlotBase.h"
#include "Threads/ThreadLocal.h"

class SignalBase
{
public:
    SignalBase();
    SignalBase( const SignalBase& other );

    bool Empty() const { return m_FirstSlot == nullptr; }

    SignalBase& operator=( const SignalBase& other );
  
protected:
    ~SignalBase();

    void ConnectInternal(SlotBase* slot) const;
    void DisconnectInternal(SlotBase* slot, bool deleteSlot = true) const;

    struct EmitGuard
    {
        EmitGuard( const SignalBase* targetSignal, SlotBase* slotIterator ) : m_SlotIterator(slotIterator)
        {
            m_Signal = targetSignal;
            m_Next = s_LocalEmitGuard.Get();
            s_LocalEmitGuard.Set(this);
        }
        ~EmitGuard()
        {
            assert(s_LocalEmitGuard.Get() == this);
            s_LocalEmitGuard.Set(m_Next);
        }
        SlotBase*         m_SlotIterator;
        const SignalBase* m_Signal;
        EmitGuard*        m_Next;
    };
  
    struct PrevSlotGuard
    {
        PrevSlotGuard( EmitGuard* guard ) {
            m_Guard = guard;
            m_Guard->m_SlotIterator = m_Guard->m_SlotIterator->GetNextInSignal();
        }
        ~PrevSlotGuard() {
            if (m_Guard->m_Signal != nullptr && m_Guard->m_SlotIterator != nullptr) {
                m_Guard->m_SlotIterator = m_Guard->m_SlotIterator->GetPrevInSignal();
            }
        }
        EmitGuard* m_Guard;
    };

    mutable SlotBase* m_FirstSlot;
  
private:
    friend class  SlotBase;
    friend struct EmitGuard;

    static ThreadLocal<EmitGuard*> s_LocalEmitGuard;
};


