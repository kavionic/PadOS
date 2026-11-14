// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <type_traits>
#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/View.h>
#include <System/ExceptionHandling.h>

#include "GUITest.h"

TEST(SkinnableGUIStyle, UnconfiguredStyleDelegatesToDefaultButtons)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>();
        application.SetGUIStyle(style);
        EXPECT_EQ(application.GetGUIStyle(), Ptr<const PGUIStyle>(style));
        Ptr<PView> target = ptr_new<PView>("skin_test", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 160, 60));
        PButtonStyleData data;
        data.Label = "Default button";
        PPoint defaultMinimum;
        PPoint defaultMaximum;
        PDefaultGUIStyle defaultStyle;
        defaultStyle.ButtonGetPreferredSize(*target, data, &defaultMinimum, &defaultMaximum);
        PPoint minimum;
        PPoint maximum;
        style->ButtonGetPreferredSize(*target, data, &minimum, &maximum);
        EXPECT_EQ(minimum, defaultMinimum);
        EXPECT_EQ(maximum, defaultMaximum);
        style->ButtonPaint(*target, target->GetBounds(), true, data);
        EXPECT_NE(target->GetBackgroundBrush(), nullptr);
        EXPECT_NE(target->GetBrush(), nullptr);
        EXPECT_EQ(target->GetBackgroundBrush(), application.GetStandardBrush(PStandardColorID::ButtonBackground));
        EXPECT_EQ(target->GetBrush(), target->GetBackgroundBrush());
        application.SetGUIStyle(nullptr);
        EXPECT_NE(application.GetGUIStyle(), nullptr);
    });
}

TEST(SkinnableGUIStyle, StatePrecedenceFallbackAndLabelLayoutAreStable)
{
    RunGUITest([](PApplication& application)
    {
        PButtonSkin skin;
        for (size_t index = 0; index < skin.States.size(); ++index)
        {
            skin.States[index].Background = ptr_new<PBrush>(PColor(uint8_t(index * 50), 0, 0));
            skin.CheckedStates[index].Background = ptr_new<PBrush>(PColor(0, uint8_t(index * 50), 0));
        }
        PSkinnableGUIStyle style(skin);
        Ptr<PView> target = ptr_new<PView>("skin_states", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 160, 60));
        PButtonStyleData data;
        data.Label = "Stable size";
        PPoint normalMinimum;
        PPoint normalMaximum;
        style.ButtonGetPreferredSize(*target, data, &normalMinimum, &normalMaximum);
        EXPECT_EQ(normalMinimum, data.LabelSize + PPoint(16, 8));
        for (bool checked : {false, true})
        {
            data.IsChecked = checked;
            for (size_t index = 0; index < skin.States.size(); ++index)
            {
                data.IsPointerOver = index >= 1;
                data.IsPressed = index >= 2;
                const bool enabled = index != 3;
                style.ButtonPaint(*target, target->GetBounds(), enabled, data);
                const PButtonSkinAppearance& appearance = checked ? skin.CheckedStates[index] : skin.States[index];
                EXPECT_EQ(target->GetBackgroundBrush(), appearance.Background);
                EXPECT_EQ(target->GetBrush(), nullptr);
                EXPECT_EQ(target->GetFgColor(), appearance.LabelColor);
                PPoint minimum;
                PPoint maximum;
                style.ButtonGetPreferredSize(*target, data, &minimum, &maximum);
                EXPECT_EQ(minimum, normalMinimum);
                EXPECT_EQ(maximum, normalMaximum);
            }
        }
        skin.CheckedStates[std::to_underlying(PButtonSkinState::Disabled)].Background = nullptr;
        PSkinnableGUIStyle fallback(skin);
        fallback.ButtonPaint(*target, target->GetBounds(), false, data);
        EXPECT_EQ(target->GetBackgroundBrush(), skin.States[0].Background);
    });
}

