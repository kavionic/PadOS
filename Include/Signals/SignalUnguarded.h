// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

template<typename TSignalReturnType, typename... TSignalArgs>
class SignalUnguarded<TSignalReturnType(TSignalArgs...)> : public SignalUnguarded<TSignalReturnType, TSignalArgs...> {};
