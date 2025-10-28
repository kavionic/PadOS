// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 20.10.2025 20:00

#pragma once

#include <functional>
#include <inttypes.h>

#include <RPC/ArgumentSerializer.h>



template<typename TReturnType, typename... TArgTypes>
class PRPCInvoker
{
public:
    using CallbackSignature = std::function<void(const void* inData, size_t inDataLength, void* outData, size_t outDataLength)>;

    PRPCInvoker(CallbackSignature callback) : m_Callback(callback) {}

    TReturnType operator()(TArgTypes... args)
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
private:
    CallbackSignature m_Callback;
    int m_DeviceFD = -1;
};

template<typename TReturnType, typename... TArgTypes>
class PRPCInvoker<TReturnType(TArgTypes...)> : public PRPCInvoker<TReturnType, TArgTypes...>
{
public:
    PRPCInvoker(PRPCInvoker<TReturnType, TArgTypes...>::CallbackSignature&& callback) : PRPCInvoker<TReturnType, TArgTypes...>(callback) {}
};
