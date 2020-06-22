// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 20.06.2020 19:47

#pragma once

#include <stdint.h>
#include <stddef.h>

namespace os
{

enum class HashAlgorithm
{
//    CRC16,
    CRC32
};

class HashCalculator
{
public:
    HashCalculator() {}
    HashCalculator(HashAlgorithm algorithm) { m_CRC = 0xffffffff; }

    void Start(HashAlgorithm algorithm) { m_CRC = 0xffffffff; }
    void AddData(const void* data, size_t length);
    uint32_t Finalize() { return ~m_CRC; }

private:
    uint32_t m_CRC = 0;

};

}
