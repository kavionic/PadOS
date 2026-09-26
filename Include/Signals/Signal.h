// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "SignalBase.h"
#include "Slot.h"
#include "SignalTarget.h"
#include "SignalSlotList.h"

template <typename TSignalReturnType, typename ...TSignalArgs>
class Signal : public SignalSlotList<TSignalReturnType, TSignalArgs...>
{
public:
    template <typename TObject,typename TMethod>
    void ReConnect( const TObject* obj, TMethod callback) const
    {
        if ( !IsSlotConnected( obj, callback ) )
        {
            Connect( obj, callback );
        }
    }

    template <typename TCallback>
    void ReConnect(TCallback callback ) const
    {
        if ( !IsSlotConnected( callback ) )
        {
            Connect( callback );
        }
    }

    bool operator() (TSignalArgs... args) const
    {
        /// Since the slot functions we call might delete either the signal object
        /// itself (our this pointer) or disconnect some of the slots in our slot list
        /// we must make sure that the SignalBase class know what we are doing. To do
        /// that we keep most of our local variables in the EmitGuard structure and
        /// add this to a single-linked list pointed to by GetThisThreadEmitGuardPtr()
        /// before we start iterating the slot list. If the destructor is called it will
        /// iterate over the guard list and set EmitGuard::m_Signal to nullptr. This
        /// will tell us that the signal object was deleted and we should return
        /// immediately without touching any member-variables. We also keep the list
        /// iterator in the guard structure so that SignalBase::Disconnect() can
        /// increment it if the slot function disconnect the slot currently referred
        /// by the iterator. Since we use a linked list of guard objects this system
        /// will also work if the slot function cause a recursive call of operator().

        if (this->m_FirstSlot != nullptr)
        {
            SignalBase::EmitGuard guard(this, this->m_FirstSlot);
            do
            {
                SlotBase* slot = guard.m_SlotIterator;
                guard.m_SlotIterator = guard.m_SlotIterator->GetNextInSignal();

                static_cast<Slot<TSignalReturnType, TSignalArgs...>*>(slot)->Call(args...);
                if (guard.m_Signal == nullptr)
                {
                    return false; // Set by the destructor if the signal object was deleted by the slot function.
                }
            } while(guard.m_SlotIterator != nullptr);
        }
        return true;
    }
};

template<typename TSignalReturnType, typename... TSignalArgs>
class Signal<TSignalReturnType (TSignalArgs...)> : public Signal<TSignalReturnType, TSignalArgs...> {};
