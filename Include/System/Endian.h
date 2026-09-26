// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 26.05.2022 15:30

#include <stdint.h>

#pragma once


#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

template<typename T> constexpr T PHostToNetwork(T value)
{
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        return __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        return __builtin_bswap64(value);
    }
}

template<typename T> T PNetworkToHost(T value)
{
    if constexpr (sizeof(T) == 1) {
        return value;
    } else if constexpr (sizeof(T) == 2) {
        return __builtin_bswap16(value);
    } else if constexpr (sizeof(T) == 4) {
        return __builtin_bswap32(value);
    } else if constexpr (sizeof(T) == 8) {
        return __builtin_bswap64(value);
    }
}

template<typename T> T PHostToLittleEndian(T value) { return value; }
template<typename T> T PLittleEndianToHost(T value) { return value; }

#else

inline uint16_t PHostToNetwork(uint16_t value) { return value; }
inline uint32_t PNetworkToHost(uint32_t value) { return value; }

inline uint16_t PNetworkToHost(uint16_t value) { return value; }
inline uint32_t PHostToNetwork(uint32_t value) { return value; }

inline uint16_t PHostToLittleEndian(uint16_t value) { return __builtin_bswap16(value); }
inline uint32_t PHostToLittleEndian(uint32_t value) { return __builtin_bswap32(value); }

inline uint16_t PLittleEndianToHost(uint16_t value) { return __builtin_bswap16(value); }
inline uint32_t PLittleEndianToHost(uint32_t value) { return __builtin_bswap32(value); }

#endif
