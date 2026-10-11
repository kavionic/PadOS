// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.10.2025 20:00

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <type_traits>
#include <utility>

#include <RPC/ArgumentSerializer.h>
#include <System/ExceptionHandling.h>

struct PRPCStackAllocator
{
};

struct PRPCNoTransmission
{
};

template<
    bool TIsConst,
    typename TSignature,
    typename TAllocator = PRPCStackAllocator,
    typename TCallback = std::function<void(const void*, size_t, void*, size_t)>>
class PRPCInvoker;

template<bool TIsConst, typename TAllocator, typename TCallback, typename TReturnType, typename... TArgTypes>
class PRPCInvoker<TIsConst, TReturnType(TArgTypes...), TAllocator, TCallback>
{
public:
    using CallbackSignature = TCallback;
    using Serializer = PArgumentSerializer<TReturnType, TArgTypes...>;

    static_assert(!std::is_same_v<TCallback, PRPCNoTransmission> || std::is_void_v<TReturnType>,
        "An RPC without a transmission callback cannot return a value.");

    explicit PRPCInvoker(TCallback callback = {}, TAllocator allocator = {})
        : m_Callback(std::move(callback))
        , m_Allocator(std::move(allocator))
    {
    }

    static constexpr size_t GetSize(TArgTypes... args) noexcept { return Serializer::GetSize(args...); }
    static constexpr size_t GetFixedSize() noexcept requires Serializer::HAS_FIXED_SIZE { return Serializer::GetFixedSize(); }

    TReturnType operator()(TArgTypes... args) requires (!TIsConst)
    {
        return Invoke(std::forward<TArgTypes>(args)...);
    }

    TReturnType operator()(TArgTypes... args) const requires TIsConst
    {
        return Invoke(std::forward<TArgTypes>(args)...);
    }

    TReturnType operator()(void* buffer, TArgTypes... args) requires (!TIsConst)
    {
        return Invoke(buffer, GetSize(args...), std::forward<TArgTypes>(args)...);
    }

    TReturnType operator()(void* buffer, TArgTypes... args) const requires TIsConst
    {
        return Invoke(buffer, GetSize(args...), std::forward<TArgTypes>(args)...);
    }

    TReturnType operator()(void* buffer, size_t bufferSize, TArgTypes... args) requires (!TIsConst)
    {
        return Invoke(buffer, bufferSize, std::forward<TArgTypes>(args)...);
    }

    TReturnType operator()(void* buffer, size_t bufferSize, TArgTypes... args) const requires TIsConst
    {
        return Invoke(buffer, bufferSize, std::forward<TArgTypes>(args)...);
    }

protected:
    void* AllocBuffer(size_t size) const requires (!std::is_same_v<TAllocator, PRPCStackAllocator>)
    {
        void* buffer = m_Allocator(size);
        if (buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::NOMEM);
        }
        return buffer;
    }

private:
    TReturnType Invoke(TArgTypes... args) const
    {
        const size_t size = GetSize(args...);
        if constexpr (std::is_same_v<TAllocator, PRPCStackAllocator>)
        {
            alignas(std::max_align_t) uint8_t argBuffer[(size != 0) ? size : 1];
            return Invoke(argBuffer, size, std::forward<TArgTypes>(args)...);
        }
        else
        {
            return Invoke(AllocBuffer(size), size, std::forward<TArgTypes>(args)...);
        }
    }

    TReturnType Invoke(void* buffer, size_t bufferSize, TArgTypes... args) const
    {
        const size_t argumentSize = GetSize(args...);
        if (argumentSize > bufferSize || (buffer == nullptr && bufferSize != 0)) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        if (Serializer::WriteArg(buffer, argumentSize, std::forward<TArgTypes>(args)...) < 0) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        if constexpr (!std::is_same_v<TCallback, PRPCNoTransmission>)
        {
            if constexpr (std::is_void_v<TReturnType>)
            {
                m_Callback(buffer, bufferSize, nullptr, 0);
            }
            else
            {
                TReturnType returnValue;
                m_Callback(buffer, bufferSize, &returnValue, sizeof(returnValue));
                return returnValue;
            }
        }
    }

    [[no_unique_address]] TCallback m_Callback;
    [[no_unique_address]] TAllocator m_Allocator;
};
