// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>
#include <string_view>
#include <GUI/BitmapLoader.h>
#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Style/SliderStyleData.h>
#include <GUI/Style/SliderStyleLayout.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/View.h>
#include <System/ExceptionHandling.h>
#include <Storage/File.h>
#include <Storage/StandardPaths.h>
#include <Utils/XMLObjectParser.h>

#include "GUITest.h"

class PButtonSkinTestStream : public PStreamableIO
{
public:
    explicit PButtonSkinTestStream(std::string_view data) : m_Data(data) {}

    ssize_t Read(void* buffer, ssize_t size) override
    {
        constexpr size_t MAX_CHUNK_SIZE = 7;
        const size_t count = std::min({size_t(size), MAX_CHUNK_SIZE, m_Data.size() - m_Offset});
        memcpy(buffer, m_Data.data() + m_Offset, count);
        m_Offset += count;
        return count;
    }

    ssize_t Write(const void*, ssize_t) override
    {
        set_last_error(EBADF);
        return -1;
    }

private:
    std::string_view m_Data;
    size_t m_Offset = 0;
};

TEST(SkinnableGUIStyle, XMLThemesLoadAllStatesWithOpaqueNineSliceCenters)
{
    RunGUITest([](PApplication& application)
    {
        constexpr std::array<const char*, 2> THEMES = {"Normal", "Dark"};
        constexpr std::array<std::array<uint32_t, 8>, 2> LABEL_COLORS =
        {{
            {0x25354a, 0x203b59, 0x25354a, 0x8a96a6, 0x173f63, 0x103b60, 0x153c5c, 0x8297a9},
            {0xeef4fc, 0xffffff, 0xe8f1fc, 0x78879c, 0xf1f8ff, 0xffffff, 0xeaf5ff, 0x8095a9}
        }};
        Ptr<PView> target = ptr_new<PView>("loaded_skin", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 160, 48));
        for (size_t themeIndex = 0; themeIndex < THEMES.size(); ++themeIndex)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", THEMES[themeIndex])));
            const PSkinnableGUIStyle style(directory, &application);
            const Ptr<const PBrush> background = style.GetBackgroundBrush();
            ASSERT_NE(background, nullptr);
            EXPECT_EQ(background->GetApplication(), &application);
            EXPECT_EQ(background->GetType(), PBrushType::Bitmap);
            EXPECT_EQ(background->GetStyle(), PBrushStyle::TexturePattern);
            ASSERT_NE(background->GetBitmap(), nullptr);
            EXPECT_EQ(background->GetBitmap()->GetBounds(), PIRect(0, 0, 64, 64));
            EXPECT_TRUE(background->GetDescription().IsOpaque());
            EXPECT_EQ(background->GetDescription().Image.LayoutMode, PImageLayoutMode::Absolute);
            EXPECT_EQ(background->GetDescription().Image.LayoutSize, PPoint(64, 64));
            EXPECT_EQ(background->GetScale(), PPoint(1.0f));
            EXPECT_EQ(style.GetBackgroundBrush(), background);
            target->SetBackgroundBrush(background);
            EXPECT_EQ(target->GetBackgroundBrush(), background);
            PButtonStyleData data;
            data.Label = "Loaded button";
            for (size_t index = 0; index < 8; ++index)
            {
                const size_t state = index % 4;
                data.IsChecked = index >= 4;
                data.IsPointerOver = state >= 1;
                data.IsPressed = state >= 2;
                style.ButtonPaint(*target, target->GetBounds(), state != 3, data);
                const Ptr<const PBrush> brush = target->GetBackgroundBrush();
                ASSERT_NE(brush, nullptr);
                ASSERT_NE(brush->GetImage(), nullptr);
                EXPECT_EQ(brush->GetApplication(), &application);
                EXPECT_EQ(brush->GetBitmap()->GetBounds(), PIRect(0, 0, 48, 48));
                EXPECT_EQ(brush->GetBitmap()->GetColorSpace(), PEColorSpace::RGBA32);
                EXPECT_EQ(brush->GetImage()->GetSliceRect(), PRect(12, 12, 36, 36));
                EXPECT_EQ(brush->GetImage()->GetLayoutMode(), PImageLayoutMode::ViewRelative);
                EXPECT_EQ(brush->GetImage()->GetLayoutSize(), PPoint(1, 1));
                EXPECT_EQ(brush->GetImage()->GetDescription().OpaqueSlices, 0x00ba);
                EXPECT_EQ(target->GetFgColor(), PColor::FromRGB32(LABEL_COLORS[themeIndex][index]));
                PPoint minimum;
                PPoint maximum;
                style.ButtonGetPreferredSize(*target, data, &minimum, &maximum);
                EXPECT_EQ(minimum, data.LabelSize + PPoint(24, 8));
                style.ButtonPaint(*target, target->GetBounds(), state != 3, data);
                EXPECT_EQ(target->GetBackgroundBrush(), brush);
            }
        }
    });
}

TEST(SkinnableGUIStyle, XMLCheckBoxesKeepIndicatorSizeAndLabelPositionAcrossAllStates)
{
    RunGUITest([](PApplication& application)
    {
        constexpr std::array<const char*, 2> THEMES = {"Normal", "Dark"};
        Ptr<PView> target = ptr_new<PView>("checkbox_skin", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 240, 96));
        const PRect frame(7, 11, 227, 75);
        const PFontHeight fontHeight = target->GetFontHeight();
        const float baseline = std::round(11.0f + (64.0f - fontHeight.ascender - fontHeight.descender) * 0.5f) + fontHeight.ascender - 1.0f;
        for (size_t themeIndex = 0; themeIndex < THEMES.size(); ++themeIndex)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", THEMES[themeIndex])));
            PSkinnableGUIStyle style(directory, &application);
            PButtonStyleData data;
            data.Label = "Option";
            for (size_t index = 0; index < 8; ++index)
            {
                const size_t state = index % 4;
                data.IsChecked = index >= 4;
                data.IsPointerOver = state >= 1;
                data.IsPressed = state >= 2;
                style.ButtonPaint(*target, frame, state != 3, data);
                const PColor expectedLabelColor = target->GetFgColor();
                target->SetDrawingMode(PDrawingMode::Blend);
                style.CheckBoxPaint(*target, frame, state != 3, data);
                EXPECT_EQ(target->GetFgColor(), expectedLabelColor);
                EXPECT_EQ(target->GetPenPosition(), PPoint(47, baseline));
                EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(7, 27));
                EXPECT_EQ(target->GetBackgroundBrush(), nullptr);
                EXPECT_EQ(target->GetBrush(), nullptr);
                EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                PPoint minimum;
                PPoint maximum;
                style.CheckBoxGetPreferredSize(*target, data, &minimum, &maximum);
                EXPECT_EQ(minimum, PPoint(data.LabelSize.x + 40, 32));
                EXPECT_EQ(maximum, minimum);
            }
            data.Label.clear();
            data.IsLabelSizeValid = false;
            PPoint minimum;
            PPoint maximum;
            style.CheckBoxGetPreferredSize(*target, data, &minimum, &maximum);
            EXPECT_EQ(minimum, PPoint(32, 32));
            style.CheckBoxPaint(*target, frame, true, data);
            EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(7, 27));
        }
    });
}

