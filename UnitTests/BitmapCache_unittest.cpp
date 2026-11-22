// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Bitmap.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>

#include "GUITest.h"

TEST(BitmapCache, SharesRasterStorageThroughIndependentApplicationHandles)
{
    RunGUITest([](PApplication& application)
    {
        const PPath path(PStandardPaths::GetPath(PStandardPath::GUI, "Styles/Skinned/Normal/Button/ButtonNormal.png"));
        Ptr<const PBitmap> first = application.LoadBitmap(path);
        Ptr<const PBitmap> repeated = application.LoadBitmap(path);
        Ptr<PApplication> otherApplication = ptr_new<PApplication>("bitmap_cache_peer");
        otherApplication->SetGUIStyle(nullptr);
        Ptr<const PBitmap> other = otherApplication->LoadBitmap(path);

        ASSERT_NE(first, nullptr);
        ASSERT_NE(repeated, nullptr);
        ASSERT_NE(other, nullptr);
        EXPECT_NE(first->GetHandle(), repeated->GetHandle());
        EXPECT_EQ(first->GetApplication(), &application);
        EXPECT_EQ(other->GetApplication(), ptr_raw_pointer_cast(otherApplication));
        EXPECT_EQ(first->GetBounds(), other->GetBounds());
        EXPECT_EQ(first->GetColorSpace(), other->GetColorSpace());
        EXPECT_EQ(first->GetBytesPerRow(), other->GetBytesPerRow());
        uint8_t firstPixel;
        {
            const PBitmapRasterRO firstRaster = first->LockRaster();
            const PBitmapRasterRO repeatedRaster = repeated->LockRaster();
            const PBitmapRasterRO otherRaster = other->LockRaster();
            ASSERT_NE(firstRaster, nullptr);
            EXPECT_EQ(firstRaster.Get(), repeatedRaster.Get());
            EXPECT_EQ(firstRaster.Get(), otherRaster.Get());
            firstPixel = firstRaster[0];
        }

        first.Reset();
        repeated.Reset();
        application.Sync();
        EXPECT_EQ(other->LockRaster()[0], firstPixel);
        other.Reset();
        otherApplication->Sync();
    });
}
