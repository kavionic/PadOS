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