TEST(SkinnableGUIStyle, UnconfiguredCheckBoxesDelegateToDefaultStyle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("default_checkbox", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 180, 40));
        PButtonStyleData data;
        data.Label = "Default checkbox";
        data.IsChecked = true;
        PDefaultGUIStyle defaultStyle;
        PPoint expectedMinimum;
        PPoint expectedMaximum;
        defaultStyle.CheckBoxGetPreferredSize(*target, data, &expectedMinimum, &expectedMaximum);
        defaultStyle.CheckBoxPaint(*target, target->GetBounds(), true, data);
        const PPoint expectedLabelPosition = target->GetPenPosition();
        const Ptr<const PBrush> expectedBackground = target->GetBackgroundBrush();

        PSkinnableGUIStyle style;
        EXPECT_EQ(style.GetBackgroundBrush(), application.GetStandardBrush(PStandardColorID::DefaultBackground));
        PPoint minimum;
        PPoint maximum;
        style.CheckBoxGetPreferredSize(*target, data, &minimum, &maximum);
        EXPECT_EQ(minimum, expectedMinimum);
        EXPECT_EQ(maximum, expectedMaximum);
        style.CheckBoxPaint(*target, target->GetBounds(), true, data);
        EXPECT_EQ(target->GetPenPosition(), expectedLabelPosition);
        EXPECT_EQ(target->GetBackgroundBrush(), expectedBackground);

        PCheckBoxSkin skin;
        skin.Size = 0.0f;
        EXPECT_THROW((PSkinnableGUIStyle(PButtonSkin(), skin)), std::system_error);
        skin.Size = 32.0f;
        skin.LabelSpacing = -1.0f;
        EXPECT_NO_THROW((PSkinnableGUIStyle(PButtonSkin(), skin)));
    });
}

TEST(SkinnableGUIStyle, XMLRadioButtonsKeepLayoutAcrossStatesAndLabelAlignments)
{
    RunGUITest([](PApplication& application)
    {
        constexpr std::array<const char*, 2> THEMES = {"Normal", "Dark"};
        constexpr std::array<PAlignment, 5> ALIGNMENTS = {PAlignment::Right, PAlignment::Left, PAlignment::Top, PAlignment::Bottom, PAlignment::Center};
        Ptr<PView> target = ptr_new<PView>("radio_skin", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        const PPoint origin(7, 11);
        const PFontHeight fontHeight = target->GetFontHeight();
        const float labelHeight = fontHeight.ascender + fontHeight.descender;
        for (const char* theme : THEMES)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            PSkinnableGUIStyle style(directory, &application);
            PButtonStyleData data;
            data.Label = "Option";
            const float labelWidth = target->GetStringWidth(data.Label);
            for (PAlignment alignment : ALIGNMENTS)
            {
                data.LabelAlignment = alignment;
                const bool vertical = alignment == PAlignment::Top || alignment == PAlignment::Bottom;
                const PPoint expectedSize = vertical
                    ? PPoint(std::max(32.0f, labelWidth), 38.0f + labelHeight) : PPoint(labelWidth + 40.0f, std::max(32.0f, labelHeight + 2.0f));
                PPoint minimum;
                PPoint maximum;
                style.RadioButtonGetPreferredSize(*target, data, &minimum, &maximum);
                EXPECT_EQ(minimum, expectedSize);
                EXPECT_EQ(maximum, minimum);
                target->SetFrame(PRect(PPoint(), expectedSize + PPoint(14, 22)));
                const PRect frame(origin, origin + expectedSize);
                PPoint expectedIndicator(origin.x, std::round(origin.y + (expectedSize.y - 32.0f) * 0.5f));
                PPoint expectedLabel(origin.x + 40.0f, std::round(origin.y + (expectedSize.y - labelHeight) * 0.5f) + fontHeight.ascender - 1.0f);
                if (vertical)
                {
                    expectedIndicator.x = std::round(origin.x + (expectedSize.x - 32.0f) * 0.5f);
                    expectedLabel.x = std::round(origin.x + (expectedSize.x - labelWidth) * 0.5f);
                    if (alignment == PAlignment::Top)
                    {
                        expectedIndicator.y = origin.y + labelHeight + 6.0f;
                        expectedLabel.y = origin.y + fontHeight.ascender;
                    }
                    else
                    {
                        expectedIndicator.y = origin.y;
                        expectedLabel.y = origin.y + 36.0f + fontHeight.ascender;
                    }
                }
                else if (alignment == PAlignment::Left)
                {
                    expectedIndicator.x = origin.x + labelWidth + 8.0f;
                    expectedLabel.x = origin.x;
                }
                for (size_t index = 0; index < 8; ++index)
                {
                    const size_t state = index % 4;
                    data.IsChecked = index >= 4;
                    data.IsPointerOver = state >= 1;
                    data.IsPressed = state >= 2;
                    style.ButtonPaint(*target, frame, state != 3, data);
                    const PColor expectedLabelColor = target->GetFgColor();
                    target->SetDrawingMode(PDrawingMode::Blend);
                    style.RadioButtonPaint(*target, frame, state != 3, data);
                    EXPECT_EQ(target->GetFgColor(), expectedLabelColor);
                    EXPECT_EQ(target->GetPenPosition(), expectedLabel);
                    EXPECT_EQ(target->GetDrawingBrushOrigin(), expectedIndicator);
                    EXPECT_EQ(target->GetBackgroundBrush(), nullptr);
                    EXPECT_EQ(target->GetBrush(), nullptr);
                    EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                }
            }
            data.Label.clear();
            data.IsLabelSizeValid = false;
            PPoint minimum;
            PPoint maximum;
            style.RadioButtonGetPreferredSize(*target, data, &minimum, &maximum);
            EXPECT_EQ(minimum, PPoint(32, 32));
            EXPECT_EQ(maximum, minimum);
            target->SetFrame(PRect(0, 0, 120, 90));
            style.RadioButtonPaint(*target, PRect(7, 11, 107, 75), true, data);
            EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(41, 27));
        }
    });
}

TEST(SkinnableGUIStyle, UnconfiguredRadioButtonsDelegateToDefaultStyle)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("default_radio", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 180, 40));
        PButtonStyleData data;
        data.Label = "Default radio";
        data.IsChecked = true;
        PDefaultGUIStyle defaultStyle;
        PPoint expectedMinimum;
        PPoint expectedMaximum;
        defaultStyle.RadioButtonGetPreferredSize(*target, data, &expectedMinimum, &expectedMaximum);
        defaultStyle.RadioButtonPaint(*target, target->GetBounds(), true, data);
        const PPoint expectedLabelPosition = target->GetPenPosition();
        const Ptr<const PBrush> expectedBackground = target->GetBackgroundBrush();

        PSkinnableGUIStyle style;
        PPoint minimum;
        PPoint maximum;
        style.RadioButtonGetPreferredSize(*target, data, &minimum, &maximum);
        EXPECT_EQ(minimum, expectedMinimum);
        EXPECT_EQ(maximum, expectedMaximum);
        style.RadioButtonPaint(*target, target->GetBounds(), true, data);
        EXPECT_EQ(target->GetPenPosition(), expectedLabelPosition);
        EXPECT_EQ(target->GetBackgroundBrush(), expectedBackground);

        PRadioButtonSkin skin;
        skin.Size = 0.0f;
        EXPECT_THROW((PSkinnableGUIStyle(PButtonSkin(), PCheckBoxSkin(), skin)), std::system_error);
        skin.Size = 32.0f;
        skin.LabelSpacing = -1.0f;
        skin.LabelVerticalSpacing = -1.0f;
        EXPECT_NO_THROW((PSkinnableGUIStyle(PButtonSkin(), PCheckBoxSkin(), skin)));
    });
}

