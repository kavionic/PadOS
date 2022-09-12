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
#include "Slot.h"
#include "SignalTarget.h"
#include "SignalSlotList.h"

template <typename R, typename ...ARGS>
class SignalUnguarded : public SignalSlotList<R, ARGS...>
{
public:
    template <typename OBJ_CLASS, typename CALLBACK_TYPE>
    void ReConnect(const OBJ_CLASS* obj, CALLBACK_TYPE callback) const
    {
        if (!IsSlotConnected(obj, callback))
        {
            Connect(obj, callback);
        }
    }

    template <typename CALLBACK_TYPE>
    void ReConnect(CALLBACK_TYPE callback) const
    {
        if (!IsSlotConnected(callback))
        {
            Connect(callback);
        }
    }

    void operator() (ARGS... args) const
    {
        for (SlotBase* slot = this->m_FirstSlot; slot != nullptr; slot = slot->GetNextInSignal())
        {
            static_cast<Slot<R, ARGS...>*>(slot)->Call(args...);
        }
    }
};

