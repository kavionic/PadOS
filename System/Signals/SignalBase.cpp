// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include "Signals/SignalBase.h"

#ifndef NDEBUG
static int g_SignalCount = 0;
#endif  //!defined(NDEBUG)

thread_local SignalBase::EmitGuard* SignalBase::s_LocalEmitGuard;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalBase::SignalBase()
{
#ifndef NDEBUG
    g_SignalCount++;
#endif // NDEBUG
    m_FirstSlot = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalBase::SignalBase(const SignalBase& other)
{
#if !defined(NDEBUG)
    g_SignalCount++;
#endif // NDEBUG
  
    m_FirstSlot = nullptr;

    for ( SlotBase* pcSlot = other.m_FirstSlot ; pcSlot != nullptr ; pcSlot = pcSlot->m_NextInSignal ) {
        ConnectInternal(pcSlot->Clone(this));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalBase::~SignalBase()
{
#if !defined(NDEBUG)
    g_SignalCount--;
#endif // defined(N3_CLIENT) || !defined(NDEBUG)

    for ( EmitGuard* guard = s_LocalEmitGuard ; guard != nullptr ; guard = guard->m_Next ) {
        if ( guard->m_Signal == this ) {
            guard->m_Signal = nullptr; // Make sure Emit() don't screw up.
        }
    }

    while( m_FirstSlot != nullptr ) {
        DisconnectInternal( m_FirstSlot );
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalBase& SignalBase::operator=(const SignalBase& other)
{
    while( m_FirstSlot != nullptr ) {
        DisconnectInternal(m_FirstSlot);
    }
    for ( SlotBase* pcSlot = other.m_FirstSlot ; pcSlot != nullptr ; pcSlot = pcSlot->m_NextInSignal ) {
        ConnectInternal(pcSlot->Clone(this));
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SignalBase::ConnectInternal(SlotBase* pcSlot) const
{
    if (m_FirstSlot != nullptr) {
        m_FirstSlot->m_PrevInSignal = pcSlot;
    }
  
    pcSlot->m_NextInSignal = m_FirstSlot;
    pcSlot->m_PrevInSignal = nullptr;
  
    m_FirstSlot = pcSlot;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SignalBase::DisconnectInternal(SlotBase* slot, bool deleteSlot) const
{
    if ( slot == nullptr ) {
        return;
    }

    assert(slot->m_Signal == this);
    
    for (EmitGuard* guard = s_LocalEmitGuard ; guard != nullptr ; guard = guard->m_Next) {
        if (guard->m_SlotIterator == slot) {
            guard->m_SlotIterator = slot->m_NextInSignal;
        }
    }
    if (slot->m_PrevInSignal != nullptr) {
        slot->m_PrevInSignal->m_NextInSignal = slot->m_NextInSignal;
    } else {
        m_FirstSlot = slot->m_NextInSignal;
    }
    if (slot->m_NextInSignal != nullptr) {
        slot->m_NextInSignal->m_PrevInSignal = slot->m_PrevInSignal;
    }
    slot->m_Signal = nullptr;
    if (deleteSlot) {
        delete slot;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SlotBase* SignalBase::FindSlotByHandle(signal_slot_handle_t handle) const
{
    for (SlotBase* slot = m_FirstSlot; slot != nullptr; slot = slot->GetNextInSignal())
    {
        if (slot == handle.SlotPtr) {
            return slot;
        }
    }
    return nullptr;
}