TEST(SkinnableGUIStyle, NegativeSpacingKeepsBothElementsWithinPreferredBoundsAndRestoresClips)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("overlapping_indicators", PViewFlag::WillDraw);
        target->SetFrame(PRect::FromSize(320.0f, 256.0f));
        target->Hide();
        ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
        PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
        target->ScrollBy(PPoint(-3, -5));
        const Ptr<const PRegion> originalClip = ptr_new<PRegion>(PIRect(4, 6, 300, 240));
        PButtonStyleData data;
        data.Label = "Option";
        const float labelWidth = target->GetStringWidth(data.Label);
        const PFontHeight fontHeight = target->GetFontHeight();
        constexpr std::array<PAlignment, 4> ALIGNMENTS = {PAlignment::Right, PAlignment::Left, PAlignment::Top, PAlignment::Bottom};
        for (float spacing : {-8.0f, -128.0f})
        {
            PCheckBoxSkin checkBoxSkin;
            checkBoxSkin.LabelSpacing = spacing;
            checkBoxSkin.States[0].Background = ptr_new<PBrush>(PColor(70, 110, 160, 128));
            PRadioButtonSkin radioSkin;
            radioSkin.LabelSpacing = spacing;
            radioSkin.LabelVerticalSpacing = spacing;
            radioSkin.States[0].Background = checkBoxSkin.States[0].Background;
            PSkinnableGUIStyle style(PButtonSkin(), checkBoxSkin, radioSkin);
            for (bool checkBox : {false, true})
            {
                for (PAlignment alignment : ALIGNMENTS)
                {
                    if (checkBox && alignment != PAlignment::Right) {
                        continue;
                    }
                    data.LabelAlignment = alignment;
                    PPoint minimum;
                    PPoint maximum;
                    if (checkBox) {
                        style.CheckBoxGetPreferredSize(*target, data, &minimum, &maximum);
                    } else {
                        style.RadioButtonGetPreferredSize(*target, data, &minimum, &maximum);
                    }
                    EXPECT_EQ(maximum, minimum);
                    const PRect frame(PPoint(11, 13), PPoint(11, 13) + minimum);
                    target->SetDrawingRegion(originalClip);
                    target->SetDrawingMode(PDrawingMode::Blend);
                    if (checkBox) {
                        style.CheckBoxPaint(*target, frame, true, data);
                    } else {
                        style.RadioButtonPaint(*target, frame, true, data);
                    }
                    EXPECT_EQ(target->GetDrawingRegion(), originalClip);
                    EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                    EXPECT_EQ(target->GetBrush(), nullptr);
                    const PPoint indicator = target->GetDrawingBrushOrigin();
                    const PPoint label = target->GetPenPosition();
                    EXPECT_GE(indicator.x, frame.left);
                    EXPECT_GE(indicator.y, frame.top);
                    EXPECT_LE(indicator.x + 32.0f, frame.right);
                    EXPECT_LE(indicator.y + 32.0f, frame.bottom);
                    EXPECT_GE(label.x, frame.left);
                    EXPECT_GE(label.y - fontHeight.ascender, frame.top);
                    EXPECT_LE(label.x + labelWidth, frame.right);
                    EXPECT_LE(label.y + fontHeight.descender, frame.bottom);
                    if (alignment == PAlignment::Top) {
                        EXPECT_FLOAT_EQ(indicator.y - (label.y + fontHeight.descender + 2.0f), spacing);
                    } else if (alignment == PAlignment::Bottom) {
                        EXPECT_FLOAT_EQ(label.y - fontHeight.ascender - (indicator.y + 32.0f), spacing);
                    } else if (alignment == PAlignment::Left) {
                        EXPECT_FLOAT_EQ(indicator.x - (label.x + labelWidth), spacing);
                    } else {
                        EXPECT_FLOAT_EQ(label.x - (indicator.x + 32.0f), spacing);
                    }
                }
            }
            target->ClearDrawingRegion();
            style.RadioButtonPaint(*target, target->GetBounds(), true, data);
            EXPECT_EQ(target->GetDrawingRegion(), nullptr);
            target->SetDrawingRegion(PRegion());
            const Ptr<const PRegion> emptyClip = target->GetDrawingRegion();
            style.CheckBoxPaint(*target, target->GetBounds(), true, data);
            EXPECT_EQ(target->GetDrawingRegion(), emptyClip);
            EXPECT_TRUE(target->GetDrawingRegion()->IsEmpty());
        }
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, IndicatorSkinsRejectNonFiniteMetrics)
{
    RunGUITest([](PApplication&)
    {
        constexpr std::array<float, 3> NON_FINITE_VALUES =
        {
            std::numeric_limits<float>::infinity(),
            -std::numeric_limits<float>::infinity(),
            std::numeric_limits<float>::quiet_NaN()
        };
        constexpr std::array<float PButtonIndicatorSkin::*, 3> METRICS =
        {
            &PButtonIndicatorSkin::Size,
            &PButtonIndicatorSkin::LabelSpacing,
            &PButtonIndicatorSkin::LabelVerticalOffset
        };
        for (float value : NON_FINITE_VALUES)
        {
            for (float PButtonIndicatorSkin::* metric : METRICS)
            {
                PCheckBoxSkin checkBoxSkin;
                checkBoxSkin.*metric = value;
                EXPECT_THROW((PSkinnableGUIStyle(PButtonSkin(), checkBoxSkin)), std::system_error);
                PRadioButtonSkin radioButtonSkin;
                radioButtonSkin.*metric = value;
                EXPECT_THROW((PSkinnableGUIStyle(PButtonSkin(), PCheckBoxSkin(), radioButtonSkin)), std::system_error);
            }
            PRadioButtonSkin radioButtonSkin;
            radioButtonSkin.LabelVerticalSpacing = value;
            EXPECT_THROW((PSkinnableGUIStyle(PButtonSkin(), PCheckBoxSkin(), radioButtonSkin)), std::system_error);
        }
    });
}

