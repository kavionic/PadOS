// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 12.11.2025 23:00

#include <gtest/gtest.h>
#include <GUI/Color.h>
#include <GUI/StandardColorsDefinition.h>


TEST(PNamedColors, TableIntegrity)
{
    for (size_t i = 1; i < PStandardColorsTable.size(); ++i)
    {
        {
            SCOPED_TRACE(PString::format_string("Hash collision! Both {} and {} have hash {:#08x}", i - 1, i, uint32_t(PStandardColorsTable[i].NameID)));
            EXPECT_NE(PStandardColorsTable[i - 1].NameID, PStandardColorsTable[i].NameID);
        }
        {
            SCOPED_TRACE(PString::format_string("Bad named color table sorting! {}:{:#08x} > {}:{:#08x}", i - 1, uint32_t(PStandardColorsTable[i - 1].NameID), i, uint32_t(PStandardColorsTable[i].NameID)));
            EXPECT_LT(PStandardColorsTable[i - 1].NameID, PStandardColorsTable[i].NameID);
        }
    }
    EXPECT_EQ(PColor::FromColorID(PNamedColors::aliceblue).GetColor32(),    0xfff0f8ff);
    EXPECT_EQ(PColor::FromColorID(PNamedColors::antiquewhite).GetColor32(), 0xfffaebd7);
    EXPECT_EQ(PColor::FromColorID(PNamedColors::yellow).GetColor32(),       0xffffff00);
    EXPECT_EQ(PColor::FromColorID(PNamedColors::yellowgreen).GetColor32(),  0xff9acd32);

    EXPECT_EQ(PColor::FromColorName("aliceblue").GetColor32(),      0xfff0f8ff);
    EXPECT_EQ(PColor::FromColorName("YellowGreen").GetColor32(),    0xff9acd32);
}
