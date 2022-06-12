// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 26.05.2022 15:30

#include <stdint.h>

namespace os
{

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

inline uint16_t HostToNetwork(uint16_t value) { return __builtin_bswap16(value); }
inline uint32_t HostToNetwork(uint32_t value) { return __builtin_bswap32(value); }

inline uint16_t NetworkToHost(uint16_t value) { return __builtin_bswap16(value); }
inline uint32_t NetworkToHost(uint32_t value) { return __builtin_bswap32(value); }

inline uint16_t HostToLittleEndian(uint16_t value) { return value; }
inline uint32_t HostToLittleEndian(uint32_t value) { return value; }

inline uint16_t LittleEndianToHost(uint16_t value) { return value; }
inline uint32_t LittleEndianToHost(uint32_t value) { return value; }

#else

inline uint16_t HostToNetwork(uint16_t value) { return value; }
inline uint32_t NetworkToHost(uint32_t value) { return value; }

inline uint16_t NetworkToHost(uint16_t value) { return value; }
inline uint32_t HostToNetwork(uint32_t value) { return value; }

inline uint16_t HostToLittleEndian(uint16_t value) { return __builtin_bswap16(value); }
inline uint32_t HostToLittleEndian(uint32_t value) { return __builtin_bswap32(value); }

inline uint16_t LittleEndianToHost(uint16_t value) { return __builtin_bswap16(value); }
inline uint32_t LittleEndianToHost(uint32_t value) { return __builtin_bswap32(value); }

#endif

} // namespace os