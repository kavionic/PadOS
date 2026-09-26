// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 20.10.2025 20:00

#pragma once

#include <RPC/ArgumentPacker.h>

template<typename R, typename... ARGS>
class PArgumentSerializer
{
public:
    static constexpr size_t AccumulateSize() noexcept { return 0; }

    template<typename FIRST>
    static constexpr size_t AccumulateSize(FIRST&& first) noexcept { return align_argument_size(first); }

    template<typename FIRST, typename... REST>
    static constexpr size_t AccumulateSize(FIRST&& first, REST&&... rest) noexcept { return align_argument_size(first) + AccumulateSize<REST...>(std::forward<REST>(rest)...); }

    static constexpr ssize_t WriteArg(void* buffer, size_t length) noexcept { return 0; }

    template<typename FIRST>
    static ssize_t WriteArg(void* buffer, size_t length, FIRST&& first)
    {
        ssize_t result = PArgumentPacker<std::decay_t<FIRST>>::Write(std::forward<FIRST>(first), buffer, length);
        if (result >= 0) {
            return align_argument_size(result);
        }
        return -1;
    }
    template<typename FIRST, typename... REST>
    static ssize_t WriteArg(void* buffer, size_t length, FIRST&& first, REST&&... rest)
    {
        ssize_t result = PArgumentPacker<std::decay_t<FIRST>>::Write(std::forward<FIRST>(first), buffer, length);
        if (result >= 0)
        {
            const size_t consumed = align_argument_size(result);
            result = WriteArg(reinterpret_cast<uint8_t*>(buffer) + consumed, length - consumed, std::forward<REST>(rest)...);
            return (result >= 0) ? (result + consumed) : -1;
        }
        return -1;
    }

    static constexpr size_t GetSize(ARGS... args) noexcept { return AccumulateSize(PArgumentPacker<std::decay_t<ARGS>>::GetSize(args)...); }
private:
};


