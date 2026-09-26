// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "Slot.h"

class SlotBase;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class SignalTarget
{
public:
    SignalTarget();
    SignalTarget(const SignalTarget&);
    virtual ~SignalTarget();

    SignalTarget& operator=(const SignalTarget&);

    void DisconnectAllSignals();

private:
    friend class SlotBase;

    void RegisterSlot( SlotBase* slot );
    void UnregisterSlot( SlotBase* slot );

    SlotBase* m_FirstSlot;
};