TEST(SliderStyleLayout, MeasuresAndTracksDamageWithoutUsingTheStylePayload)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("independent_slider_layout");
        target->SetFrame(PRect::FromSize(240.0f, 200.0f));
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            PSliderStyleLayout layout;
            layout.SetKnobBounds(PRect(-5.0f, -10.0f, 12.0f, 20.0f));
            PSliderStyleData data;
            data.Orientation = orientation;
            data.HasValueLabel = true;
            data.ValueLabel = "50";
            data.MinLabel = "Low";
            data.MaxLabel = "High";
            data.ShadowValues = {0.25f, 0.75f};
            PPoint minimum;
            PPoint maximum;
            layout.GetPreferredSize(*target, data, &minimum, &maximum);
            EXPECT_GT(minimum.x, 0.0f);
            EXPECT_GT(minimum.y, 0.0f);
            const PPoint position = layout.GetValuePosition(*target, data, 0.5f);
            EXPECT_NEAR(layout.GetValueFromPosition(*target, data, position), 0.5f, 0.01f);
            EXPECT_FALSE(layout.HasVisualState());
            EXPECT_EQ(data.StyleData, nullptr);
            layout.PropertyChanged(*target, data, PSliderProperty::Appearance);
            EXPECT_TRUE(layout.HasVisualState());
            data.Value = 0.75f;
            layout.PropertyChanged(*target, data, PSliderProperty::Value);
            EXPECT_EQ(data.StyleData, nullptr);
        }
    });
}

TEST(SkinnableGUIStyle, XMLSlidersKeepGeometryAndDrawingStateStableAcrossStatesForBothThemes)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("slider_skins", PViewFlag::WillDraw);
        target->SetFrame(PRect::FromSize(240.0f, 200.0f));
        target->Hide();
        ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
        PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
        const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(2, 3, 238, 198));
        constexpr std::array<PPoint, 4> ANCHORS = {PPoint(7, 10), PPoint(7, 15), PPoint(7, 11), PPoint(7, 16)};
        constexpr std::array<PPoint, 4> BITMAP_SIZES = {PPoint(15, 21), PPoint(15, 23), PPoint(15, 23), PPoint(15, 33)};
        for (const char* theme : {"Normal", "Dark"})
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            PSkinnableGUIStyle style(directory, &application);
            for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
            {
                for (size_t shape = 0; shape < 4; ++shape)
                {
                    const bool isHorizontal = orientation == POrientation::Horizontal;
                    PSliderStyleData geometryData;
                    geometryData.Orientation = orientation;
                    geometryData.KnobPointUp = shape % 2 != 0;
                    geometryData.KnobPointDown = shape >= 2;
                    PPoint nativeMinimum;
                    PPoint nativeMaximum;
                    style.SliderGetPreferredSize(*target, geometryData, &nativeMinimum, &nativeMaximum);
                    const PPoint expectedSize = isHorizontal ? BITMAP_SIZES[shape] : PPoint(BITMAP_SIZES[shape].y, BITMAP_SIZES[shape].x);
                    const PPoint expectedAnchor = isHorizontal ? ANCHORS[shape] : PPoint(ANCHORS[shape].y, ANCHORS[shape].x);
                    EXPECT_EQ(nativeMinimum, expectedSize);
                    target->SetFrame(PRect::FromSize(isHorizontal ? PPoint(240.0f, nativeMinimum.y) : PPoint(nativeMinimum.x, 200.0f)));
                    EXPECT_EQ(style.SliderGetValuePosition(*target, geometryData, isHorizontal ? 0.0f : 1.0f), expectedAnchor);
                    target->SetFrame(PRect::FromSize(240.0f, 200.0f));
                    PSliderStyleData data;
                    data.Orientation = orientation;
                    data.KnobPointUp = shape % 2 != 0;
                    data.KnobPointDown = shape >= 2;
                    data.TicksAbove = true;
                    data.TicksBelow = true;
                    data.HasValueLabel = true;
                    data.ValueLabel = "Value: 50";
                    data.MinLabel = "Low";
                    data.MaxLabel = "High";
                    data.ShadowValues = {0.2f, 0.8f};
                    PPoint minimum;
                    PPoint maximum;
                    style.SliderGetPreferredSize(*target, data, &minimum, &maximum);
                    EXPECT_GT(minimum.x, 0.0f);
                    EXPECT_GT(minimum.y, 0.0f);
                    EXPECT_GE(maximum.x, minimum.x);
                    EXPECT_GE(maximum.y, minimum.y);
                    const Ptr<PtrTarget> privateState = data.StyleData;
                    for (float value : {0.0f, 0.5f, 1.0f})
                    {
                        data.Value = value;
                        const PPoint position = style.SliderGetValuePosition(*target, data, value);
                        EXPECT_NEAR(style.SliderGetValueFromPosition(*target, data, position), value, 0.01f);
                        style.SliderPropertyChanged(*target, data, PSliderProperty::Value);
                        for (size_t state = 0; state < 4; ++state)
                        {
                            data.IsPointerOver = state >= 1;
                            data.IsPressed = state >= 2;
                            data.IsEnabled = state != 3;
                            style.SliderPropertyChanged(
                                *target,
                                data,
                                PSliderProperties(PSliderProperty::Pressed, PSliderProperty::Hovered, PSliderProperty::Enabled));
                            target->SetDrawingRegion(clip);
                            target->SetDrawingMode(PDrawingMode::Blend);
                            style.SliderPaint(*target, target->GetNormalizedBounds(), data);
                            EXPECT_EQ(target->GetDrawingRegion(), clip);
                            EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                            EXPECT_EQ(target->GetBrush(), nullptr);
                            EXPECT_EQ(target->GetBackgroundBrush(), nullptr);
                            EXPECT_EQ(data.StyleData, privateState);
                            EXPECT_EQ(style.SliderGetValuePosition(*target, data, value), position);
                        }
                    }
                }
            }
        }
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, SliderLayoutUsesBitmapSizesAndAsymmetricAnchorsAcrossStates)
{
    RunGUITest([](PApplication&)
    {
        Ptr<PBitmap> trackBitmap = ptr_new<PBitmap>(16, 5, PEColorSpace::RGBA32);
        Ptr<PImage> track = ptr_new<PImage>(trackBitmap);
        Ptr<PView> target = ptr_new<PView>("slider_anchor_layout");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool isHorizontal = orientation == POrientation::Horizontal;
            PSliderKnobSkin knob;
            PSliderKnobAppearance& normal = knob.States[std::to_underlying(PButtonSkinState::Normal)];
            normal.Bitmap = ptr_new<PBitmap>(isHorizontal ? 31 : 9, isHorizontal ? 9 : 31, PEColorSpace::RGBA32);
            normal.Anchor = isHorizontal ? PPoint(5.0f, 3.0f) : PPoint(3.0f, 5.0f);
            PSliderKnobAppearance& hovered = knob.States[std::to_underlying(PButtonSkinState::Hovered)];
            hovered.Bitmap = ptr_new<PBitmap>(isHorizontal ? 41 : 13, isHorizontal ? 13 : 41, PEColorSpace::RGBA32);
            hovered.Anchor = isHorizontal ? PPoint(9.0f, 5.0f) : PPoint(5.0f, 9.0f);
            knob.Shadow.Bitmap = ptr_new<PBitmap>(isHorizontal ? 37 : 11, isHorizontal ? 11 : 37, PEColorSpace::RGBA32);
            knob.Shadow.Anchor = isHorizontal ? PPoint(7.0f, 4.0f) : PPoint(4.0f, 7.0f);
            PSliderSkin skin;
            skin.Horizontal.Track = track;
            skin.Vertical.Track = track;
            skin.Horizontal.Knobs.fill(knob);
            skin.Vertical.Knobs.fill(knob);
            PSkinnableGUIStyle style(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), skin);
            for (size_t appearance = 0; appearance < 16; ++appearance)
            {
                const size_t knobShape = appearance % 4;
                const size_t tickSides = appearance / 4;
                PSliderStyleData data;
                data.Orientation = orientation;
                data.KnobPointUp = knobShape % 2 != 0;
                data.KnobPointDown = knobShape >= 2;
                data.TicksAbove = tickSides % 2 != 0;
                data.TicksBelow = tickSides >= 2;
                const float leadingSpace = data.TicksAbove ? 11.0f : 5.0f;
                const float trailingSpace = data.TicksBelow ? 12.0f : 8.0f;
                const float thickness = leadingSpace + trailingSpace;
                target->SetFrame(PRect::FromSize(isHorizontal ? PPoint(200.0f, thickness) : PPoint(thickness, 200.0f)));
                PPoint minimum;
                PPoint maximum;
                style.SliderGetPreferredSize(*target, data, &minimum, &maximum);
                EXPECT_EQ(minimum, isHorizontal ? PPoint(41.0f, thickness) : PPoint(thickness, 41.0f));
                for (size_t state = 0; state < 4; ++state)
                {
                    data.IsPointerOver = state == 1;
                    data.IsPressed = state == 2;
                    data.IsEnabled = state != 3;
                    const PPoint first = style.SliderGetValuePosition(*target, data, isHorizontal ? 0.0f : 1.0f);
                    const PPoint last = style.SliderGetValuePosition(*target, data, isHorizontal ? 1.0f : 0.0f);
                    EXPECT_EQ(first, isHorizontal ? PPoint(9.0f, leadingSpace) : PPoint(leadingSpace, 9.0f));
                    EXPECT_EQ(last, isHorizontal ? PPoint(168.0f, leadingSpace) : PPoint(leadingSpace, 168.0f));
                    for (float value : {0.0f, 0.5f, 1.0f})
                    {
                        const PPoint position = style.SliderGetValuePosition(*target, data, value);
                        EXPECT_NEAR(style.SliderGetValueFromPosition(*target, data, position), value, 0.01f);
                    }
                }
            }
        }
    });
}

