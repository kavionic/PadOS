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
#include <GUI/Color.h>
#include <GUI/StandardColorsDefinition.h>


TEST(NamedColors, TableIntegrity)
{
    for (size_t i = 1; i < PStandardColorsTable.size(); ++i)
    {
        {
            SCOPED_TRACE(std::format("Hash collision! Both {} and {} have hash {:#08x}", i - 1, i, uint32_t(PStandardColorsTable[i].NameID)));
            EXPECT_NE(PStandardColorsTable[i - 1].NameID, PStandardColorsTable[i].NameID);
        }
        {
            SCOPED_TRACE(std::format("Bad named color table sorting! {}:{:#08x} > {}:{:#08x}", i - 1, uint32_t(PStandardColorsTable[i - 1].NameID), i, uint32_t(PStandardColorsTable[i].NameID)));
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
