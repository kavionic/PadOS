// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 06.09.2022 21:00

#pragma once

#include "SignalBase.h"
#include "SignalSystem.h"
#include "Slot.h"
#include "SignalSlotList.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename ...ARGS>
class VFConnectorUnguarded : public SignalSlotList<R, ARGS...>
{
public:
    VFConnectorUnguarded() {}
    VFConnectorUnguarded(const VFConnectorUnguarded& other) { *this = other; }
    VFConnectorUnguarded& operator=(const VFConnectorUnguarded& other)
    {
        for (SlotBase* slot = other.m_FirstSlot; slot != nullptr; slot = slot->GetNextInSignal())
        {
            ConnectInternal(slot->Clone(this));
        }
        return *this;
    }

    R operator() (ARGS... args) const
    {
        if (this->m_FirstSlot != nullptr)
        {
            return static_cast<Slot<R, ARGS...>*>(this->m_FirstSlot)->Call(args...);
        }
        return VFCDefaultValue<R>::GetDefault();
    }
};
