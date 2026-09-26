// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

    SlotBase* FindSlotByHandle(signal_slot_handle_t handle) const;

    struct EmitGuard
    {
        EmitGuard( const SignalBase* targetSignal, SlotBase* slotIterator ) : m_SlotIterator(slotIterator)
        {
            m_Signal = targetSignal;
            m_Next = s_LocalEmitGuard;
            s_LocalEmitGuard = this;
        }
        ~EmitGuard()
        {
            assert(s_LocalEmitGuard == this);
            s_LocalEmitGuard = m_Next;
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

    static thread_local EmitGuard* s_LocalEmitGuard;
};


