// This file is part of PadOS.
//
// Copyright (c) 2018-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <functional>
#include <utility>
#include <tuple>
#include <forward_list>

#include "SlotBase.h"

///////////////////////////////////////////////////////////////////////////////
/// Second level of the slot class. Defines return type and arguments.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TReturnType, typename... TSignalArgs>
class Slot : public SlotBase
{
public:
    Slot(SignalBase* targetSignal, SignalTarget* obj) : SlotBase(targetSignal, obj) {}
    virtual TReturnType Call(TSignalArgs... args) const = 0;
};

///////////////////////////////////////////////////////////////////////////////
/// Third level of the slot class. Defines the function type (normal, virtual,
/// const, static, global) and actual number of arguments.
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

// Specialization for class methods.
template<int TUsedArgsCount, typename TSignalTarget, typename TReturnType, typename TSignature, typename... TSignalArgs>
class SlotFull : public Slot<TReturnType, TSignalArgs...>
{
public:
    SlotFull(SignalBase* targetSignal, TSignalTarget* obj, TSignature callback) : Slot<TReturnType, TSignalArgs...>(targetSignal, obj), m_Callback(callback) {}
    
    const TSignature& GetCallback() const { return m_Callback; }

    template<typename TObjectType, typename TFunctionArgs, std::size_t... I>
    TReturnType DoInvoke(TObjectType*, TFunctionArgs&& args, std::index_sequence<I...>) const {
        return (static_cast<TSignalTarget*>(this->m_Object)->*m_Callback)(std::forward<decltype(std::get<I>(args))>(std::get<I>(args))...);
    }
    
    virtual SlotBase* Clone(SignalBase* owningSignal) override
    {
        return new SlotFull(owningSignal, static_cast<TSignalTarget*>(this->m_Object), m_Callback);
    }
    
    virtual TReturnType Call(TSignalArgs... args) const override
    {
        return DoInvoke((TSignalTarget*)nullptr, std::tuple<TSignalArgs...>(args...), std::make_index_sequence<TUsedArgsCount>());
    }
    
private:
    TSignature m_Callback;
};

// Specialization for function pointers, functors and lambdas.
template<int TUsedArgsCount, typename TReturnType, typename TSignature, typename... TSignalArgs>
class SlotFull<TUsedArgsCount, void, TReturnType, TSignature, TSignalArgs...> : public Slot<TReturnType, TSignalArgs...>
{
public:
    SlotFull(SignalBase* targetSignal, SignalTarget* signalTarget, TSignature&& callback) : Slot<TReturnType, TSignalArgs...>(targetSignal, signalTarget), m_Callback(std::move(callback)) {}

    const TSignature& GetCallback() const { return m_Callback; }

    template<typename TFunctionArgs, std::size_t... I>
    TReturnType DoInvoke(TFunctionArgs&& args, std::index_sequence<I...>) const {
        return m_Callback(std::forward<std::decay_t<decltype(std::get<I>(args))>>(std::get<I>(args))...);
    }

    virtual SlotBase* Clone(SignalBase* owningSignal) override
    {
        return new SlotFull(owningSignal, this->m_Object, TSignature(m_Callback));
    }

    virtual TReturnType Call(TSignalArgs... args) const override
    {
        return DoInvoke(std::tuple<TSignalArgs...>(args...), std::make_index_sequence<TUsedArgsCount>());
    }

private:
    TSignature m_Callback;
};
