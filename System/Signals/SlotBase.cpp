// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 17.03.2018 14:59:22

#include "Signals/SlotBase.h"
#include "Signals/SignalBase.h"
#include "Signals/SignalTarget.h"

#ifndef NDEBUG
static int g_SlotCount = 0;
#endif  //!defined(NDEBUG)

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SlotBase::SlotBase(SignalBase* signal, SignalTarget* object) : m_Signal(signal), m_Object(object)
{
#ifndef NDEBUG
    g_SlotCount++;
#endif // NDEBUG
    
    m_PrevInSignal = nullptr;
    m_NextInSignal = nullptr;
    m_PrevInTarget = nullptr;
    m_NextInTarget = nullptr;

    if ( m_Object != nullptr ) {
        m_Object->RegisterSlot(this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SlotBase::~SlotBase()
{
    if ( m_Signal != nullptr ) {
        m_Signal->DisconnectInternal(this, false);
    }
    if ( m_Object != nullptr ) {
        m_Object->UnregisterSlot(this);
    }
#ifndef NDEBUG
    g_SlotCount--;
#endif // NDEBUG
}
