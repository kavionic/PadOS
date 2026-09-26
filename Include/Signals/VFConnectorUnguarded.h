// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
