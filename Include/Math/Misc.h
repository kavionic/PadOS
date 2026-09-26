// This file is part of PadOS.
//
// Copyright (c) 2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 16.05.2021 15:15

#pragma once

#include <cmath>
#include <cstdint>

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
