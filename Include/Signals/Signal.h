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

#include "SignalBase.h"
#include "Slot.h"
#include "SignalTarget.h"
#include "SignalSlotList.h"

template <typename R, typename ...ARGS>
class Signal : public SignalSlotList<R, ARGS...>
{
public:
    template <typename OBJ_CLASS,typename CALLBACK_TYPE>
    void ReConnect( const OBJ_CLASS* obj, CALLBACK_TYPE callback) const
    {
        if ( !IsSlotConnected( obj, callback ) )
        {
            Connect( obj, callback );
        }
    }

    template <typename CALLBACK_TYPE>
    void ReConnect( CALLBACK_TYPE callback ) const
    {
        if ( !IsSlotConnected( callback ) )
        {
            Connect( callback );
        }
    }

    bool operator() (ARGS... args) const
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

                static_cast<Slot<R, ARGS...>*>(slot)->Call(args...);
                if (guard.m_Signal == nullptr)
                {
                    return false; // Set by the destructor if the signal object was deleted by the slot function.
                }
            } while(guard.m_SlotIterator != nullptr);
        }
        return true;
    }
};

