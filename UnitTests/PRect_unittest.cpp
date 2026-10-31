// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <Math/Rect.h>

TEST(PRect, CenterIncludesTheOrigin)
{
    static_assert(PRect(10.0f, 20.0f, 30.0f, 60.0f).Center() == PPoint(20.0f, 40.0f));
    EXPECT_EQ(PRect(0.0f, 0.0f, 80.0f, 40.0f).Center(), PPoint(40.0f, 20.0f));
    EXPECT_EQ(PRect(10.0f, 20.0f, 30.0f, 60.0f).Center(), PPoint(20.0f, 40.0f));
    EXPECT_EQ(PRect(-60.0f, -40.0f, -20.0f, -10.0f).Center(), PPoint(-40.0f, -25.0f));
    EXPECT_EQ(PRect(-10.0f, -20.0f, 30.0f, 40.0f).Center(), PPoint(10.0f, 10.0f));
    EXPECT_EQ(PRect(1.5f, -2.5f, 5.5f, 6.5f).Center(), PPoint(3.5f, 2.0f));
    EXPECT_EQ(PRect(4.0f, 7.0f, 4.0f, 7.0f).Center(), PPoint(4.0f, 7.0f));
}

TEST(PRect, CenterTracksTranslation)
{
    const PRect rect(10.0f, -20.0f, 30.0f, 40.0f);
    const PPoint offset(-50.0f, 15.0f);
    EXPECT_EQ((rect + offset).Center(), rect.Center() + offset);
    EXPECT_EQ((rect - offset).Center(), rect.Center() - offset);
}

TEST(PRect, CenterMatchesCenteredFactory)
{
    constexpr PPoint center(12.5f, -3.5f);
    constexpr PRect rect = PRect::Centered(center, PPoint(7.0f, 11.0f));
    static_assert(rect.Center() == center);
    EXPECT_EQ(rect.Center(), center);
}
