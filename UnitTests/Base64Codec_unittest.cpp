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
// Created: 12.11.2025 23:00

#include <gtest/gtest.h>
#include <Utils/Base64Codec.h>

TEST(PBase64Codec, EncodeDecode)
{
    for (size_t binSize : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 200, 500, 600, 601, 602, 603, 604})
    {
        std::vector<uint8_t> binaryData;

        binaryData.resize(binSize);
        for (size_t i = 0; i < binaryData.size(); ++i) {
            binaryData[i] = uint8_t(rand());
        }
        PString encodedData = PBase64Codec::Encode(binaryData.data(), binaryData.size());
        std::vector<uint8_t> decodedData = PBase64Codec::Decode(encodedData.data(), encodedData.size());

        EXPECT_EQ(binaryData, decodedData);
    }
}
