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

#include "SignalTarget.h"
#include "SignalSystem.h"

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalTarget::SignalTarget()
{
    m_FirstSlot = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \par Description:
///   Copy constructor. It's implemented to allow inherited classes to support
///   copying, but it does not copy the signal connections that has been made.
///   If the new copy is supposed to have the same signal connections as the
///   source, thay has to be setup manually before or after the copying. The
///   copy constructor simply initialize the object in the same way as the
///   default constructor.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalTarget::SignalTarget(const SignalTarget&)
{
    m_FirstSlot = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalTarget::~SignalTarget()
{
    DisconnectAllSignals();
}

///////////////////////////////////////////////////////////////////////////////
/// \par Description:
///   Assignment operator. It's implemented to allow inherited classes to support
///   copying, but it does not copy the signal connections that has been made.
///   If the target is supposed to have the same signal connections as the
///   source, thay has to be setup manually before or after the assignment.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SignalTarget& SignalTarget::operator=( const SignalTarget& )
{
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SignalTarget::DisconnectAllSignals()
{
    while( m_FirstSlot != nullptr )
    {
        // The SlotBase destructor will unlink it self from the linked
        // list starting at m_FirstSlot. So after the destructor returns
        // m_FirstSlot will either point to the next slot in the list,
        // or be nullptr if this was the last slot.
        delete m_FirstSlot; 
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SignalTarget::RegisterSlot( SlotBase* slot )
{
    if ( m_FirstSlot != nullptr )
    {
        m_FirstSlot->m_PrevInTarget = slot;
    }
  
    slot->m_NextInTarget = m_FirstSlot;
    slot->m_PrevInTarget = nullptr;
  
    m_FirstSlot = slot;
    slot->m_Object  = this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SignalTarget::UnregisterSlot(SlotBase* slot)
{
    if ( slot->m_PrevInTarget != nullptr ) {
        slot->m_PrevInTarget->m_NextInTarget = slot->m_NextInTarget;
    } else {
        m_FirstSlot = slot->m_NextInTarget;
    }
    if ( slot->m_NextInTarget != nullptr ) {
        slot->m_NextInTarget->m_PrevInTarget = slot->m_PrevInTarget;
    }  
    slot->m_Object = nullptr;
}
