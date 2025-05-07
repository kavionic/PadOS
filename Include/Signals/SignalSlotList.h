// This file is part of PadOS.
//
// Copyright (C) 2018-2025 Kurt Skauen <http://kavionic.com/>
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

#include <Utils/TypeTraits.h>

#include "SignalBase.h"
#include "Slot.h"
#include "SignalTarget.h"


template <typename TSignalReturnType, typename ...TSignalArgs>
class SignalSlotList : public SignalBase
{
public:
    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass, typename TOwner, typename... TMethodArgs>
    auto FindSlot(const TOwner* obj, TMethodReturnType(TMethodClass::*callback)(TMethodArgs...) ) const
    {
        typedef TMethodReturnType(TMethodClass::* Signature)(TMethodArgs...);
        typedef SlotFull<sizeof...(TMethodArgs), TMethodClass, TMethodReturnType, Signature, TSignalArgs...> SlotType;

        for ( SlotBase* i = m_FirstSlot ; i != nullptr ; i = i->GetNextInSignal() )
        {
            SlotType* tmp = dynamic_cast<SlotType*>(i);
            if ( tmp != nullptr && tmp->GetSignalTarget() == obj && tmp->GetCallback() == callback ) {
                return tmp;
            }
        }
        return (SlotType*) nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass, typename TOwner, typename ...TMethodArgs>
    auto FindSlot(const TOwner* obj, TMethodReturnType(TMethodClass::*callback)(TMethodArgs...) const ) const
    {
        typedef TMethodReturnType(TMethodClass::* Signature)(TMethodArgs...) const;
        typedef SlotFull<sizeof...(TMethodArgs), TMethodClass, TMethodReturnType, Signature, TSignalArgs...> SlotType;

        for ( SlotBase* i = m_FirstSlot ; i != nullptr ; i = i->GetNextInSignal() )
        {
            SlotType* tmp = dynamic_cast<SlotType*>(i);
            if ( tmp != nullptr && tmp->GetSignalTarget() == obj && tmp->GetCallback() == callback ) {
                return tmp;
            }
        }
        return (SlotType*) nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TFunctionReturnType, typename ...TFunctionArgs>
    auto FindSlot(TFunctionReturnType (*callback)(TFunctionArgs...) ) const
    {
        typedef TFunctionReturnType Signature(TFunctionArgs...);
        typedef SlotFull<sizeof...(TFunctionArgs), SignalTarget, TFunctionReturnType, Signature, TSignalArgs...> SlotType;

        for ( SlotBase* i = m_FirstSlot ; i != nullptr ; i = i->GetNextInSignal() )
        {
            SlotType* tmp = dynamic_cast<SlotType*>(i);
            if ( tmp != nullptr && tmp->GetCallback() == callback ) {
                return tmp;
            }
        }
        return (SlotType*) nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass, typename TOwner, typename ...TMethodArgs>
    signal_slot_handle_t Connect(const TOwner* object, TMethodReturnType(TMethodClass::*callback)(TMethodArgs...)) const
    {
        typedef TMethodReturnType (TMethodClass::*Signature)(TMethodArgs...);
        auto slot = new SlotFull<sizeof...(TMethodArgs), TMethodClass, TMethodReturnType, Signature, TSignalArgs...>(const_cast<SignalBase*>(static_cast<const SignalBase*>(this)), const_cast<TMethodClass*>(static_cast<const TMethodClass*>(object)), callback);
        ConnectInternal(slot);

        return signal_slot_handle_t(slot);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass,typename TOwner, typename ...TMethodArgs>
    signal_slot_handle_t Connect(const TOwner* object, TMethodReturnType (TMethodClass::*callback)(TMethodArgs...) const) const
    {
        typedef TMethodReturnType (TMethodClass::*Signature)(TMethodArgs...) const;
        auto slot = new SlotFull<sizeof...(TMethodArgs), TMethodClass, TMethodReturnType, Signature, TSignalArgs...>(const_cast<SignalBase*>(static_cast<const SignalBase*>(this)), object, callback);
        ConnectInternal(slot);

        return signal_slot_handle_t(slot);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TFunctionReturnType, typename ...TFunctionArgs>
    signal_slot_handle_t Connect(TFunctionReturnType (*callback)(TFunctionArgs...)) const
    {
        typedef TFunctionReturnType (*Signature)(TFunctionArgs...);
        auto slot = new SlotFull<sizeof...(TFunctionArgs), void, TFunctionReturnType, Signature, TSignalArgs...>(const_cast<SignalBase*>(
            static_cast<const SignalBase*>(this)),
            nullptr,
            callback
        );
        ConnectInternal(slot);

        return signal_slot_handle_t(slot);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TFunctionReturnType, typename ...TFunctionArgs>
    signal_slot_handle_t Connect(SignalTarget* owner, TFunctionReturnType (*callback)(TFunctionArgs...)) const
    {
        typedef TFunctionReturnType (*Signature)(TFunctionArgs...);
        auto slot = new SlotFull<sizeof...(TFunctionArgs), void, TFunctionReturnType, Signature, TSignalArgs...>(
            const_cast<SignalBase*>(static_cast<const SignalBase*>(this)),
            owner,
            callback
        );
        ConnectInternal(slot);

        return signal_slot_handle_t(slot);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TSignature>
    signal_slot_handle_t Connect(TSignature&& callback) const
    {
        auto slot = new SlotFull<std::tuple_size_v<callable_argument_types_t<TSignature>>, void, callable_return_type_t<TSignature>, TSignature, TSignalArgs...>(
            const_cast<SignalBase*>(static_cast<const SignalBase*>(this)),
            nullptr,
            std::move(callback)
        );
        ConnectInternal(slot);

        return signal_slot_handle_t(slot);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TSignature>
    signal_slot_handle_t Connect(SignalTarget* owner, TSignature&& callback) const
    {
        auto slot = new SlotFull<std::tuple_size_v<callable_argument_types_t<TSignature>>, void, callable_return_type_t<TSignature>, TSignature, TSignalArgs...>(
            const_cast<SignalBase*>(static_cast<const SignalBase*>(this)),
            owner,
            std::move(callback)
        );
        ConnectInternal(slot);

        return signal_slot_handle_t(slot);
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    void Disconnect(signal_slot_handle_t handle) const
    {
        DisconnectInternal(FindSlotByHandle(handle));
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass, typename TOwner, typename ...TMethodArgs>
    void Disconnect(const TOwner* obj, TMethodReturnType (TMethodClass::*callback)(TMethodArgs...)) const
    {
        DisconnectInternal(FindSlot(static_cast<const TMethodClass*>(obj), callback));
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass, typename TOwner, typename ...TMethodArgs>
    void Disconnect(const TOwner* obj, TMethodReturnType (TMethodClass::*callback)(TMethodArgs...) const) const
    {
        DisconnectInternal(FindSlot(static_cast<const TMethodClass*>(obj), callback));
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TFunctionReturnType, typename ...TFunctionArgs>
    void Disconnect(TFunctionReturnType(*callback)(TFunctionArgs...)) const
    {
        DisconnectInternal(FindSlot(callback));
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename ...TFunctionArgs>
    void ConnectOrDisconnect(bool doConnect, TFunctionArgs&& ...args) const
    {
        if (doConnect) {
            Connect(std::move(args)...);
        } else {
            Disconnect(std::move(args)...);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass, typename TOwner, typename ...TMethodArgs>
    bool IsSlotConnected(const TOwner* obj, TMethodReturnType(TMethodClass::*callback)(TMethodArgs...)) const
    {
        return FindSlot(static_cast<const TMethodClass*>(obj), callback) != nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TMethodReturnType, typename TMethodClass, typename TOwner, typename ...TMethodArgs>
    bool IsSlotConnected(const TOwner* obj, TMethodReturnType (TMethodClass::*callback)(TMethodArgs...) const) const
    {
        return FindSlot(static_cast<const TMethodClass*>(obj), callback) != nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    /// \author Kurt Skauen
    ///////////////////////////////////////////////////////////////////////////////

    template <typename TFunctionReturnType, typename ...TFunctionArgs> bool IsSlotConnected(TFunctionReturnType(*callback)(TFunctionArgs...)) const
    {
        return FindSlot(callback)  != nullptr;
    }
};
