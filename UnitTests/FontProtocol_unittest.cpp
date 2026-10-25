// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <gtest/gtest.h>
#include <ApplicationServer/FontProtocol.h>

TEST(FontProtocol, CopiesEveryFontFieldWithoutRetainingClientStrings)
{
    PFontDescription original;
    original.Family = "Family with UTF-8 \xc3\x85";
    original.Style = "Bold Oblique";
    original.PointSize = 19.5f;
    original.Rotation = 270.0f;
    original.Shear = 75.0f;
    original.RenderMode = PFontRenderMode::Monochrome;
    original.Hinting = false;
    const PFontDescription expected = original;
    alignas(uint32_t) std::array<uint8_t, 256> buffer{};
    const size_t length = PArgumentPacker<PFontDescription>::GetSize(original);
    ASSERT_LE(length, buffer.size());
    ASSERT_EQ(PArgumentPacker<PFontDescription>::Write(original, buffer.data(), length), ssize_t(length));
    original.Family.clear();
    original.Style.clear();
    PFontDescription restored;
    ASSERT_EQ(PArgumentPacker<PFontDescription>::Read(buffer.data(), length, &restored), ssize_t(length));
    EXPECT_EQ(restored, expected);
}

TEST(FontProtocol, RejectsTruncatedFieldsAndAlignmentPadding)
{
    const PFontDescription original;
    alignas(uint32_t) std::array<uint8_t, 256> buffer{};
    const size_t length = PArgumentPacker<PFontDescription>::GetSize(original);
    ASSERT_LE(length, buffer.size());
    ASSERT_EQ(PArgumentPacker<PFontDescription>::Write(original, buffer.data(), length), ssize_t(length));
    PFontDescription restored;
    EXPECT_EQ(PArgumentPacker<PFontDescription>::Read(buffer.data(), sizeof(uint32_t), &restored), -1);
    EXPECT_EQ(PArgumentPacker<PFontDescription>::Read(buffer.data(), length - 1, &restored), -1);
    EXPECT_EQ(PArgumentPacker<PFontDescription>::Read(buffer.data(), length - sizeof(uint32_t), &restored), -1);
}