TEST(SkinnableGUIStyle, SliderAnchorsAllowOffsetsOutsideTheBitmapAndRejectNonfiniteCoordinates)
{
    RunGUITest([](PApplication&)
    {
        PSliderSkin skin;
        PSliderKnobAppearance& appearance = skin.Horizontal.Knobs[0].States[std::to_underlying(PButtonSkinState::Normal)];
        appearance.Bitmap = ptr_new<PBitmap>(31, 19, PEColorSpace::RGBA32);
        appearance.Anchor = PPoint(-4.0f, 23.0f);
        EXPECT_EQ(appearance.GetBounds(), PRect(4.0f, -23.0f, 35.0f, -4.0f));
        EXPECT_NO_THROW((PSkinnableGUIStyle(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), skin)));
        for (float value : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
        {
            appearance.Anchor = PPoint(value, 0.0f);
            EXPECT_THROW((PSkinnableGUIStyle(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), skin)), std::system_error);
            appearance.Anchor = PPoint(0.0f, value);
            EXPECT_THROW((PSkinnableGUIStyle(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), skin)), std::system_error);
        }
    });
}

TEST(SkinnableGUIStyle, UnconfiguredSlidersUseDefaultGeometryAndPainting)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("default_slider_skin", PViewFlag::WillDraw);
        target->SetFrame(PRect::FromSize(180.0f, 60.0f));
        PSliderStyleData data;
        data.Value = 0.4f;
        PDefaultGUIStyle defaultStyle;
        PSkinnableGUIStyle style;
        PPoint expectedMinimum;
        PPoint expectedMaximum;
        defaultStyle.SliderGetPreferredSize(*target, data, &expectedMinimum, &expectedMaximum);
        PPoint minimum;
        PPoint maximum;
        style.SliderGetPreferredSize(*target, data, &minimum, &maximum);
        EXPECT_EQ(minimum, expectedMinimum);
        EXPECT_EQ(maximum, expectedMaximum);
        style.SliderPaint(*target, target->GetNormalizedBounds(), data);
        EXPECT_EQ(target->GetBackgroundBrush(), application.GetStandardBrush(PStandardColorID::SliderTrackNormal));
        EXPECT_EQ(style.SliderGetValuePosition(*target, data, 0.4f), defaultStyle.SliderGetValuePosition(*target, data, 0.4f));
    });
}

