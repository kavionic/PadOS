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

#include <tuple>
#include <System/ExceptionHandling.h>
#include <RPC/ArgumentPacker.h>

template<typename R, typename... ARGS>
class PArgumentDeserializer
{
public:
    using ReturnType = R;

    typedef std::tuple<std::decay_t<ARGS>...> ArgTuple_t;
    PArgumentDeserializer() {}

    template<typename TSignature>
    static ReturnType Invoke(const void* data, size_t length, TSignature&& callback)
    {
        using Indices = std::make_index_sequence<std::tuple_size<std::decay_t<ArgTuple_t>>::value>;
        return Invoke(data, length, callback, Indices());
    }

private:
    template<typename TSignature, std::size_t... I>
    static ReturnType Invoke(const void* data, size_t length, TSignature&& callback, std::index_sequence<I...>)
    {
        ArgTuple_t argPack;
        UnpackArgs<0, std::decay_t<ARGS>...>(argPack, data, length);
        return callback(std::get<I>(argPack)...);
    }

    template<int I>
    static ssize_t UnpackArgs(ArgTuple_t& tuple, const void* data, size_t length) { return 0; }

    template<int I, typename FIRST, typename... REST>
    static ssize_t UnpackArgs(ArgTuple_t& tuple, const void* data, size_t length)
    {
        ssize_t result = PArgumentPacker<FIRST>::Read(data, length, &std::get<I>(tuple));
        if (result >= 0)
        {
            const size_t consumed = align_argument_size(result);
            data = reinterpret_cast<const uint8_t*>(data) + consumed;
            result = UnpackArgs<I + 1, REST...>(tuple, data, length - consumed);
            if (result >= 0)
            {
                return result + consumed;
            }
        }
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
};

template<typename TReturnType, typename... TArgTypes>
class PArgumentDeserializer<TReturnType(TArgTypes...)> : public PArgumentDeserializer<TArgTypes...> {};
