// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "SignalBase.h"
#include "SignalSystem.h"
#include "Slot.h"
#include "SignalSlotList.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TSignalReturnType, typename ...TSignalArgs>
class VFConnector : public SignalSlotList<TSignalReturnType, TSignalArgs...>
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

    TSignalReturnType operator() (TSignalArgs... args) const
    {
        if ( this->m_FirstSlot != nullptr )
        {
            SignalBase::EmitGuard guard( this, this->m_FirstSlot );
            return static_cast<Slot<TSignalReturnType, TSignalArgs...>*>(this->m_FirstSlot)->Call(args...);
        }
        return VFCDefaultValue<TSignalReturnType>::GetDefault();
    }
    static TSignalReturnType CallBase(TSignalArgs... args)
    {
        SignalBase::EmitGuard* emitGuardPtr = SignalBase::s_LocalEmitGuard;
        SignalBase::PrevSlotGuard cGuard( emitGuardPtr );
        if ( emitGuardPtr->m_SlotIterator == nullptr) {
            assert(!"Call to a VFConnector with a null SlotIterator");
            return VFCDefaultValue<TSignalReturnType>::GetDefault();
        }
        SlotBase* slot = emitGuardPtr->m_SlotIterator;
        return static_cast<Slot<TSignalReturnType, TSignalArgs...>*>(slot)->Call(args...);
    }
};

template<typename TSignalReturnType, typename... TSignalArgs>
class VFConnector<TSignalReturnType (TSignalArgs...)> : public VFConnector<TSignalReturnType, TSignalArgs...> {};
