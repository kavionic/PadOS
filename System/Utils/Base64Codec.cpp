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

#include <assert.h>

#include <Utils/Base64Codec.h>
#include <Utils/Utils.h>

namespace os
{

const uint8_t Base64Codec::g_Base64EncodeAlphabet[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '/'
};

const uint8_t Base64Codec::g_Base64DecodeAlphabet[] =
{
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, //  0-15
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // 16-31
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62,  0, 63, // 32-47
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0, // 48-63
     0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, // 64-79
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0, 63, // 80-95
     0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, // 96-111
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0  // 112-127
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String Base64Codec::Encode(const uint8_t* data, size_t length)
{
    const uint8_t* srcData = static_cast<const uint8_t*>(data);

    const size_t resultLength = ((length + 2) / 3) * 4;

    if (resultLength < length) {
        return String::zero; // size_t overflow.
    }

    String resultBuffer;
    resultBuffer.resize(resultLength);

    uint8_t* dstPtr = reinterpret_cast<uint8_t*>(resultBuffer.data());

    size_t i;
    for (i = 0; length - i >= 3; i += 3)
    {
        *dstPtr++ = EncodeBit00_05(&srcData[i]);
        *dstPtr++ = EncodeBit06_11(&srcData[i]);
        *dstPtr++ = EncodeBit12_17(&srcData[i]);
        *dstPtr++ = EncodeBit18_23(&srcData[i]);
    }

    if ((length - i) != 0)
    {
        *dstPtr++ = EncodeBit00_05(&srcData[i]);
        if ((length - i) == 1)
        {
            *dstPtr++ = EncodeBit06_07(&srcData[i]);
            *dstPtr++ = '=';
        }
        else
        {
            *dstPtr++ = EncodeBit06_11(&srcData[i]);
            *dstPtr++ = EncodeBit12_15(&srcData[i]);
        }
        *dstPtr++ = '=';
    }
    assert(dstPtr == reinterpret_cast<uint8_t*>(&resultBuffer[resultBuffer.size()]));
    return resultBuffer;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> Base64Codec::Decode(const void* data, const size_t length)
{
    const uint8_t* srcData = static_cast<const uint8_t*>(data);

    size_t unpaddedLength = length;
    uint32_t paddingBytes = length & 0x03;

    if (paddingBytes == 0)
    {
        if (length > 1 && srcData[length - 2] == '=') {
            paddingBytes = 2;
        } else if (length > 0 && srcData[length - 1] == '=') {
            paddingBytes = 1;
        }
        if (paddingBytes) {
            unpaddedLength -= 4;
        }
    }
    else
    {
        paddingBytes = 4 - paddingBytes;
        unpaddedLength &= ~0x03;
    }

    const size_t resultLength = (length + 3) / 4 * 3 - paddingBytes;

    std::vector<uint8_t> resultBuffer(resultLength);
    uint8_t* dstPtr = resultBuffer.data();

    for (size_t i = 0; i < unpaddedLength; i += 4)
    {
        const uint32_t value  = DecodeCharacter(srcData[i + 0]) << 18 |
                                DecodeCharacter(srcData[i + 1]) << 12 |
                                DecodeCharacter(srcData[i + 2]) << 6 |
                                DecodeCharacter(srcData[i + 3]);
        *dstPtr++ = uint8_t(value >> 16);
        *dstPtr++ = uint8_t(value >> 8);
        *dstPtr++ = uint8_t(value);
    }
    if (paddingBytes == 2)
    {
        const uint32_t value  = DecodeCharacter(srcData[unpaddedLength + 0]) << 18 |
                                DecodeCharacter(srcData[unpaddedLength + 1]) << 12;
        
        *dstPtr++ = uint8_t(value >> 16);
    }
    else if (paddingBytes == 1)
    {
        const uint32_t value  = DecodeCharacter(srcData[unpaddedLength + 0]) << 18 |
                                DecodeCharacter(srcData[unpaddedLength + 1]) << 12 |
                                DecodeCharacter(srcData[unpaddedLength + 2]) << 6;

        *dstPtr++ = uint8_t(value >> 16);
        *dstPtr++ = uint8_t(value >> 8);
    }
    assert(dstPtr == &resultBuffer[resultBuffer.size()]);

    return resultBuffer;
}

} // namespace os

namespace unit_test
{
using namespace os;

void TestBase64Codec()
{
    for (size_t binSize : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 200, 500, 600, 601, 602, 603, 604})
    {
        std::vector<uint8_t> binaryData;

        binaryData.resize(binSize);
        for (size_t i = 0; i < binaryData.size(); ++i) {
            binaryData[i] = uint8_t(rand());
        }
        String encodedData = Base64Codec::Encode(binaryData.data(), binaryData.size());
        std::vector<uint8_t> decodedData = Base64Codec::Decode(encodedData.data(), encodedData.size());

        _EXPECT_TRUE(binaryData == decodedData);
    }
}

} // namespace unit_test