TEST(SkinnableGUIStyle, SliderTrackSourcesStayImmutableAcrossViewsResizingAndColorOverrides)
{
    RunGUITest([](PApplication& application)
    {
        PSliderSkin skin;
        Ptr<PBitmap> mask = ptr_new<PBitmap>(16, 5, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(mask->LockRaster().Get()), 16 * 5, PColor(128, 128, 128).GetColor32());
        Ptr<PImage> source = ptr_new<PImage>(mask);
        source->SetSliceRect(PRect(2, 2, 14, 3));
        source->SetLayoutSize(PPoint(16, 5), PImageLayoutMode::Absolute);
        const PImageDescription description = source->GetDescription();
        skin.Horizontal.Track = source;
        skin.Vertical.Track = source;
        Ptr<PBitmap> knob = ptr_new<PBitmap>(30, 42, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(knob->LockRaster().Get()), 30 * 42, PColor(220, 230, 240, 128).GetColor32());
        skin.Horizontal.Knobs[0].States[std::to_underlying(PButtonSkinState::Normal)] = {knob, PPoint(14.0f, 20.0f)};
        skin.Vertical.Knobs[0].States[std::to_underlying(PButtonSkinState::Normal)] = {knob, PPoint(14.0f, 20.0f)};
        PSkinnableGUIStyle style(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), skin);
        Ptr<PView> first = ptr_new<PView>("first_slider");
        Ptr<PView> second = ptr_new<PView>("second_slider");
        first->SetFrame(PRect::FromSize(160.0f, 60.0f));
        second->SetFrame(PRect::FromSize(280.0f, 90.0f));
        const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(2, 3, 158, 58));
        first->SetDrawingRegion(clip);
        PSliderStyleData firstData;
        PSliderStyleData secondData;
        firstData.HasCustomSliderColors = true;
        firstData.SliderColor1 = PColor(180, 40, 30);
        firstData.SliderColor2 = PColor(20, 160, 80);
        secondData.Value = 0.75f;
        style.SliderPaint(*first, first->GetNormalizedBounds(), firstData);
        style.SliderPaint(*second, second->GetNormalizedBounds(), secondData);
        EXPECT_NE(firstData.StyleData, secondData.StyleData);
        EXPECT_EQ(first->GetDrawingRegion(), clip);
        EXPECT_EQ(first->GetBrush(), nullptr);
        EXPECT_EQ(source->GetDescription(), description);
        first->SetFrame(PRect::FromSize(220.0f, 80.0f));
        firstData.SliderSize = 9.0f;
        firstData.SliderColor1 = PColor(20, 60, 200, 128);
        style.SliderPropertyChanged(*first, firstData, PSliderProperties(PSliderProperty::SliderSize, PSliderProperty::SliderColors));
        style.SliderPaint(*first, first->GetNormalizedBounds(), firstData);
        EXPECT_EQ(source->GetDescription(), description);
        EXPECT_EQ(PColor::FromRGB32A(reinterpret_cast<const uint32_t*>(mask->LockRaster().Get())[0]), PColor(128, 128, 128));
        first->SetFrame(PRect());
        EXPECT_NO_THROW(style.SliderPaint(*first, first->GetNormalizedBounds(), firstData));
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, XMLScrollBarsUseSkinGeometryForBothOrientationsAndAllStates)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("scrollbar_skins", PViewFlag::WillDraw);
        target->SetFrame(PRect::FromSize(200, 20));
        target->Hide();
        ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
        PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
        for (const char* theme : {"Normal", "Dark"})
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            PSkinnableGUIStyle style(directory, &application);
            for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
            {
                const bool horizontal = orientation == POrientation::Horizontal;
                target->SetFrame(PRect::FromSize(horizontal ? PPoint(200, 20) : PPoint(20, 200)));
                PScrollBarStyleData data;
                data.Orientation = orientation;
                data.Proportion = 0.1f;
                PPoint minimum;
                PPoint maximum;
                style.ScrollBarGetPreferredSize(*target, data, &minimum, &maximum);
                EXPECT_EQ(minimum, horizontal ? PPoint(64, 20) : PPoint(20, 64));
                EXPECT_EQ(maximum, horizontal ? PPoint(COORD_MAX, 20) : PPoint(20, COORD_MAX));
                EXPECT_EQ(style.ScrollBarHitTest(*target, data, horizontal ? PPoint(9, 10) : PPoint(10, 9)), PScrollBarPart::DecrementStart);
                EXPECT_EQ(style.ScrollBarHitTest(*target, data, horizontal ? PPoint(27, 10) : PPoint(10, 27)), PScrollBarPart::IncrementStart);
                EXPECT_EQ(style.ScrollBarHitTest(*target, data, horizontal ? PPoint(173, 10) : PPoint(10, 173)), PScrollBarPart::DecrementEnd);
                EXPECT_EQ(style.ScrollBarHitTest(*target, data, horizontal ? PPoint(191, 10) : PPoint(10, 191)), PScrollBarPart::IncrementEnd);
                const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(target->GetNormalizedBounds()));
                target->SetDrawingRegion(clip);
                for (float proportion : {0.001f, 0.25f, 0.75f})
                {
                    data.Proportion = proportion;
                    for (float value : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
                    {
                        data.Value = value;
                        const PPoint position = style.ScrollBarGetValuePosition(*target, data, value);
                        EXPECT_NEAR(style.ScrollBarGetValueFromPosition(*target, data, position), value, 0.025f);
                        EXPECT_EQ(style.ScrollBarHitTest(*target, data, position), PScrollBarPart::Knob);
                        for (PScrollBarPart part : {PScrollBarPart::Knob, PScrollBarPart::DecrementStart, PScrollBarPart::PageIncrement})
                        {
                            for (size_t state = 0; state < std::to_underlying(PButtonSkinState::Count); ++state)
                            {
                                data.IsEnabled = state != std::to_underlying(PButtonSkinState::Disabled);
                                data.HoveredPart = (state == std::to_underlying(PButtonSkinState::Hovered)) ? part : PScrollBarPart::None;
                                data.PressedPart = (state == std::to_underlying(PButtonSkinState::Pressed)) ? part : PScrollBarPart::None;
                                target->SetDrawingMode(PDrawingMode::Overlay);
                                style.ScrollBarPaint(*target, target->GetNormalizedBounds(), data);
                                EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Overlay);
                                EXPECT_EQ(target->GetDrawingRegion(), clip);
                                EXPECT_EQ(target->GetBrush(), nullptr);
                                EXPECT_NE(target->GetBackgroundBrush(), nullptr);
                                EXPECT_EQ(style.ScrollBarGetValuePosition(*target, data, value), position);
                            }
                        }
                        data.IsEnabled = true;
                    }
                }
                data.HasRange = false;
                EXPECT_EQ(style.ScrollBarHitTest(*target, data, PPoint(9, 9)), PScrollBarPart::None);
                EXPECT_EQ(style.ScrollBarGetValuePosition(*target, data, 0.0f), style.ScrollBarGetValuePosition(*target, data, 1.0f));
                target->SetDrawingRegion(nullptr);
                target->SetFrame(PRect::FromSize(horizontal ? PPoint(10, 20) : PPoint(20, 10)));
                EXPECT_NO_THROW(style.ScrollBarPaint(*target, target->GetNormalizedBounds(), data));
                target->SetFrame(PRect());
                EXPECT_NO_THROW(style.ScrollBarPaint(*target, target->GetNormalizedBounds(), data));
            }
        }
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, ScrollBarMetricsAndImagesAreIndependentOfDefaultGeometryAndOtherViews)
{
    RunGUITest([](PApplication&)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(20, 20, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster().Get()), 400, PColor(90, 120, 150).GetColor32());
        Ptr<PImage> source = ptr_new<PImage>(bitmap);
        source->SetSliceRect(PRect(4, 4, 16, 16));
        source->SetLayoutSize(PPoint(20, 20), PImageLayoutMode::Absolute);
        const PImageDescription original = source->GetDescription();
        PScrollBarSkin skin;
        for (PScrollBarOrientationSkin* orientation : {&skin.Horizontal, &skin.Vertical})
        {
            for (PScrollBarSkinPart* part : {&orientation->Track, &orientation->Knob, &orientation->Decrement, &orientation->Increment}) {
                part->States[0].Image = source;
            }
        }
        skin.Horizontal.Thickness = 22;
        skin.Horizontal.ArrowLength = 17;
        skin.Horizontal.MinimumKnobLength = 32;
        skin.Horizontal.KnobInsets = PRect(3, 2, 5, 4);
        skin.Horizontal.DoubleArrows = false;
        skin.Vertical.Thickness = 26;
        skin.Vertical.ArrowLength = 11;
        skin.Vertical.MinimumKnobLength = 18;
        skin.Vertical.KnobInsets = PRect(2, 3, 4, 5);
        PSkinnableGUIStyle style(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), PSliderSkin(), skin);
        Ptr<PView> first = ptr_new<PView>("first_scrollbar");
        Ptr<PView> second = ptr_new<PView>("second_scrollbar");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool horizontal = orientation == POrientation::Horizontal;
            PScrollBarStyleData firstData;
            firstData.Orientation = orientation;
            firstData.Proportion = 0.001f;
            first->SetFrame(PRect::FromSize(horizontal ? PPoint(200, 22) : PPoint(26, 200)));
            PPoint minimum;
            PPoint maximum;
            style.ScrollBarGetPreferredSize(*first, firstData, &minimum, &maximum);
            EXPECT_EQ(minimum, horizontal ? PPoint(74, 22) : PPoint(26, 48));
            EXPECT_EQ(style.ScrollBarGetValuePosition(*first, firstData, 0.0f), horizontal ? PPoint(36, 10) : PPoint(12, 34));
            EXPECT_EQ(style.ScrollBarGetValuePosition(*first, firstData, 1.0f), horizontal ? PPoint(162, 10) : PPoint(12, 164));
            PScrollBarStyleData secondData;
            secondData.Orientation = orientation;
            secondData.Value = 0.75f;
            second->SetFrame(PRect::FromSize(horizontal ? PPoint(280, 22) : PPoint(26, 280)));
            style.ScrollBarPaint(*first, first->GetNormalizedBounds(), firstData);
            style.ScrollBarPaint(*second, second->GetNormalizedBounds(), secondData);
            EXPECT_NE(firstData.StyleData, secondData.StyleData);
            EXPECT_NE(first->GetBackgroundBrush(), second->GetBackgroundBrush());
            EXPECT_EQ(source->GetDescription(), original);
            first->SetFrame(second->GetFrame());
            firstData.PressedPart = PScrollBarPart::Knob;
            style.ScrollBarPropertyChanged(*first, firstData, PScrollBarProperty::Frame);
            style.ScrollBarPaint(*first, first->GetNormalizedBounds(), firstData);
            EXPECT_EQ(source->GetDescription(), original);
        }
    });
}