TEST(SkinnableGUIStyle, SliceBordersParticipateInSizeAndBrushChangesPreserveOrigins)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(32, 24, PEColorSpace::RGB32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster().Get()), 32 * 24, PColor(80, 120, 160).GetColor32());
        PButtonSkin skin;
        skin.LabelPadding = PRect(1.0f);
        skin.States[0].Background = ptr_new<PBrush>(PColor(0, 0, 0));
        Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(15, 11, 17, 13));
        skin.CheckedStates[2].Background = ptr_new<PBrush>(image);
        PSkinnableGUIStyle style(skin);
        Ptr<PView> target = ptr_new<PView>("skin_bindings", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        PButtonStyleData data;
        PPoint minimum;
        PPoint maximum;
        style.ButtonGetPreferredSize(*target, data, &minimum, &maximum);
        EXPECT_EQ(minimum, data.LabelSize + PPoint(30, 22));
        const Ptr<const PBrush> standard = application.GetStandardBrush(PStandardColorID::DefaultBackground);
        target->SetBackgroundBrush(skin.States[0].Background);
        target->SetBackgroundBrushOrigin(PPoint(12, 34));
        target->SetBrushOrigin(PPoint(-7, 9));
        target->SetBrush(skin.States[0].Background);
        target->SetBackgroundBrush(PStandardColorID::DefaultBackground);
        target->SetBrush(PStandardColorID::DefaultBackground);
        EXPECT_NE(target->GetBackgroundBrush(), nullptr);
        EXPECT_NE(target->GetBrush(), nullptr);
        EXPECT_EQ(target->GetBackgroundBrush()->GetDescription().Type, PBrushType::Solid);
        EXPECT_EQ(target->GetBrush()->GetDescription().Type, PBrushType::Solid);
        EXPECT_EQ(target->GetBackgroundBrush(), standard);
        EXPECT_EQ(target->GetBrush(), standard);
        EXPECT_EQ(target->GetBackgroundBrushOrigin(), PPoint(12, 34));
        EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(-7, 9));
        target->SetBackgroundBrush(nullptr);
        target->SetBrush(nullptr);
        EXPECT_EQ(target->GetBackgroundBrush(), nullptr);
        EXPECT_EQ(target->GetBrush(), nullptr);
        EXPECT_FALSE(target->GetFlags().Has(PViewFlag::Transparent));
    });
}

TEST(Brush, StandardAndNamedColorsShareReadOnlyApplicationResources)
{
    static_assert(std::is_same_v<decltype(std::declval<PApplication&>().GetStandardBrush(PNamedColors::white)), Ptr<const PBrush>>);
    static_assert(std::is_same_v<decltype(std::declval<PApplication&>().GetStandardBrush(PStandardColorID::DefaultBackground)), Ptr<const PBrush>>);
    RunGUITest([](PApplication& application)
    {
        const Ptr<const PBrush> standard = application.GetStandardBrush(PStandardColorID::DefaultBackground);
        const Ptr<const PBrush> named = application.GetStandardBrush(PNamedColors::white);
        EXPECT_EQ(standard, application.GetStandardBrush(PStandardColorID::DefaultBackground));
        EXPECT_EQ(named, application.GetStandardBrush(PNamedColors::white));
        EXPECT_EQ(named->GetDescription().Color, PColor(PNamedColors::white));
        Ptr<PView> first = ptr_new<PView>("first_brush_view", PViewFlag::WillDraw);
        Ptr<PView> second = ptr_new<PView>("second_brush_view", PViewFlag::WillDraw);
        EXPECT_EQ(first->GetBackgroundBrush(), standard);
        EXPECT_EQ(second->GetBackgroundBrush(), standard);
        EXPECT_EQ(first->GetBrush(), standard);
        first->SetBrush(PNamedColors::white);
        second->SetBackgroundBrush(PNamedColors::white);
        EXPECT_EQ(first->GetBrush(), named);
        EXPECT_EQ(second->GetBackgroundBrush(), named);
        first->SetBrush(PStandardColorID::DefaultBackground);
        second->SetBackgroundBrush(PStandardColorID::DefaultBackground);
        EXPECT_EQ(first->GetBrush(), standard);
        EXPECT_EQ(second->GetBackgroundBrush(), standard);
    });
}

