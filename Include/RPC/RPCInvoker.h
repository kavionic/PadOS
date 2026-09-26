// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.10.2025 20:00

#pragma once

#include <functional>
#include <inttypes.h>

#include <RPC/ArgumentSerializer.h>



template<bool TIsConst, typename TReturnType, typename... TArgTypes>
class PRPCInvoker
{
public:
    using CallbackSignature = std::function<void(const void* inData, size_t inDataLength, void* outData, size_t outDataLength)>;

    PRPCInvoker(CallbackSignature callback) : m_Callback(callback) {}

    template<bool TOIsConst = TIsConst>
    std::enable_if_t<!TOIsConst, TReturnType> operator()(TArgTypes... args)
    {
        return Invoke(std::forward<TArgTypes>(args)...);
    }

    template<bool TOIsConst = TIsConst>
    std::enable_if_t<TOIsConst, TReturnType> operator()(TArgTypes... args) const
    {
        return Invoke(std::forward<TArgTypes>(args)...);
    }

private:
    TReturnType Invoke(TArgTypes... args) const
    {
        uint8_t argBuffer[PArgumentSerializer<TReturnType, TArgTypes...>::GetSize(args...)];

        PArgumentSerializer<TReturnType, TArgTypes...>::WriteArg(argBuffer, sizeof(argBuffer), args...);
        if constexpr (std::is_void_v<TReturnType>)
        {
            m_Callback(argBuffer, sizeof(argBuffer), nullptr, 0);
        }
        else
        {
            TReturnType returnValue;
            m_Callback(argBuffer, sizeof(argBuffer), &returnValue, sizeof(returnValue));
            return returnValue;
        }
    }

    CallbackSignature m_Callback;
    int m_DeviceFD = -1;
};

template<bool TIsConst, typename TReturnType, typename... TArgTypes>
class PRPCInvoker<TIsConst, TReturnType(TArgTypes...)> : public PRPCInvoker<TIsConst, TReturnType, TArgTypes...>
{
public:
    PRPCInvoker(PRPCInvoker<TIsConst, TReturnType, TArgTypes...>::CallbackSignature&& callback) : PRPCInvoker<TIsConst, TReturnType, TArgTypes...>(callback) {}
};
