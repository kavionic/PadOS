// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 19.10.2025 22:30

#pragma once

#include <vector>
#include <functional>
#include <sys/types.h>
#include <RPC/ArgumentDeserializer.h>

class PRPCDispatcher
{
public:
    template<typename TSignature, typename TReturnType, typename TOwner, bool TWithPayload = false, typename... TArgTypes>
    void AddHandler(int handlerID, TOwner obj, TSignature callback)
    {
        if (handlerID < 0) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        const auto handler = [obj, callback](const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
        {
            const auto invoke = [obj, callback](auto&&... args) -> TReturnType
            {
                return (obj->*callback)(std::forward<decltype(args)>(args)...);
            };
            if constexpr (std::is_void_v<TReturnType>)
            {
                PArgumentDeserializer<TReturnType, TArgTypes...>::template Invoke<TWithPayload>(inData, inDataLength, invoke);
            }
            else
            {
                if (outData == nullptr || outDataLength != sizeof(TReturnType)) {
                    PERROR_THROW_CODE(PErrorCode::INVAL);
                }
                *static_cast<TReturnType*>(outData) =
                    PArgumentDeserializer<TReturnType, TArgTypes...>::template Invoke<TWithPayload>(inData, inDataLength, invoke);
            }
        };
        const size_t index = size_t(handlerID);
        if (index >= m_RemoteProcedures.size()) {
            m_RemoteProcedures.resize(index + 1, HandleUnknownProcedure);
        }
        m_RemoteProcedures[index] = handler;
    }

    template<typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(int handlerID, TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...))
    {
        AddHandler<TReturnType(TMethodClass::*)(TArgTypes...), TReturnType, TOwner*, false, TArgTypes...>(
            handlerID,
            obj,
            callback);
    }
    template<typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(int handlerID, const TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...) const)
    {
        AddHandler<decltype(callback), TReturnType, const TOwner*, false, TArgTypes...>(handlerID, obj, callback);
    }

    template<typename THandlerDefinition, typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...))
    {
        AddDefinedHandler<THandlerDefinition, TReturnType, std::tuple<TArgTypes...>>(
            obj,
            callback,
            std::type_identity<typename THandlerDefinition::ArgumentTypes>());
    }

    template<typename THandlerDefinition, typename TReturnType, typename TMethodClass, typename TOwner, typename... TArgTypes>
    void AddHandler(TOwner* obj, TReturnType(TMethodClass::* callback)(TArgTypes...) const)
    {
        AddDefinedHandler<THandlerDefinition, TReturnType, std::tuple<TArgTypes...>>(
            obj,
            callback,
            std::type_identity<typename THandlerDefinition::ArgumentTypes>());
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
        const size_t index = size_t(request);
        if (index >= m_RemoteProcedures.size()) {
            PERROR_THROW_CODE(PErrorCode::NOSYS);
        }
        m_RemoteProcedures[index](inData, inDataLength, outData, outDataLength);
    }

private:
    static void HandleUnknownProcedure(const void*, size_t, void*, size_t)
    {
        PERROR_THROW_CODE(PErrorCode::NOSYS);
    }

    template<
        typename TDefinition,
        typename TReturnType,
        typename TCallbackArguments,
        typename TOwner,
        typename TSignature,
        typename... TArgTypes>
    void AddDefinedHandler(TOwner obj, TSignature callback, std::type_identity<std::tuple<TArgTypes...>>)
    {
        static_assert(std::is_same_v<TReturnType, typename TDefinition::ReturnType>);
        constexpr bool WITH_PAYLOAD =
            std::is_same_v<TCallbackArguments, std::tuple<std::span<const std::byte>, TArgTypes...>>;
        static_assert(WITH_PAYLOAD || std::is_same_v<TCallbackArguments, std::tuple<TArgTypes...>>);
        AddHandler<TSignature, TReturnType, TOwner, WITH_PAYLOAD, TArgTypes...>(TDefinition::HandlerID, obj, callback);
    }

    std::vector<std::function<void(const void*, size_t, void*, size_t)>> m_RemoteProcedures;
};