TEST(SkinnableGUIStyle, MissingXMLAndInvalidTextColorsAreRejected)
{
    RunGUITest([](PApplication& application)
    {
        const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, "Styles/Skinned/Normal/Button/ButtonNormal.png"));
        EXPECT_THROW(PSkinnableGUIStyle(directory, &application), std::system_error);
    });
    PColor color;
    EXPECT_TRUE(p_xml_object_parser::parse("#25354A", color));
    EXPECT_EQ(color, PColor(37, 53, 74));
    EXPECT_FALSE(p_xml_object_parser::parse("#25354Z", color));
    EXPECT_FALSE(p_xml_object_parser::parse("#25354", color));
    EXPECT_FALSE(p_xml_object_parser::parse("25354A", color));
}

TEST(BitmapLoader, ShortReadsPreserveAlphaAndTruncatedPNGIsRejected)
{
    RunGUITest([](PApplication& application)
    {
        const PString path = PStandardPaths::GetPath(PStandardPath::GUI, "Styles/Skinned/Normal/Button/ButtonNormal.png");
        PFile file(path);
        ASSERT_TRUE(file.IsValid());
        PString data;
        ASSERT_TRUE(file.Read(data));
        ASSERT_GT(data.size(), 12u);

        PButtonSkinTestStream complete(std::string_view(data.data(), data.size()));
        const Ptr<PBitmap> bitmap = PBitmapLoader::Load(complete, &application);
        ASSERT_NE(bitmap, nullptr);
        const PBitmapRasterRO raster = std::as_const(*bitmap).LockRaster();
        const uint32_t* pixels = reinterpret_cast<const uint32_t*>(raster.Get());
        EXPECT_EQ(PColor::FromRGB32A(pixels[0]).GetAlpha(), 0);
        EXPECT_EQ(PColor::FromRGB32A(pixels[24 * size_t(bitmap->GetBytesPerRow()) / sizeof(uint32_t) + 24]).GetAlpha(), 255);

        PButtonSkinTestStream truncated(std::string_view(data.data(), data.size() - 12));
        EXPECT_EQ(PBitmapLoader::Load(truncated, &application), nullptr);
    });
}

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

TEST(SkinnableGUIStyle, SliceBordersSetMinimumWithoutAddingLabelPaddingAndBrushChangesPreserveOrigins)
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
        EXPECT_EQ(minimum, PPoint(30, data.LabelSize.y + 2));
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
        EXPECT_EQ(first->GetBackgroundBrush(), nullptr);
        EXPECT_EQ(second->GetBackgroundBrush(), nullptr);
        EXPECT_EQ(first->GetBrush(), nullptr);
        EXPECT_EQ(second->GetBrush(), nullptr);
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