TEST(Brush, CustomBrushesKeepHandlesAcrossColorChangesAndRebinding)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("custom_brush_view", PViewFlag::WillDraw);
        Ptr<PBrush> drawing = ptr_new<PBrush>(PColor(21, 43, 65), &application);
        Ptr<PBrush> background = ptr_new<PBrush>(PColor(65, 43, 21), &application);
        view->SetBrush(drawing);
        view->SetBackgroundBrush(background);
        const handle_id drawingHandle = drawing->GetHandle();
        const handle_id backgroundHandle = background->GetHandle();
        for (uint8_t value = 0; value < 16; ++value)
        {
            drawing->SetColor(PColor(value, 43, 65));
            background->SetColor(PColor(65, 43, value));
            EXPECT_EQ(view->GetBrush(), drawing);
            EXPECT_EQ(view->GetBackgroundBrush(), background);
            EXPECT_EQ(drawing->GetHandle(), drawingHandle);
            EXPECT_EQ(background->GetHandle(), backgroundHandle);
            EXPECT_EQ(view->GetBrush()->GetDescription().Color, PColor(value, 43, 65));
            EXPECT_EQ(view->GetBackgroundBrush()->GetDescription().Color, PColor(65, 43, value));
        }
        view->SetBrush(nullptr);
        view->SetBrush(drawing);
        EXPECT_EQ(view->GetBrush(), drawing);
        view->SetBackgroundBrush(PNamedColors::white);
        EXPECT_EQ(view->GetBackgroundBrush(), application.GetStandardBrush(PNamedColors::white));
        view->SetBackgroundBrush(background);
        EXPECT_EQ(view->GetBackgroundBrush(), background);
        EXPECT_EQ(application.GetStandardBrush(PNamedColors::white)->GetDescription().Color, PColor(PNamedColors::white));
        application.Sync();
    });
}

TEST(Brush, PropertyChangesPreserveHandlesAndValidateBeforePublishing)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(4, 4, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster().Get()), 16, PColor(20, 40, 60, 128).GetColor32());
        Ptr<PBrush> brush = ptr_new<PBrush>(PColor(10, 20, 30));
        const handle_id handle = brush->GetHandle();
        Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(1, 1, 3, 3));
        brush->SetImage(image);
        EXPECT_EQ(brush->GetHandle(), handle);
        EXPECT_EQ(brush->GetDescription().Type, PBrushType::Image);
        EXPECT_FALSE(brush->GetDescription().IsOpaque());
        EXPECT_EQ(brush->GetDescription().Image.OpaqueSlices, 0);
        const PBrushDescription previous = brush->GetDescription();
        EXPECT_THROW(brush->SetBitmap(bitmap, PIRect(0, 0, 5, 4)), std::system_error);
        EXPECT_EQ(brush->GetDescription(), previous);
        brush->SetColor(PColor(80, 100, 120));
        EXPECT_EQ(brush->GetHandle(), handle);
        EXPECT_EQ(brush->GetDescription().Type, PBrushType::Solid);
        EXPECT_TRUE(brush->GetDescription().IsOpaque());
        EXPECT_EQ(brush->GetBitmap(), nullptr);
        PBrushDescription solid;
        solid.Color = PColor(80, 100, 120);
        EXPECT_EQ(brush->GetDescription(), solid);
        brush->SetColor(PColor(80, 100, 120, 0));
        EXPECT_FALSE(brush->GetDescription().IsOpaque());
        brush->SetColor(PColor(80, 100, 120, 128));
        EXPECT_FALSE(brush->GetDescription().IsOpaque());
        brush->SetColor(solid.Color);
        EXPECT_EQ(brush->GetDescription(), solid);
        bitmap = nullptr;
        application.Sync();
    });
}

TEST(Brush, PaletteLookupUpdatesTheExistingSharedResource)
{
    RunGUITest([](PApplication& application)
    {
        constexpr PStandardColorID COLOR = PStandardColorID::ListViewHighlight;
        const PColor original = pget_standard_color(COLOR);
        PScopeExit restore([original] { pset_standard_color(COLOR, original); });
        const Ptr<const PBrush> brush = application.GetStandardBrush(COLOR);
        const handle_id handle = brush->GetHandle();
        pset_standard_color(COLOR, PColor(19, 37, 53));
        EXPECT_EQ(application.GetStandardBrush(COLOR), brush);
        EXPECT_EQ(brush->GetHandle(), handle);
        EXPECT_EQ(brush->GetDescription().Color, PColor(19, 37, 53));
    });
}

