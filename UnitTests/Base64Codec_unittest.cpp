// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
