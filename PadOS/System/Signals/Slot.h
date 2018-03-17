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

#include <forward_list>
#include "SlotBase.h"


#include <utility>
#include <tuple>

///////////////////////////////////////////////////////////////////////////////
/// Second level of the slot class. Defines return type and arguments.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename R, typename... ARGS_FULL>
class Slot : public SlotBase
{
public:
    Slot(SignalBase* targetSignal, SignalTarget* obj) : SlotBase(targetSignal, obj) {}
    virtual R Call(ARGS_FULL... args) const = 0;
};

///////////////////////////////////////////////////////////////////////////////
/// Third level of the slot class. Defines the function type (normal, virtual,
/// const, static, global) and actual number of arguments.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<int USED_ARGS, typename fT, typename R, typename SIGNATURE, typename... ARGS_FULL>
class SlotFull : public Slot<R, ARGS_FULL...>
{
public:
    SlotFull(SignalBase* targetSignal, fT* obj, SIGNATURE callback) : Slot<R, ARGS_FULL...>(targetSignal, obj), m_Callback(callback) {}
    
    template<typename OBJ_TYPE, typename ARGS, std::size_t... I>
    R DoInvoke(OBJ_TYPE*, ARGS&& args, std::index_sequence<I...>) const {
        return (static_cast<fT*>(this->m_Object)->*m_Callback)(std::forward<decltype(std::get<I>(args))>(std::get<I>(args))...);
    }
    
    // Specialization for non-member functions. SignalTarget does not have any slots itself, so we use that as a key to trigger this overload.
    template<typename ARGS, std::size_t... I>
    R DoInvoke(SignalTarget*, ARGS&& args, std::index_sequence<I...>) const {
        return m_Callback(std::forward<std::decay_t<decltype(std::get<I>(args))>>(std::get<I>(args))...);
    }

/*    template<typename OBJ_TYPE>
    void* DoGetCallpackAddress(OBJ_TYPE*) const {
        return (void*)*(static_cast<fT*>(this->m_Object)->*m_Callback);
    }
    //template<>
    void* DoGetCallpackAddress(SignalTarget*) const {
        return (void*)m_Callback;
    }

    virtual void* GetCallbackAddress() const override { return DoGetCallpackAddress((fT*)nullptr); }
*/
    virtual SlotBase* Clone(SignalBase* owningSignal) override
    {
        return new SlotFull(owningSignal, static_cast<fT*>(this->m_Object), m_Callback);
    }
    
    virtual R Call(ARGS_FULL... args) const override
    {
        return DoInvoke((fT*)nullptr, std::tuple<ARGS_FULL...>(/*std::forward<ARGS_FULL>(*/args/*)*/...), std::make_index_sequence<USED_ARGS>());
    }
    
private:
    SIGNATURE m_Callback;
};