TEST(ImageBrush, LayoutAndTilingUpdatesKeepBrushHandlesAndSharedBitmap)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(6, 6, PEColorSpace::RGBA32);
        {
            const PBitmapRasterRW raster = bitmap->LockRaster();
            std::fill_n(reinterpret_cast<uint32_t*>(raster.Get()), 36, PColor(30, 60, 90).GetColor32());
            reinterpret_cast<uint32_t*>(raster.Get())[0] = PColor(255, 0, 0, 0).GetColor32();
        }
        Ptr<PImage> image = ptr_new<PImage>(bitmap);
        EXPECT_EQ(image->GetLayoutSize(), PPoint(1, 1));
        EXPECT_EQ(image->GetLayoutMode(), PImageLayoutMode::ViewRelative);
        image->SetSliceRect(PRect(1, 1, 5, 5));
        Ptr<PBrush> first = ptr_new<PBrush>(image);
        Ptr<PBrush> second = ptr_new<PBrush>(image);
        const handle_id firstHandle = first->GetHandle();
        const handle_id secondHandle = second->GetHandle();
        image->SetLayoutSize(PPoint(47, 19), PImageLayoutMode::Absolute);
        image->SetTiling(PImageTilingFlag::LeftEdge | PImageTilingFlag::CenterHorizontal);
        first->SetScale(PPoint(2, 3));
        EXPECT_EQ(first->GetHandle(), firstHandle);
        EXPECT_EQ(second->GetHandle(), secondHandle);
        EXPECT_EQ(first->GetBitmap(), bitmap);
        EXPECT_EQ(second->GetBitmap(), bitmap);
        EXPECT_EQ(first->GetDescription().Image, image->GetDescription());
        EXPECT_EQ(second->GetDescription().Image, image->GetDescription());
        EXPECT_EQ(first->GetScale(), PPoint(2, 3));
        EXPECT_EQ(second->GetScale(), PPoint(1, 1));
        EXPECT_FALSE(first->GetDescription().IsOpaque());
        EXPECT_EQ(image->GetDescription().OpaqueSlices, 0x01fe);
        EXPECT_THROW(image->SetSliceRect(PRect(-1, 1, 5, 5)), std::system_error);
        EXPECT_THROW(image->SetSliceRect(PRect(1.5f, 1, 5, 5)), std::system_error);
        EXPECT_THROW(image->SetLayoutSize(PPoint(0, 19)), std::system_error);
        EXPECT_EQ(image->GetLayoutSize(), PPoint(47, 19));
        EXPECT_EQ(image->GetLayoutMode(), PImageLayoutMode::Absolute);
        EXPECT_NO_THROW(image->SetLayoutSize(PPoint(47, 19), PImageLayoutMode(0xff)));
        EXPECT_NO_THROW(image->SetTiling(PImageTilingFlag(0x40)));
        EXPECT_THROW(first->SetScale(PPoint(-1, 1)), std::system_error);
        application.Sync();
    });
}

TEST(ImageBrush, NoneAndSolidStylesRetainTheirSourceUntilColorSelection)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(3, 2, PEColorSpace::RGB32);
        Ptr<PBrush> brush = ptr_new<PBrush>(bitmap);
        EXPECT_EQ(brush->GetDescription().Image.LayoutSize, PPoint(3, 2));
        EXPECT_EQ(brush->GetDescription().Image.LayoutMode, PImageLayoutMode::Absolute);
        const handle_id handle = brush->GetHandle();
        brush->SetStyle(PBrushStyle::None);
        EXPECT_EQ(brush->GetStyle(), PBrushStyle::None);
        EXPECT_EQ(brush->GetType(), PBrushType::Bitmap);
        EXPECT_EQ(brush->GetBitmap(), bitmap);
        EXPECT_FALSE(brush->GetDescription().IsOpaque());
        brush->SetScale(PPoint(0, 0));
        EXPECT_THROW(brush->SetStyle(PBrushStyle::TexturePattern), std::system_error);
        EXPECT_EQ(brush->GetStyle(), PBrushStyle::None);
        brush->SetScale(PPoint(1, 1));
        EXPECT_THROW(brush->SetStyle(PBrushStyle(0xff)), std::system_error);
        brush->SetStyle(PBrushStyle::SolidPattern);
        EXPECT_TRUE(brush->GetDescription().IsOpaque());
        brush->SetStyle(PBrushStyle::TexturePattern);
        EXPECT_EQ(brush->GetBitmap(), bitmap);
        brush->SetStyle(PBrushStyle::None);
        brush->SetColor(brush->GetDescription().Color);
        EXPECT_EQ(brush->GetStyle(), PBrushStyle::SolidPattern);
        EXPECT_EQ(brush->GetType(), PBrushType::Solid);
        EXPECT_EQ(brush->GetBitmap(), nullptr);
        EXPECT_EQ(brush->GetHandle(), handle);
        EXPECT_THROW(brush->SetStyle(PBrushStyle::TexturePattern), std::system_error);
        application.Sync();
    });
}

