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

#pragma once

#include "SignalBase.h"
#include "Slot.h"
#include "SignalTarget.h"


template <typename R, typename ...ARGS>
class SignalSlotList : public SignalBase
{
public:
    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename... fARGS>
    auto FindSlot( const T* obj, R (fC::*callback)(fARGS...) ) const
    {
        for ( SlotBase* i = m_FirstSlot ; i != nullptr ; i = i->GetNextInSignal() )
        {
            typedef R (fC::*Signature)(fARGS...);
            typedef SlotFull<sizeof...(fARGS), fC, R, Signature, ARGS...> SlotType;
            SlotType* tmp = dynamic_cast<SlotType*>(i);
            if ( tmp != nullptr && tmp->GetSignalTarget() == obj && tmp->m_Callback == callback ) {
                return tmp;
            }
        }
        return nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename ...fARGS>
    auto FindSlot(const T* obj, R (fC::*callback)(fARGS...) const ) const
    {
        for ( SlotBase* i = m_FirstSlot ; i != nullptr ; i = i->GetNextInSignal() )
        {
            typedef R (fC::*Signature)(fARGS...) const;
            typedef SlotFull<sizeof...(fARGS), fC, R, Signature, ARGS...> SlotType;
            SlotType* tmp = dynamic_cast<SlotType*>(i);
            if ( tmp != nullptr && tmp->GetSignalTarget() == obj && tmp->m_Callback == callback ) {
                return tmp;
            }
        }
        return nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename ...fARGS>
    auto FindSlot( R (*callback)(fARGS...) ) const
    {
        for ( SlotBase* i = m_FirstSlot ; i != nullptr ; i = i->GetNextInSignal() )
        {
            typedef R Signature(fARGS...);
            typedef SlotFull<sizeof...(fARGS), SignalTarget, R, Signature, ARGS...> SlotType;
            SlotType* tmp = dynamic_cast<SlotType*>(i);
            if ( tmp != nullptr && tmp->m_Callback == callback ) {
                return tmp;
            }
        }
        return nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename ...fARGS>
    void Connect(const T* object, R (fC::*callback)(fARGS...)) const
    {
        typedef R (fC::*Signature)(fARGS...);
        auto slot = new SlotFull<sizeof...(fARGS), fC, R, Signature, ARGS...>(const_cast<SignalBase*>(static_cast<const SignalBase*>(this)), const_cast<fC*>(static_cast<const fC*>(object)), callback);
        ConnectInternal(slot);
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename ...fARGS>
    void Connect(const T* object, R (fC::*callback)(fARGS...) const) const
    {
        typedef R (fC::*Signature)(fARGS...) const;
        auto slot = new SlotFull<sizeof...(fARGS), fC, R, Signature, ARGS...>(const_cast<SignalBase*>(static_cast<const SignalBase*>(this)), object, callback);
        ConnectInternal(slot);
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename ...fARGS>
    void Connect(R (*callback)(fARGS...)) const
    {
        typedef R (*Signature)(fARGS...);
        auto slot = new SlotFull<sizeof...(fARGS), SignalTarget, R, Signature, ARGS...>(const_cast<SignalBase*>(static_cast<const SignalBase*>(this)), nullptr, callback);
        ConnectInternal(slot);
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename ...fARGS> void Disconnect(const T* obj, R (fC::*callback)(fARGS...)) const
    {
        DisconnectInternal(FindSlot(static_cast<const fC*>(obj), callback));
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename ...fARGS> void Disconnect(const T* obj, R (fC::*callback)(fARGS...) const) const
    {
        DisconnectInternal(FindSlot(static_cast<const fC*>(obj), callback ));
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename ...fARGS>
    void Disconnect(R (*callback)(fARGS...)) const
    {
        DisconnectInternal(FindSlot(callback));
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename ...fARGS> bool IsSlotConnected(const T* obj, R (fC::*callback)(fARGS...)) const
    {
        return FindSlot(static_cast<const fC*>(obj), callback) != nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename fC,typename T, typename ...fARGS> bool IsSlotConnected(const T* obj, R (fC::*callback)(fARGS...) const) const
    {
        return FindSlot(static_cast<const fC*>(obj), callback) != nullptr;
    }

    /////////////////////////////////////////////////////////////////////////////
    ///
    /////////////////////////////////////////////////////////////////////////////

    template <typename ...fARGS> bool IsSlotConnected(R (*callback)(fARGS...)) const
    {
        return FindSlot(callback)  != nullptr;
    }
};
