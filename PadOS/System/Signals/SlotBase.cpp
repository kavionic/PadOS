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
// Created: 17.03.2018 14:59:22

#include "SlotBase.h"
#include "SignalBase.h"
#include "SignalTarget.h"

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
