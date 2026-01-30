// This file is part of PadOS.
//
// Copyright (C) 2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 16.05.2021 15:15

#pragma once

#include <cmath>

namespace PMath
{

template<typename T> constexpr T square(T value) { return value * value; }

template<typename T> constexpr bool is_almost_zero(T value, T tolerance = T(0.0000001)) { return fabs(value) <= tolerance; }

inline uint64_t pow_u64(uint64_t base, int exp)
{
    uint64_t result = 1;
    for (int i = 0; i < exp; ++i) result *= base;
    return result;
}

inline bool checked_mul_u64(uint64_t& inOut, uint64_t factor, uint64_t limit)
{
    if (factor == 0) {
        inOut = 0; return true;
    }
    if (inOut > limit / factor) {
        return false;
    }
    inOut *= factor;
    return true;
}

template<typename TIn, typename TOut> TOut signed_to_unsigned_abs(TIn value)
{
    return (value < 0) ? TOut(-(value + 1)) + 1 : TOut(value);
}

inline uint8_t  signed_to_unsigned_abs(int8_t value)  { return signed_to_unsigned_abs<int8_t, uint8_t>(value); }
inline uint16_t signed_to_unsigned_abs(int16_t value) { return signed_to_unsigned_abs<int16_t, uint16_t>(value); }
inline uint32_t signed_to_unsigned_abs(int32_t value) { return signed_to_unsigned_abs<int32_t, uint32_t>(value); }
inline uint64_t signed_to_unsigned_abs(int64_t value) { return signed_to_unsigned_abs<int64_t, uint64_t>(value); }

} // namespace PMath