TEST(SkinnableGUIStyle, XMLTabViewsUseTheSameFramesForPaintingScrollingAndHitTesting)
{
    RunGUITest([](PApplication& application)
    {
        for (const char* theme : {"Normal", "Dark"})
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            const PSkinnableGUIStyle style(directory, &application);
            Ptr<PView> target = ptr_new<PView>("tab_frame", PViewFlag::WillDraw);
            Ptr<PView> tabsTarget = ptr_new<PView>("tab_strip", PViewFlag::WillDraw);
            target->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
            tabsTarget->SetFont(target->GetFont());
            target->SetFrame(PRect(0, 0, 240, 140));
            target->AddChild(tabsTarget);
            target->Hide();
            ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
            PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
            PTabViewStyleData data;
            data.Titles = {"First long tab", "Second long tab", "Third long tab"};
            data.Selection = 0;
            data.HasTopBarClient = true;
            data.TopBarMinSize = PPoint(50, 20);
            data.TopBarMaxSize = PPoint(70, 20);
            PTabViewFrames frames = style.TabViewGetFrames(*target, data);
            tabsTarget->SetFrame(frames.TabsFrame);
            const PPoint content = style.TabViewGetTabsContentSize(*target, data);
            EXPECT_GT(content.x, frames.TabsFrame.Width());
            EXPECT_EQ(content.y, frames.TabsFrame.Height());
            EXPECT_FLOAT_EQ(frames.ClientFrame.left, 3.0f);
            EXPECT_FLOAT_EQ(frames.ClientFrame.right, 237.0f);
            EXPECT_FLOAT_EQ(frames.ClientFrame.bottom, 137.0f);
            EXPECT_EQ(frames.ClientFrame.top, frames.TabsFrame.bottom);
            EXPECT_LE(frames.TabsFrame.right, frames.TopBarClientFrame.left);
            EXPECT_GE(frames.TopBarClientFrame.Width(), 50.0f);
            const PPoint firstPosition(10.0f, frames.TabsFrame.Height() * 0.5f);
            EXPECT_EQ(style.TabViewGetTabFromPosition(*target, *tabsTarget, data, firstPosition), 0u);
            EXPECT_EQ(style.TabViewGetTabFromPosition(*target, *tabsTarget, data, PPoint(-1, firstPosition.y)), INVALID_INDEX);
            const PTabViewProperties appearanceProperties(PTabViewProperty::Selection, PTabViewProperty::Highlight);
            for (size_t selected = 0; selected < data.Titles.size(); ++selected)
            {
                data.Selection = selected;
                for (PTabSkinState appearance : {PTabSkinState::Normal, PTabSkinState::Hovered, PTabSkinState::Pressed})
                {
                    data.HighlightedTab = (appearance == PTabSkinState::Normal) ? INVALID_INDEX : selected;
                    data.PressedTab = (appearance == PTabSkinState::Pressed) ? selected : INVALID_INDEX;
                    style.TabViewPropertyChanged(*target, *tabsTarget, data, appearanceProperties, INVALID_INDEX);
                    tabsTarget->SetDrawingMode(PDrawingMode::Overlay);
                    const Ptr<const PRegion> region = ptr_new<PRegion>(PIRect(tabsTarget->GetNormalizedBounds()));
                    tabsTarget->SetDrawingRegion(region);
                    style.TabViewPaint(*target, *tabsTarget, target->GetBounds(), data);
                    style.TabViewTabsPaint(*tabsTarget, *target, tabsTarget->GetBounds(), data);
                    EXPECT_EQ(tabsTarget->GetDrawingMode(), PDrawingMode::Overlay);
                    EXPECT_EQ(tabsTarget->GetDrawingRegion(), region);
                    EXPECT_EQ(tabsTarget->GetBrush(), nullptr);
                    EXPECT_EQ(target->GetBackgroundBrush(), style.GetBackgroundBrush());
                    EXPECT_EQ(style.TabViewGetFrames(*target, data).ClientFrame, frames.ClientFrame);
                }
            }
            tabsTarget->SetDrawingRegion(nullptr);
            tabsTarget->ScrollTo(PPoint(frames.TabsFrame.Width() - content.x, 0.0f));
            const PPoint lastPosition(tabsTarget->GetBounds().right - 10.0f, firstPosition.y);
            EXPECT_EQ(style.TabViewGetTabFromPosition(*target, *tabsTarget, data, lastPosition), 2u);
            style.TabViewTabsPaint(*tabsTarget, *target, tabsTarget->GetBounds(), data);
            const float oldWidth = content.x;
            data.Titles[1] = "A much longer renamed tab";
            style.TabViewPropertyChanged(*target, *tabsTarget, data, PTabViewProperty::Title, 1);
            EXPECT_GT(style.TabViewGetTabsContentSize(*target, data).x, oldWidth);
            const PPoint smallFontContent = style.TabViewGetTabsContentSize(*target, data);
            target->SetFont(ptr_new<PFont>(PFontID::e_FontLarge));
            tabsTarget->SetFont(target->GetFont());
            style.TabViewPropertyChanged(*target, *tabsTarget, data, PTabViewProperty::Font, INVALID_INDEX);
            EXPECT_GT(style.TabViewGetTabsContentSize(*target, data).x, smallFontContent.x);
            EXPECT_GT(style.TabViewGetTabsContentSize(*target, data).y, smallFontContent.y);
            for (const PPoint& size : {PPoint(1200, 140), PPoint(10, 5), PPoint(0, 0)})
            {
                target->SetFrame(PRect::FromSize(size));
                frames = style.TabViewGetFrames(*target, data);
                EXPECT_GE(frames.ClientFrame.Width(), 0.0f);
                EXPECT_GE(frames.ClientFrame.Height(), 0.0f);
                EXPECT_GE(frames.TabsFrame.Width(), 0.0f);
                tabsTarget->SetFrame(frames.TabsFrame);
                style.TabViewPaint(*target, *tabsTarget, target->GetBounds(), data);
                style.TabViewTabsPaint(*tabsTarget, *target, tabsTarget->GetBounds(), data);
            }
            data.Titles.clear();
            data.Selection = INVALID_INDEX;
            data.HighlightedTab = INVALID_INDEX;
            data.PressedTab = INVALID_INDEX;
            style.TabViewPropertyChanged(*target, *tabsTarget, data, PTabViewProperty::TabDeleted, 0);
            EXPECT_EQ(style.TabViewGetTabsContentSize(*target, data), PPoint());
        }
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, TabViewMetricsAreDataDrivenAndSharedImagesRemainUnchanged)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PBitmap> bitmap = ptr_new<PBitmap>(40, 32, PEColorSpace::RGB32);
        const Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(6, 6, 34, 28));
        const PImageDescription description = image->GetDescription();
        PTabViewSkin skin;
        skin.Frame = image;
        skin.States[0].Image = image;
        skin.SelectedStates[0].Image = image;
        skin.FrameInsets = PRect(7, 5, 9, 6);
        skin.LabelPadding = PRect(-2, 3, 8, 4);
        skin.MinimumTabSize = PPoint(80, 34);
        skin.TabSpacing = -8.0f;
        skin.InactiveTopInset = 4.0f;
        skin.OverflowWidth = 10.0f;
        skin.States[0].LabelOffset = PPoint(-100, -100);
        const PSkinnableGUIStyle style(PButtonSkin(), PCheckBoxSkin(), PRadioButtonSkin(), PSliderSkin(), PScrollBarSkin(), skin, &application);
        Ptr<PView> target = ptr_new<PView>("custom_tabs", PViewFlag::WillDraw);
        Ptr<PView> tabsTarget = ptr_new<PView>("custom_strip", PViewFlag::WillDraw);
        target->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
        tabsTarget->SetFont(target->GetFont());
        target->SetFrame(PRect(0, 0, 600, 200));
        target->AddChild(tabsTarget);
        target->Hide();
        ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
        PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
        PTabViewStyleData data;
        data.Titles = {"A", "B"};
        const PTabViewFrames frames = style.TabViewGetFrames(*target, data);
        EXPECT_EQ(frames.ClientFrame, PRect(7, 39, 591, 194));
        tabsTarget->SetFrame(frames.TabsFrame);
        EXPECT_EQ(style.TabViewGetTabsContentSize(*target, data), PPoint(152, 39));
        PPoint minimum;
        PPoint maximum;
        style.TabViewGetPreferredSize(*target, data, PPoint(200, 100), PPoint(400, 300), &minimum, &maximum);
        EXPECT_EQ(minimum, PPoint(216, 145));
        EXPECT_EQ(maximum, PPoint(416, 345));
        EXPECT_EQ(style.TabViewGetTabFromPosition(*target, *tabsTarget, data, PPoint(75, 20)), 1u);
        data.Selection = 0;
        EXPECT_EQ(style.TabViewGetTabFromPosition(*target, *tabsTarget, data, PPoint(75, 20)), 0u);
        EXPECT_EQ(style.TabViewGetTabFromPosition(*target, *tabsTarget, data, PPoint(20, 37)), 0u);
        EXPECT_EQ(style.TabViewGetTabFromPosition(*target, *tabsTarget, data, PPoint(110, 37)), INVALID_INDEX);
        const Ptr<const PRegion> region = ptr_new<PRegion>(PIRect(tabsTarget->GetNormalizedBounds()));
        tabsTarget->SetDrawingRegion(region);
        style.TabViewPaint(*target, *tabsTarget, target->GetBounds(), data);
        style.TabViewTabsPaint(*tabsTarget, *target, tabsTarget->GetBounds(), data);
        EXPECT_EQ(tabsTarget->GetDrawingRegion(), region);
        EXPECT_EQ(image->GetDescription(), description);
        PTabViewStyleData otherData;
        otherData.Titles = {"A different view"};
        target->SetFrame(PRect(0, 0, 180, 80));
        tabsTarget->SetFrame(style.TabViewGetFrames(*target, otherData).TabsFrame);
        style.TabViewPaint(*target, *tabsTarget, target->GetBounds(), otherData);
        style.TabViewTabsPaint(*tabsTarget, *target, tabsTarget->GetBounds(), otherData);
        EXPECT_NE(data.StyleData, otherData.StyleData);
        EXPECT_EQ(image->GetDescription(), description);
        application.Sync();
    });
}