TEST(ImageBrush, ViewsKeepIndependentOriginsAndNoneSelections)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("origins", PViewFlag::WillDraw);
        Ptr<PBrush> none = ptr_new<PBrush>(PBrushStyle::None, &application);
        view->SetBackgroundBrush(none);
        view->SetBrush(none);
        view->SetBackgroundBrushOrigin(PPoint(11, 17));
        view->SetBrushOrigin(PPoint(-13, 29));
        EXPECT_EQ(view->GetBackgroundBrush(), none);
        EXPECT_EQ(view->GetBrush(), none);
        view->SetBrush(nullptr);
        EXPECT_EQ(view->GetBackgroundBrush(), none);
        EXPECT_EQ(view->GetBrush(), nullptr);
        EXPECT_EQ(view->GetBackgroundBrushOrigin(), PPoint(11, 17));
        EXPECT_EQ(view->GetDrawingBrushOrigin(), PPoint(-13, 29));
        view->ScrollBy(PPoint(0, -37));
        EXPECT_EQ(view->GetBackgroundBrushOrigin(), PPoint(11, 17));
        EXPECT_EQ(view->GetDrawingBrushOrigin(), PPoint(-13, 29));
    });
}

TEST(SkinnableGUIStyle, DifferentButtonSizesShareTheConfiguredImageAndBrush)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(6, 6, PEColorSpace::RGB32);
        Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(2, 2, 4, 4));
        const PImageDescription description = image->GetDescription();
        PButtonSkin skin;
        skin.States[0].Background = ptr_new<PBrush>(image);
        PSkinnableGUIStyle style(skin);
        Ptr<PView> first = ptr_new<PView>("first_size", PViewFlag::WillDraw);
        Ptr<PView> second = ptr_new<PView>("second_size", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(first));
        ASSERT_TRUE(application.AddHandler(second));
        PScopeExit removeViews([&application, &first, &second] { application.RemoveHandler(first); application.RemoveHandler(second); });
        first->SetFrame(PRect(0, 0, 120, 40));
        second->SetFrame(PRect(0, 0, 180, 60));
        PButtonStyleData firstData;
        PButtonStyleData secondData;
        style.ButtonPaint(*first, first->GetBounds(), true, firstData);
        style.ButtonPaint(*second, second->GetBounds(), true, secondData);
        const Ptr<const PBrush> firstBrush = first->GetBackgroundBrush();
        EXPECT_EQ(firstBrush, skin.States[0].Background);
        EXPECT_EQ(firstBrush, second->GetBackgroundBrush());
        EXPECT_EQ(firstBrush->GetBitmap(), bitmap);
        EXPECT_EQ(second->GetBackgroundBrush()->GetBitmap(), bitmap);
        EXPECT_EQ(firstBrush->GetImage(), image);
        EXPECT_EQ(second->GetBackgroundBrush()->GetImage(), image);
        EXPECT_EQ(image->GetLayoutSize(), PPoint(1, 1));
        EXPECT_EQ(image->GetLayoutMode(), PImageLayoutMode::ViewRelative);
        first->SetFrame(PRect(0, 0, 140, 50));
        style.ButtonPaint(*first, first->GetBounds(), true, firstData);
        EXPECT_EQ(first->GetBackgroundBrush(), firstBrush);
        EXPECT_EQ(second->GetBackgroundBrush(), firstBrush);
        EXPECT_EQ(image->GetDescription(), description);
        image->SetLayoutSize(PPoint(0.5f, 0.5f));
        EXPECT_EQ(image->GetLayoutMode(), PImageLayoutMode::ViewRelative);
        EXPECT_EQ(firstBrush->GetDescription().Image.LayoutSize, PPoint(0.5f, 0.5f));
        image->SetLayoutSize(PPoint(6, 6), PImageLayoutMode::Absolute);
        style.ButtonPaint(*first, first->GetBounds(), true, firstData);
        EXPECT_EQ(first->GetBackgroundBrush(), firstBrush);
        EXPECT_EQ(image->GetLayoutSize(), PPoint(6, 6));
        EXPECT_EQ(image->GetLayoutMode(), PImageLayoutMode::Absolute);
        application.Sync();
    });
}
