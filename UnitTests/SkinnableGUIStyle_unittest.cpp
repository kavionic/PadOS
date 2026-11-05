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
        EXPECT_EQ(target->GetBackgroundBrushBinding().Source, PBrushSource::Brush);
        EXPECT_EQ(target->GetBrushBinding().Source, PBrushSource::Brush);
        EXPECT_EQ(target->GetBackgroundBrushBinding().Brush, application.GetStandardBrush(PStandardColorID::ButtonBackground));
        EXPECT_EQ(target->GetBrushBinding().Brush, target->GetBackgroundBrushBinding().Brush);
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
                EXPECT_EQ(target->GetBackgroundBrushBinding().Brush, appearance.Background);
                EXPECT_EQ(target->GetBrushBinding().Source, PBrushSource::Inherit);
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
        EXPECT_EQ(target->GetBackgroundBrushBinding().Brush, skin.States[0].Background);
    });
}

TEST(SkinnableGUIStyle, SliceBordersParticipateInSizeAndStandardBrushesResetBindings)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(32, 24, PEColorSpace::RGB32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster()), 32 * 24, PColor(80, 120, 160).GetColor32());
        bitmap->UnlockRaster();
        PButtonSkin skin;
        skin.LabelPadding = PRect(1.0f);
        skin.States[0].Background = ptr_new<PBrush>(PColor(0, 0, 0));
        skin.CheckedStates[2].Background = ptr_new<PBrush>(bitmap, bitmap->GetBounds(), PBrushType::NineSlice, PBrushBorders{15, 11, 15, 11});
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
        target->SetBackgroundBrush(skin.States[0].Background, PRect(0, 0, 200, 900));
        target->SetBrush(skin.States[0].Background);
        target->SetBackgroundBrush(PStandardColorID::DefaultBackground);
        target->SetBrush(PStandardColorID::DefaultBackground);
        EXPECT_EQ(target->GetBackgroundBrushBinding().Source, PBrushSource::Brush);
        EXPECT_EQ(target->GetBrushBinding().Source, PBrushSource::Brush);
        EXPECT_EQ(target->GetBackgroundBrushBinding().Brush->GetDescription().Type, PBrushType::Solid);
        EXPECT_EQ(target->GetBrushBinding().Brush->GetDescription().Type, PBrushType::Solid);
        EXPECT_EQ(target->GetBackgroundBrushBinding().Brush, standard);
        EXPECT_EQ(target->GetBrushBinding().Brush, standard);
        EXPECT_TRUE(target->GetBackgroundBrushBinding().AutomaticBounds);
        EXPECT_TRUE(target->GetBrushBinding().AutomaticBounds);
        target->SetBackgroundBrush(nullptr);
        target->SetBrush(nullptr);
        EXPECT_EQ(target->GetBackgroundBrushBinding().Source, PBrushSource::Inherit);
        EXPECT_EQ(target->GetBrushBinding().Source, PBrushSource::Inherit);
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
        EXPECT_EQ(first->GetBackgroundBrushBinding().Brush, standard);
        EXPECT_EQ(second->GetBackgroundBrushBinding().Brush, standard);
        EXPECT_EQ(first->GetBrushBinding().Brush, standard);
        first->SetBrush(PNamedColors::white);
        second->SetBackgroundBrush(PNamedColors::white);
        EXPECT_EQ(first->GetBrushBinding().Brush, named);
        EXPECT_EQ(second->GetBackgroundBrushBinding().Brush, named);
        first->SetBrush(PStandardColorID::DefaultBackground);
        second->SetBackgroundBrush(PStandardColorID::DefaultBackground);
        EXPECT_EQ(first->GetBrushBinding().Brush, standard);
        EXPECT_EQ(second->GetBackgroundBrushBinding().Brush, standard);
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
            EXPECT_EQ(view->GetBrushBinding().Brush, drawing);
            EXPECT_EQ(view->GetBackgroundBrushBinding().Brush, background);
            EXPECT_EQ(drawing->GetHandle(), drawingHandle);
            EXPECT_EQ(background->GetHandle(), backgroundHandle);
            EXPECT_EQ(view->GetBrushBinding().Brush->GetDescription().Color, PColor(value, 43, 65));
            EXPECT_EQ(view->GetBackgroundBrushBinding().Brush->GetDescription().Color, PColor(65, 43, value));
        }
        view->SetBrush(nullptr);
        view->SetBrush(drawing);
        EXPECT_EQ(view->GetBrushBinding().Brush, drawing);
        view->SetBackgroundBrush(PNamedColors::white);
        EXPECT_EQ(view->GetBackgroundBrushBinding().Brush, application.GetStandardBrush(PNamedColors::white));
        view->SetBackgroundBrush(background);
        EXPECT_EQ(view->GetBackgroundBrushBinding().Brush, background);
        EXPECT_EQ(application.GetStandardBrush(PNamedColors::white)->GetDescription().Color, PColor(PNamedColors::white));
        application.Sync();
    });
}

TEST(Brush, PropertyChangesPreserveHandlesAndValidateBeforePublishing)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(4, 4, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster()), 16, PColor(20, 40, 60, 128).GetColor32());
        bitmap->UnlockRaster();
        Ptr<PBrush> brush = ptr_new<PBrush>(PColor(10, 20, 30));
        const handle_id handle = brush->GetHandle();
        brush->SetBitmap(bitmap, bitmap->GetBounds(), PBrushType::NineSlice, PBrushBorders{1, 1, 1, 1});
        EXPECT_EQ(brush->GetHandle(), handle);
        EXPECT_EQ(brush->GetDescription().Type, PBrushType::NineSlice);
        EXPECT_FALSE(brush->GetDescription().IsOpaque);
        EXPECT_EQ(brush->GetDescription().OpaqueSlices, 0);
        const PBrushDescription previous = brush->GetDescription();
        EXPECT_THROW(brush->SetBitmap(bitmap, PIRect(0, 0, 5, 4)), std::system_error);
        EXPECT_EQ(brush->GetDescription(), previous);
        brush->SetColor(PColor(80, 100, 120));
        EXPECT_EQ(brush->GetHandle(), handle);
        EXPECT_EQ(brush->GetDescription().Type, PBrushType::Solid);
        EXPECT_TRUE(brush->GetDescription().IsOpaque);
        EXPECT_EQ(brush->GetBitmap(), nullptr);
        PBrushDescription solid;
        solid.Color = PColor(80, 100, 120);
        EXPECT_EQ(brush->GetDescription(), solid);
        brush->SetColor(PColor(80, 100, 120, 0));
        EXPECT_FALSE(brush->GetDescription().IsOpaque);
        brush->SetColor(PColor(80, 100, 120, 128));
        EXPECT_FALSE(brush->GetDescription().IsOpaque);
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
