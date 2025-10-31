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
// Created: 19.10.2025 22:30

#pragma once

#include <map>
#include <functional>
#include <sys/types.h>
#include <RPC/ArgumentDeserializer.h>

class PRPCDispatcher
{
public:
    template<typename TSignature, typename TReturnType, typename TOwner, typename... TArgTypes>
    void AddHandler(int handlerID, TOwner obj, TSignature callback)
    {
        std::function<void(const void* inData, size_t inDataLength, void* outData, size_t outDataLength)> handler = [obj, callback](const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
        {
            if constexpr (std::is_void_v<TReturnType>)
            {
                ArgumentDeserializer<TReturnType, TArgTypes...>::Invoke(inData, inDataLength, [obj, callback](TArgTypes... args)
                    {
                        (obj->*callback)(args...);
                    }
                );
            }
            else
            {
                if (outDataLength != sizeof(TReturnType)) {
                    PERROR_THROW_CODE(PErrorCode::InvalidArg);
                }
                *reinterpret_cast<TReturnType*>(outData) = ArgumentDeserializer<TReturnType, TArgTypes...>::Invoke(inData, inDataLength, [obj, callback](TArgTypes... args) -> TReturnType
                    {
                        return (obj->*callback)(args...);
                    }
                );
            }
        };
        m_RemoteProcedures[handlerID] = handler;
    }

    template<typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(int handlerID, TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...))
    {
        AddHandler<TReturnType(TMethodClass::*)(TArgTypes...), TReturnType, TOwner*, TArgTypes...>(handlerID, obj, callback);
    }
    template<typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(int handlerID, const TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...) const)
    {
        AddHandler<decltype(callback), TReturnType, const TOwner*, TArgTypes...>(handlerID, obj, callback);
    }

    template<typename THandlerDefinition, typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...))
    {
        static_assert(std::is_same_v<std::tuple<TArgTypes...>, typename THandlerDefinition::ArgumentTypes>);
        AddHandler(THandlerDefinition::HandlerID, obj, callback);
    }

    template<typename THandlerDefinition, typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...) const)
    {
        static_assert(std::is_same_v<std::tuple<TArgTypes...>, typename THandlerDefinition::ArgumentTypes>);
        AddHandler(THandlerDefinition::HandlerID, obj, callback);
    }

    template<typename TInvokerOwner, typename TInvokerType, typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(TInvokerType TInvokerOwner::* invoker, TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...))
    {
        AddHandler<typename TInvokerType::Definition>(obj, callback);
    }
    template<typename TInvokerOwner, typename TInvokerType, typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(TInvokerType TInvokerOwner::* invoker, TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...) const)
    {
        AddHandler<typename TInvokerType::Definition>(obj, callback);
    }

    void Dispatch(int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
    {
        auto sig = m_RemoteProcedures.find(request);
        if (sig == m_RemoteProcedures.end()) {
            PERROR_THROW_CODE(PErrorCode::NotImplemented);
        }
        sig->second(inData, inDataLength, outData, outDataLength);
    }

private:
    std::map<int, std::function<void(const void* inData, size_t inDataLength, void* outData, size_t outDataLength)>> m_RemoteProcedures;
};
