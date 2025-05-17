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
// Created: 14.05.2025 20:30

#pragma once

#include <vector>
#include <stdint.h>

#include <Utils/String.h>

namespace os
{

class Base64Codec
{
public:
    static os::String Encode(const uint8_t* data, size_t length);
    static std::vector<uint8_t> Decode(const void* data, const size_t length);

    static uint8_t EncodeBit00_05(const uint8_t* srcData) { return g_Base64EncodeAlphabet[srcData[0] >> 2]; }
    static uint8_t EncodeBit06_07(const uint8_t* srcData) { return g_Base64EncodeAlphabet[(srcData[0] & 0x03) << 4]; }
    static uint8_t EncodeBit06_11(const uint8_t* srcData) { return g_Base64EncodeAlphabet[((srcData[0] & 0x03) << 4) | (srcData[1] >> 4)]; }
    static uint8_t EncodeBit12_15(const uint8_t* srcData) { return g_Base64EncodeAlphabet[(srcData[1] & 0x0f) << 2]; }
    static uint8_t EncodeBit12_17(const uint8_t* srcData) { return g_Base64EncodeAlphabet[((srcData[1] & 0x0f) << 2) | (srcData[2] >> 6)]; }
    static uint8_t EncodeBit18_23(const uint8_t* srcData) { return g_Base64EncodeAlphabet[srcData[2] & 0x3f]; }

    static uint32_t DecodeCharacter(uint8_t character) { return g_Base64DecodeAlphabet[(character & 0x7f)]; }

private:
    static const uint8_t g_Base64EncodeAlphabet[64];
    static const uint8_t g_Base64DecodeAlphabet[128];

};

} // namespace os


namespace unit_test
{
void TestBase64Codec();
}
