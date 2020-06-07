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
#include "SignalSystem.h"
#include "Slot.h"
#include "SignalSlotList.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename ...ARGS>
class VFConnector : public SignalSlotList<R, ARGS...>
{
public:
    VFConnector() {}
    VFConnector(const VFConnector& other) { *this = other; }
    VFConnector& operator=(const VFConnector& other)
    {
        for (SlotBase* slot = other.m_FirstSlot ; slot != nullptr ; slot = slot->GetNextInSignal())
        {
            ConnectInternal( slot->Clone( this ) );
        }
        return *this;
    }

    R operator() (ARGS... args) const
    {
        if ( this->m_FirstSlot != nullptr )
        {
            SignalBase::EmitGuard guard( this, this->m_FirstSlot );
            return static_cast<Slot<R, ARGS...>*>(this->m_FirstSlot)->Call(args...);
        }
        return VFCDefaultValue<R>::GetDefault();
    }
    static R CallBase(ARGS... args)
    {
        SignalBase::EmitGuard* emitGuardPtr = SignalBase::s_LocalEmitGuard.Get();
        SignalBase::PrevSlotGuard cGuard( emitGuardPtr );
        if ( emitGuardPtr->m_SlotIterator == nullptr) {
            assert(!"Call to a VFConnector with a null SlotIterator");
            return VFCDefaultValue<R>::GetDefault();
        }
        SlotBase* slot = emitGuardPtr->m_SlotIterator;
        return static_cast<Slot<R, ARGS...>*>(slot)->Call(args...);
    }
};
