// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string_view>
#include <type_traits>

#include <GUI/BitmapLoader.h>
#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Style/SliderStyleData.h>
#include <GUI/Style/SliderStyleLayout.h>
#include <GUI/Style/ViewRendererButton.h>
#include <GUI/Style/ViewRendererScrollBar.h>
#include <GUI/Style/ViewRendererSlider.h>
#include <GUI/Style/ViewRendererTabView.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/Styles/Skinned/ViewRendererButtonSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererCheckBoxSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererRadioButtonSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererScrollBarSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererSliderSkinned.h>
#include <GUI/Styles/Skinned/ViewRendererTabViewSkinned.h>
#include <GUI/View.h>
#include <Storage/File.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>
#include <Utils/XMLObjectParser.h>

#include "GUITest.h"
#include "SkinnableGUIStyleTest.h"
#include "ViewRendererTest.h"

static PPoint GetSkinnedTestButtonLabelSize(const PView& target, const PButtonStyleData& data)
{
    const PFontHeight fontHeight = target.GetFontHeight();
    return PPoint(target.GetStringWidth(data.Label), fontHeight.ascender + fontHeight.descender + fontHeight.line_gap);
}

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

class PGUILabelPaletteTestStyle : public PDefaultGUIStyle
{
public:
    explicit PGUILabelPaletteTestStyle(PApplication* application, uint8_t colorOffset) : PDefaultGUIStyle(application)
    {
        for (size_t index = 0; index < std::to_underlying(PGUIColorID::Count); ++index) {
            SetColor(PGUIColorID(index), PColor(uint8_t(index + colorOffset), 20, 40));
        }
    }
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
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("loaded_skin", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 160, 48));
        for (size_t themeIndex = 0; themeIndex < THEMES.size(); ++themeIndex)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", THEMES[themeIndex])));
            const Ptr<const PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>(directory, &application);
            application.SetGUIStyle(style);
            const PBrush background = style->GetBrush(PGUIColorID::Background);
            ASSERT_TRUE(background.IsValid());
            EXPECT_EQ(background.GetApplication(), &application);
            EXPECT_EQ(background.GetType(), PBrushType::Bitmap);
            EXPECT_EQ(background.GetStyle(), PBrushStyle::TexturePattern);
            ASSERT_NE(background.GetBitmap(), nullptr);
            EXPECT_EQ(background.GetBitmap()->GetBounds(), PIRect(0, 0, 64, 64));
            EXPECT_TRUE(background.IsOpaque());
            EXPECT_EQ(background.GetImage().GetDescription().LayoutMode, PImageLayoutMode::Absolute);
            EXPECT_EQ(background.GetImage().GetDescription().LayoutSize, PPoint(64, 64));
            EXPECT_EQ(background.GetScale(), PPoint(1.0f));
            EXPECT_EQ(style->GetBrush(PGUIColorID::Background), background);
            target->SetBackgroundBrush(PGUIColorID::Background);
            EXPECT_EQ(target->GetBackgroundBrush(), background);
            PButtonStyleData& data = target->GetButtonStyleData();
            data = PButtonStyleData();
            PViewRendererTestBinding<PViewRendererButton> buttonRenderer(*target, *style, PGUIComponentID::Button);
            data.Label = "Loaded button";
            for (size_t index = 0; index < 8; ++index)
            {
                const size_t state = index % 4;
                data.IsChecked = index >= 4;
                data.IsPointerOver = state >= 1;
                data.IsPressed = state >= 2;
                target->SetButtonEnabled(state != 3);
                buttonRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), buttonRenderer.GetContext());
                const PBrush brush = target->GetBackgroundBrush();
                ASSERT_TRUE(brush.IsValid());
                ASSERT_TRUE(brush.GetImage().IsValid());
                EXPECT_EQ(brush.GetApplication(), &application);
                EXPECT_EQ(brush.GetBitmap()->GetBounds(), PIRect(0, 0, 48, 48));
                EXPECT_EQ(brush.GetBitmap()->GetColorSpace(), PEColorSpace::RGBA32);
                EXPECT_EQ(brush.GetImage().GetSliceRect(), PRect(12, 12, 36, 36));
                EXPECT_EQ(brush.GetImage().GetLayoutMode(), PImageLayoutMode::ViewRelative);
                EXPECT_EQ(brush.GetImage().GetLayoutSize(), PPoint(1, 1));
                EXPECT_EQ(brush.GetImage().GetDescription().OpaqueSlices, 0x00ba);
                EXPECT_EQ(target->GetFgColor(), PColor::FromRGB32(LABEL_COLORS[themeIndex][index]));
                PPoint minimum;
                PPoint maximum;
                buttonRenderer->GetPreferredSize(*target, &minimum, &maximum, buttonRenderer.GetContext());
                EXPECT_EQ(minimum, GetSkinnedTestButtonLabelSize(*target, data) + PPoint(24, 8));
                target->SetButtonEnabled(state != 3);
                buttonRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), buttonRenderer.GetContext());
                EXPECT_EQ(target->GetBackgroundBrush(), brush);
            }
        }
    });
}

TEST(SkinnableGUIStyle, StyleColorsRemainAvailableWithAndWithoutBackgroundTextures)
{
    RunGUITest([](PApplication& application)
    {
        PSkinnableGUIStyleTestDirectory directory("StyleColorsTest");
        for (bool textured : {true, false})
        {
            directory.WriteFile("Style.xml", [textured](pugi::xml_node root)
            {
                root.attribute("color_foreground").set_value("#E1D2C3");
                root.attribute("color_background").set_value("#123456");
                if (!textured) {
                    root.remove_attribute("background_texture");
                }
            });
            const PSkinnableGUIStyle style(directory.GetPath(), &application);
            EXPECT_EQ(style.GetColor(PGUIColorID::Foreground), PColor::FromRGB32(0xe1d2c3));
            EXPECT_EQ(style.GetColor(PGUIColorID::Background), PColor::FromRGB32(0x123456));
            const PBrush background = style.GetBrush(PGUIColorID::Background);
            ASSERT_TRUE(background.IsValid());
            EXPECT_EQ(background.GetType(), textured ? PBrushType::Bitmap : PBrushType::Solid);
            EXPECT_EQ(style.GetBrush(PGUIColorID::Background), background);
            if (!textured) {
                EXPECT_EQ(background.GetColor(), style.GetColor(PGUIColorID::Background));
            }
        }
    });
}

TEST(SkinnableGUIStyle, XMLCheckBoxesKeepIndicatorSizeAndLabelPositionAcrossAllStates)
{
    RunGUITest([](PApplication& application)
    {
        constexpr std::array<const char*, 2> THEMES = {"Normal", "Dark"};
        constexpr std::array<PGUIColorID, 8> LABEL_COLORS =
        {
            PGUIColorID::CheckBoxLabelNormal,
            PGUIColorID::CheckBoxLabelHovered,
            PGUIColorID::CheckBoxLabelPressed,
            PGUIColorID::CheckBoxLabelDisabled,
            PGUIColorID::CheckBoxLabelCheckedNormal,
            PGUIColorID::CheckBoxLabelCheckedHovered,
            PGUIColorID::CheckBoxLabelCheckedPressed,
            PGUIColorID::CheckBoxLabelCheckedDisabled
        };
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("checkbox_skin", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 240, 96));
        const PRect frame(7, 11, 227, 75);
        const PFontHeight fontHeight = target->GetFontHeight();
        const float baseline = std::round(11.0f + (64.0f - fontHeight.ascender - fontHeight.descender) * 0.5f) + fontHeight.ascender - 1.0f;
        for (size_t themeIndex = 0; themeIndex < THEMES.size(); ++themeIndex)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", THEMES[themeIndex])));
            const Ptr<const PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>(directory, &application);
            application.SetGUIStyle(style);
            PButtonStyleData& data = target->GetButtonStyleData();
            data = PButtonStyleData();
            PViewRendererTestBinding<PViewRendererCheckBox> checkBoxRenderer(*target, *style, PGUIComponentID::CheckBox);
            data.Label = "Option";
            for (size_t index = 0; index < 8; ++index)
            {
                const size_t state = index % 4;
                data.IsChecked = index >= 4;
                data.IsPointerOver = state >= 1;
                data.IsPressed = state >= 2;
                target->SetButtonEnabled(state != 3);
                const PColor expectedLabelColor = style->GetColor(LABEL_COLORS[index]);
                target->SetDrawingMode(PDrawingMode::Blend);
                target->SetButtonEnabled(state != 3);
                checkBoxRenderer->Paint(*target, frame, target->GetBounds(), checkBoxRenderer.GetContext());
                EXPECT_EQ(target->GetFgColor(), expectedLabelColor);
                EXPECT_EQ(target->GetPenPosition(), PPoint(47, baseline));
                EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(7, 27));
                EXPECT_FALSE(target->GetBackgroundBrush().IsValid());
                EXPECT_FALSE(target->GetBrush().IsValid());
                EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                PPoint minimum;
                PPoint maximum;
                checkBoxRenderer->GetPreferredSize(*target, &minimum, &maximum, checkBoxRenderer.GetContext());
                EXPECT_EQ(minimum, PPoint(GetSkinnedTestButtonLabelSize(*target, data).x + 40, 32));
                EXPECT_EQ(maximum, minimum);
            }
            data.Label.clear();
            checkBoxRenderer->PropertyChanged(*target, PButtonProperty::Label, checkBoxRenderer.GetContext());
            PPoint minimum;
            PPoint maximum;
            checkBoxRenderer->GetPreferredSize(*target, &minimum, &maximum, checkBoxRenderer.GetContext());
            EXPECT_EQ(minimum, PPoint(32, 32));
            target->SetButtonEnabled(true);
            checkBoxRenderer->Paint(*target, frame, target->GetBounds(), checkBoxRenderer.GetContext());
            EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(7, 27));
        }
    });
}

TEST(SkinnableGUIStyle, XMLIndicatorsRequirePositiveSizesAndAllowNegativeSpacing)
{
    RunGUITest([](PApplication& application)
    {
        PSkinnableGUIStyleTestDirectory directory("IndicatorSizes");
        ASSERT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        for (const char* fileName : {"CheckBox.xml", "RadioButton.xml"})
        {
            directory.WriteFile(fileName, [](pugi::xml_node root) { root.attribute("size").set_value("0"); });
            EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
            directory.WriteFile(fileName, [](pugi::xml_node root)
            {
                root.attribute("label_spacing").set_value("-1");
                if (std::string_view(root.name()) == "RadioButton") {
                    root.attribute("label_vertical_spacing").set_value("-1");
                }
            });
            EXPECT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
            directory.WriteFile(fileName);
        }
    });
}

TEST(SkinnableGUIStyle, XMLRadioButtonsKeepLayoutAcrossStatesAndLabelAlignments)
{
    RunGUITest([](PApplication& application)
    {
        constexpr std::array<const char*, 2> THEMES = {"Normal", "Dark"};
        constexpr std::array<PGUIColorID, 8> LABEL_COLORS =
        {
            PGUIColorID::RadioButtonLabelNormal,
            PGUIColorID::RadioButtonLabelHovered,
            PGUIColorID::RadioButtonLabelPressed,
            PGUIColorID::RadioButtonLabelDisabled,
            PGUIColorID::RadioButtonLabelCheckedNormal,
            PGUIColorID::RadioButtonLabelCheckedHovered,
            PGUIColorID::RadioButtonLabelCheckedPressed,
            PGUIColorID::RadioButtonLabelCheckedDisabled
        };
        constexpr std::array<PAlignment, 5> ALIGNMENTS = {PAlignment::Right, PAlignment::Left, PAlignment::Top, PAlignment::Bottom, PAlignment::Center};
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("radio_skin", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        const PPoint origin(7, 11);
        const PFontHeight fontHeight = target->GetFontHeight();
        const float labelHeight = fontHeight.ascender + fontHeight.descender;
        for (const char* theme : THEMES)
        {
            const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", theme)));
            const Ptr<const PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>(directory, &application);
            application.SetGUIStyle(style);
            PButtonStyleData& data = target->GetButtonStyleData();
            data = PButtonStyleData();
            PViewRendererTestBinding<PViewRendererRadioButton> radioButtonRenderer(*target, *style, PGUIComponentID::RadioButton);
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
                radioButtonRenderer->GetPreferredSize(*target, &minimum, &maximum, radioButtonRenderer.GetContext());
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
                    target->SetButtonEnabled(state != 3);
                    const PColor expectedLabelColor = style->GetColor(LABEL_COLORS[index]);
                    target->SetDrawingMode(PDrawingMode::Blend);
                    target->SetButtonEnabled(state != 3);
                    radioButtonRenderer->Paint(*target, frame, target->GetBounds(), radioButtonRenderer.GetContext());
                    EXPECT_EQ(target->GetFgColor(), expectedLabelColor);
                    EXPECT_EQ(target->GetPenPosition(), expectedLabel);
                    EXPECT_EQ(target->GetDrawingBrushOrigin(), expectedIndicator);
                    EXPECT_FALSE(target->GetBackgroundBrush().IsValid());
                    EXPECT_FALSE(target->GetBrush().IsValid());
                    EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                }
            }
            data.Label.clear();
            radioButtonRenderer->PropertyChanged(*target, PButtonProperty::Label, radioButtonRenderer.GetContext());
            PPoint minimum;
            PPoint maximum;
            radioButtonRenderer->GetPreferredSize(*target, &minimum, &maximum, radioButtonRenderer.GetContext());
            EXPECT_EQ(minimum, PPoint(32, 32));
            EXPECT_EQ(maximum, minimum);
            target->SetFrame(PRect(0, 0, 120, 90));
            target->SetButtonEnabled(true);
            radioButtonRenderer->Paint(*target, PRect(7, 11, 107, 75), target->GetBounds(), radioButtonRenderer.GetContext());
            EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(41, 27));
        }
    });
}

TEST(SkinnableGUIStyle, NegativeSpacingKeepsBothElementsWithinPreferredBoundsAndRestoresClips)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("overlapping_indicators", PViewFlag::WillDraw);
        target->SetFrame(PRect::FromSize(320.0f, 256.0f));
        target->Hide();
        ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
        PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
        target->ScrollBy(PPoint(-3, -5));
        const Ptr<const PRegion> originalClip = ptr_new<PRegion>(PIRect(4, 6, 300, 240));
        PButtonStyleData& data = target->GetButtonStyleData();
        data = PButtonStyleData();
        data.Label = "Option";
        const float labelWidth = target->GetStringWidth(data.Label);
        const PFontHeight fontHeight = target->GetFontHeight();
        constexpr std::array<PAlignment, 4> ALIGNMENTS = {PAlignment::Right, PAlignment::Left, PAlignment::Top, PAlignment::Bottom};
        for (float spacing : {-8.0f, -128.0f})
        {
            PCheckBoxSkin checkBoxSkin;
            checkBoxSkin.LabelSpacing = spacing;
            checkBoxSkin.States[0].Background = PBrush(PColor(70, 110, 160, 128));
            PRadioButtonSkin radioSkin;
            radioSkin.LabelSpacing = spacing;
            radioSkin.LabelVerticalSpacing = spacing;
            radioSkin.States[0].Background = checkBoxSkin.States[0].Background;
            const Ptr<const PViewRendererCheckBox> sharedCheckBoxRenderer = ptr_new<PViewRendererCheckBoxSkinned>(checkBoxSkin);
            const Ptr<const PViewRendererRadioButton> sharedRadioButtonRenderer = ptr_new<PViewRendererRadioButtonSkinned>(radioSkin);
            PViewRendererTestBinding<PViewRendererCheckBox> checkBoxRenderer(*target, sharedCheckBoxRenderer);
            PViewRendererTestBinding<PViewRendererRadioButton> radioButtonRenderer(*target, sharedRadioButtonRenderer);
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
                        checkBoxRenderer->GetPreferredSize(*target, &minimum, &maximum, checkBoxRenderer.GetContext());
                    } else {
                        radioButtonRenderer->GetPreferredSize(*target, &minimum, &maximum, radioButtonRenderer.GetContext());
                    }
                    EXPECT_EQ(maximum, minimum);
                    const PRect frame(PPoint(11, 13), PPoint(11, 13) + minimum);
                    target->SetDrawingRegion(originalClip);
                    target->SetDrawingMode(PDrawingMode::Blend);
                    if (checkBox)
                    {
                        target->SetButtonEnabled(true);
                        checkBoxRenderer->Paint(*target, frame, target->GetBounds(), checkBoxRenderer.GetContext());
                    }
                    else
                    {
                        target->SetButtonEnabled(true);
                        radioButtonRenderer->Paint(*target, frame, target->GetBounds(), radioButtonRenderer.GetContext());
                    }
                    EXPECT_EQ(target->GetDrawingRegion(), originalClip);
                    EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                    EXPECT_FALSE(target->GetBrush().IsValid());
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
            target->SetButtonEnabled(true);
            radioButtonRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), radioButtonRenderer.GetContext());
            EXPECT_EQ(target->GetDrawingRegion(), nullptr);
            target->SetDrawingRegion(PRegion());
            const Ptr<const PRegion> emptyClip = target->GetDrawingRegion();
            target->SetButtonEnabled(true);
            checkBoxRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), checkBoxRenderer.GetContext());
            EXPECT_EQ(target->GetDrawingRegion(), emptyClip);
            EXPECT_TRUE(target->GetDrawingRegion()->IsEmpty());
        }
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, XMLIndicatorSkinsRejectNonFiniteMetrics)
{
    RunGUITest([](PApplication& application)
    {
        PSkinnableGUIStyleTestDirectory directory("IndicatorMetrics");
        ASSERT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        for (const char* fileName : {"CheckBox.xml", "RadioButton.xml"})
        {
            for (const char* metric : {"size", "label_spacing", "label_vertical_offset"})
            {
                for (const char* value : {"inf", "-inf", "nan"})
                {
                    directory.WriteFile(fileName, [metric, value](pugi::xml_node root) { root.attribute(metric).set_value(value); });
                    EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
                }
            }
            directory.WriteFile(fileName);
        }
        for (const char* value : {"inf", "-inf", "nan"})
        {
            directory.WriteFile("RadioButton.xml", [value](pugi::xml_node root) { root.attribute("label_vertical_spacing").set_value(value); });
            EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
        }
    });
}

TEST(SliderStyleLayout, MeasuresAndTracksDamageIndependentlyOfOtherLayouts)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> target = ptr_new<PView>("independent_slider_layout");
        target->SetFrame(PRect::FromSize(240.0f, 200.0f));
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            PSliderStyleLayout layout;
            PSliderStyleLayout otherLayout;
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
            EXPECT_FALSE(otherLayout.HasVisualState());
            layout.PropertyChanged(*target, data, PSliderProperty::Appearance);
            EXPECT_TRUE(layout.HasVisualState());
            data.Value = 0.75f;
            layout.PropertyChanged(*target, data, PSliderProperty::Value);
            EXPECT_FALSE(otherLayout.HasVisualState());
        }
    });
}

TEST(SkinnableGUIStyle, XMLSlidersKeepGeometryAndDrawingStateStableAcrossStatesForBothThemes)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("slider_skins", PViewFlag::WillDraw);
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
                    PSliderStyleData& geometryData = target->GetSliderStyleData();
                    geometryData = PSliderStyleData();
                    PViewRendererTestBinding<PViewRendererSlider> geometrySliderRenderer(*target, style, PGUIComponentID::Slider);
                    geometryData.Orientation = orientation;
                    geometryData.KnobPointUp = shape % 2 != 0;
                    geometryData.KnobPointDown = shape >= 2;
                    PPoint nativeMinimum;
                    PPoint nativeMaximum;
                    geometrySliderRenderer->GetPreferredSize(*target, &nativeMinimum, &nativeMaximum, geometrySliderRenderer.GetContext());
                    const PPoint expectedSize = isHorizontal ? BITMAP_SIZES[shape] : PPoint(BITMAP_SIZES[shape].y, BITMAP_SIZES[shape].x);
                    const PPoint expectedAnchor = isHorizontal ? ANCHORS[shape] : PPoint(ANCHORS[shape].y, ANCHORS[shape].x);
                    EXPECT_EQ(nativeMinimum, expectedSize);
                    target->SetFrame(PRect::FromSize(isHorizontal ? PPoint(240.0f, nativeMinimum.y) : PPoint(nativeMinimum.x, 200.0f)));
                    EXPECT_EQ(
                        geometrySliderRenderer->GetValuePosition(*target, isHorizontal ? 0.0f : 1.0f, geometrySliderRenderer.GetContext()),
                        expectedAnchor);
                    target->SetFrame(PRect::FromSize(240.0f, 200.0f));
                    PSliderStyleData& data = target->GetSliderStyleData();
                    data = PSliderStyleData();
                    PViewRendererTestBinding<PViewRendererSlider> sliderRenderer(*target, style, PGUIComponentID::Slider);
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
                    sliderRenderer->GetPreferredSize(*target, &minimum, &maximum, sliderRenderer.GetContext());
                    EXPECT_GT(minimum.x, 0.0f);
                    EXPECT_GT(minimum.y, 0.0f);
                    EXPECT_GE(maximum.x, minimum.x);
                    EXPECT_GE(maximum.y, minimum.y);
                    PtrTarget* const privateState = sliderRenderer.GetContext();
                    for (float value : {0.0f, 0.5f, 1.0f})
                    {
                        data.Value = value;
                        const PPoint position = sliderRenderer->GetValuePosition(*target, value, sliderRenderer.GetContext());
                        EXPECT_NEAR(sliderRenderer->GetValueFromPosition(*target, position, sliderRenderer.GetContext()), value, 0.01f);
                        sliderRenderer->PropertyChanged(*target, PSliderProperty::Value, sliderRenderer.GetContext());
                        for (size_t state = 0; state < 4; ++state)
                        {
                            data.IsPointerOver = state >= 1;
                            data.IsPressed = state >= 2;
                            data.IsEnabled = state != 3;
                            sliderRenderer->PropertyChanged(
                                *target,
                                PSliderProperties(PSliderProperty::Pressed, PSliderProperty::Hovered, PSliderProperty::Enabled),
                                sliderRenderer.GetContext());
                            target->SetDrawingRegion(clip);
                            target->SetDrawingMode(PDrawingMode::Blend);
                            sliderRenderer->Paint(*target, target->GetNormalizedBounds(), target->GetBounds(), sliderRenderer.GetContext());
                            EXPECT_EQ(target->GetDrawingRegion(), clip);
                            EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Blend);
                            EXPECT_FALSE(target->GetBrush().IsValid());
                            EXPECT_FALSE(target->GetBackgroundBrush().IsValid());
                            EXPECT_EQ(sliderRenderer.GetContext(), privateState);
                            EXPECT_EQ(sliderRenderer->GetValuePosition(*target, value, sliderRenderer.GetContext()), position);
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
        PImage track(trackBitmap);
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("slider_anchor_layout");
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
            const Ptr<const PViewRendererSlider> sharedRenderer = ptr_new<PViewRendererSliderSkinned>(skin);
            for (size_t appearance = 0; appearance < 16; ++appearance)
            {
                const size_t knobShape = appearance % 4;
                const size_t tickSides = appearance / 4;
                PSliderStyleData& data = target->GetSliderStyleData();
                data = PSliderStyleData();
                PViewRendererTestBinding<PViewRendererSlider> sliderRenderer(*target, sharedRenderer);
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
                sliderRenderer->GetPreferredSize(*target, &minimum, &maximum, sliderRenderer.GetContext());
                EXPECT_EQ(minimum, isHorizontal ? PPoint(41.0f, thickness) : PPoint(thickness, 41.0f));
                for (size_t state = 0; state < 4; ++state)
                {
                    data.IsPointerOver = state == 1;
                    data.IsPressed = state == 2;
                    data.IsEnabled = state != 3;
                    const PPoint first = sliderRenderer->GetValuePosition(*target, isHorizontal ? 0.0f : 1.0f, sliderRenderer.GetContext());
                    const PPoint last = sliderRenderer->GetValuePosition(*target, isHorizontal ? 1.0f : 0.0f, sliderRenderer.GetContext());
                    EXPECT_EQ(first, isHorizontal ? PPoint(9.0f, leadingSpace) : PPoint(leadingSpace, 9.0f));
                    EXPECT_EQ(last, isHorizontal ? PPoint(168.0f, leadingSpace) : PPoint(leadingSpace, 168.0f));
                    for (float value : {0.0f, 0.5f, 1.0f})
                    {
                        const PPoint position = sliderRenderer->GetValuePosition(*target, value, sliderRenderer.GetContext());
                        EXPECT_NEAR(sliderRenderer->GetValueFromPosition(*target, position, sliderRenderer.GetContext()), value, 0.01f);
                    }
                }
            }
        }
    });
}

TEST(SkinnableGUIStyle, XMLSliderAnchorsAllowOffsetsOutsideTheBitmapAndRejectNonfiniteCoordinates)
{
    RunGUITest([](PApplication& application)
    {
        PSliderKnobAppearance appearance;
        appearance.Bitmap = ptr_new<PBitmap>(31, 19, PEColorSpace::RGBA32);
        appearance.Anchor = PPoint(-4.0f, 23.0f);
        EXPECT_EQ(appearance.GetBounds(), PRect(4.0f, -23.0f, 35.0f, -4.0f));
        PSkinnableGUIStyleTestDirectory directory("SliderAnchors");
        ASSERT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        directory.WriteFile("Slider.xml", [](pugi::xml_node root) { root.child("Horizontal").child("Plain").attribute("anchor").set_value("Point(-4, 23)"); });
        EXPECT_NO_THROW(PSkinnableGUIStyle(directory.GetPath(), &application));
        for (const char* anchor : {"Point(inf, 0)", "Point(-inf, 0)", "Point(nan, 0)", "Point(0, inf)", "Point(0, -inf)", "Point(0, nan)"})
        {
            directory.WriteFile("Slider.xml", [anchor](pugi::xml_node root) { root.child("Horizontal").child("Plain").attribute("anchor").set_value(anchor); });
            EXPECT_THROW(PSkinnableGUIStyle(directory.GetPath(), &application), std::system_error);
        }
    });
}

TEST(SkinnableGUIStyle, SliderTrackSourcesStayImmutableAcrossViewsResizingAndColorOverrides)
{
    RunGUITest([](PApplication& application)
    {
        PSliderSkin skin;
        Ptr<PBitmap> mask = ptr_new<PBitmap>(16, 5, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(mask->LockRaster().Get()), 16 * 5, PColor(128, 128, 128).GetColor32());
        PImage source(mask);
        source.SetSliceRect(PRect(2, 2, 14, 3));
        source.SetLayoutSize(PPoint(16, 5), PImageLayoutMode::Absolute);
        const PImageDescription description = source.GetDescription();
        skin.Horizontal.Track = source;
        skin.Vertical.Track = source;
        Ptr<PBitmap> knob = ptr_new<PBitmap>(30, 42, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(knob->LockRaster().Get()), 30 * 42, PColor(220, 230, 240, 128).GetColor32());
        skin.Horizontal.Knobs[0].States[std::to_underlying(PButtonSkinState::Normal)] = {knob, PPoint(14.0f, 20.0f)};
        skin.Vertical.Knobs[0].States[std::to_underlying(PButtonSkinState::Normal)] = {knob, PPoint(14.0f, 20.0f)};
        const Ptr<const PViewRendererSlider> sharedRenderer = ptr_new<PViewRendererSliderSkinned>(skin);
        Ptr<PViewRendererTestView> first = ptr_new<PViewRendererTestView>("first_slider");
        Ptr<PViewRendererTestView> second = ptr_new<PViewRendererTestView>("second_slider");
        first->SetFrame(PRect::FromSize(160.0f, 60.0f));
        second->SetFrame(PRect::FromSize(280.0f, 90.0f));
        const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(2, 3, 158, 58));
        first->SetDrawingRegion(clip);
        PSliderStyleData& firstData = first->GetSliderStyleData();
        firstData = PSliderStyleData();
        PViewRendererTestBinding<PViewRendererSlider> firstSliderRenderer(*first, sharedRenderer);
        PSliderStyleData& secondData = second->GetSliderStyleData();
        secondData = PSliderStyleData();
        PViewRendererTestBinding<PViewRendererSlider> secondSliderRenderer(*second, sharedRenderer);
        firstData.HasCustomSliderColors = true;
        firstData.SliderColor1 = PColor(180, 40, 30);
        firstData.SliderColor2 = PColor(20, 160, 80);
        secondData.Value = 0.75f;
        firstSliderRenderer->Paint(*first, first->GetNormalizedBounds(), first->GetBounds(), firstSliderRenderer.GetContext());
        secondSliderRenderer->Paint(*second, second->GetNormalizedBounds(), second->GetBounds(), secondSliderRenderer.GetContext());
        EXPECT_NE(firstSliderRenderer.GetContext(), secondSliderRenderer.GetContext());
        EXPECT_EQ(first->GetDrawingRegion(), clip);
        EXPECT_FALSE(first->GetBrush().IsValid());
        EXPECT_EQ(source.GetDescription(), description);
        first->SetFrame(PRect::FromSize(220.0f, 80.0f));
        firstData.SliderSize = 9.0f;
        firstData.SliderColor1 = PColor(20, 60, 200, 128);
        firstSliderRenderer->PropertyChanged(
            *first,
            PSliderProperties(PSliderProperty::SliderSize, PSliderProperty::SliderColors),
            firstSliderRenderer.GetContext());
        firstSliderRenderer->Paint(*first, first->GetNormalizedBounds(), first->GetBounds(), firstSliderRenderer.GetContext());
        EXPECT_EQ(source.GetDescription(), description);
        EXPECT_EQ(PColor::FromRGB32A(reinterpret_cast<const uint32_t*>(mask->LockRaster().Get())[0]), PColor(128, 128, 128));
        first->SetFrame(PRect());
        EXPECT_NO_THROW(firstSliderRenderer->Paint(*first, first->GetNormalizedBounds(), first->GetBounds(), firstSliderRenderer.GetContext()));
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, XMLScrollBarsUseSkinGeometryForBothOrientationsAndAllStates)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("scrollbar_skins", PViewFlag::WillDraw);
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
                PScrollBarStyleData& data = target->GetScrollBarStyleData();
                data = PScrollBarStyleData();
                PViewRendererTestBinding<PViewRendererScrollBar> scrollBarRenderer(*target, style, PGUIComponentID::ScrollBar);
                data.Orientation = orientation;
                data.Proportion = 0.1f;
                PPoint minimum;
                PPoint maximum;
                scrollBarRenderer->GetPreferredSize(*target, &minimum, &maximum, scrollBarRenderer.GetContext());
                EXPECT_EQ(minimum, horizontal ? PPoint(64, 20) : PPoint(20, 64));
                EXPECT_EQ(maximum, horizontal ? PPoint(COORD_MAX, 20) : PPoint(20, COORD_MAX));
                EXPECT_EQ(
                    scrollBarRenderer->HitTest(*target, horizontal ? PPoint(9, 10) : PPoint(10, 9), scrollBarRenderer.GetContext()),
                    PScrollBarPart::DecrementStart);
                EXPECT_EQ(
                    scrollBarRenderer->HitTest(*target, horizontal ? PPoint(27, 10) : PPoint(10, 27), scrollBarRenderer.GetContext()),
                    PScrollBarPart::IncrementStart);
                EXPECT_EQ(
                    scrollBarRenderer->HitTest(*target, horizontal ? PPoint(173, 10) : PPoint(10, 173), scrollBarRenderer.GetContext()),
                    PScrollBarPart::DecrementEnd);
                EXPECT_EQ(
                    scrollBarRenderer->HitTest(*target, horizontal ? PPoint(191, 10) : PPoint(10, 191), scrollBarRenderer.GetContext()),
                    PScrollBarPart::IncrementEnd);
                const Ptr<const PRegion> clip = ptr_new<PRegion>(PIRect(target->GetNormalizedBounds()));
                target->SetDrawingRegion(clip);
                for (float proportion : {0.001f, 0.25f, 0.75f})
                {
                    data.Proportion = proportion;
                    for (float value : {0.0f, 0.25f, 0.5f, 0.75f, 1.0f})
                    {
                        data.Value = value;
                        const PPoint position = scrollBarRenderer->GetValuePosition(*target, value, scrollBarRenderer.GetContext());
                        EXPECT_NEAR(scrollBarRenderer->GetValueFromPosition(*target, position, scrollBarRenderer.GetContext()), value, 0.025f);
                        EXPECT_EQ(scrollBarRenderer->HitTest(*target, position, scrollBarRenderer.GetContext()), PScrollBarPart::Knob);
                        for (PScrollBarPart part : {PScrollBarPart::Knob, PScrollBarPart::DecrementStart, PScrollBarPart::PageIncrement})
                        {
                            for (size_t state = 0; state < std::to_underlying(PButtonSkinState::Count); ++state)
                            {
                                data.IsEnabled = state != std::to_underlying(PButtonSkinState::Disabled);
                                data.HoveredPart = (state == std::to_underlying(PButtonSkinState::Hovered)) ? part : PScrollBarPart::None;
                                data.PressedPart = (state == std::to_underlying(PButtonSkinState::Pressed)) ? part : PScrollBarPart::None;
                                target->SetDrawingMode(PDrawingMode::Overlay);
                                scrollBarRenderer->Paint(*target, target->GetNormalizedBounds(), target->GetBounds(), scrollBarRenderer.GetContext());
                                EXPECT_EQ(target->GetDrawingMode(), PDrawingMode::Overlay);
                                EXPECT_EQ(target->GetDrawingRegion(), clip);
                                EXPECT_FALSE(target->GetBrush().IsValid());
                                EXPECT_TRUE(target->GetBackgroundBrush().IsValid());
                                EXPECT_EQ(scrollBarRenderer->GetValuePosition(*target, value, scrollBarRenderer.GetContext()), position);
                            }
                        }
                        data.IsEnabled = true;
                    }
                }
                data.HasRange = false;
                EXPECT_EQ(scrollBarRenderer->HitTest(*target, PPoint(9, 9), scrollBarRenderer.GetContext()), PScrollBarPart::None);
                EXPECT_EQ(
                    scrollBarRenderer->GetValuePosition(*target, 0.0f, scrollBarRenderer.GetContext()),
                    scrollBarRenderer->GetValuePosition(*target, 1.0f, scrollBarRenderer.GetContext()));
                target->SetDrawingRegion(nullptr);
                target->SetFrame(PRect::FromSize(horizontal ? PPoint(10, 20) : PPoint(20, 10)));
                EXPECT_NO_THROW(scrollBarRenderer->Paint(*target, target->GetNormalizedBounds(), target->GetBounds(), scrollBarRenderer.GetContext()));
                target->SetFrame(PRect());
                EXPECT_NO_THROW(scrollBarRenderer->Paint(*target, target->GetNormalizedBounds(), target->GetBounds(), scrollBarRenderer.GetContext()));
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
        PImage source(bitmap);
        source.SetSliceRect(PRect(4, 4, 16, 16));
        source.SetLayoutSize(PPoint(20, 20), PImageLayoutMode::Absolute);
        const PImageDescription original = source.GetDescription();
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
        const Ptr<const PViewRendererScrollBar> sharedRenderer = ptr_new<PViewRendererScrollBarSkinned>(skin);
        Ptr<PViewRendererTestView> first = ptr_new<PViewRendererTestView>("first_scrollbar");
        Ptr<PViewRendererTestView> second = ptr_new<PViewRendererTestView>("second_scrollbar");
        for (POrientation orientation : {POrientation::Horizontal, POrientation::Vertical})
        {
            const bool horizontal = orientation == POrientation::Horizontal;
            PScrollBarStyleData& firstData = first->GetScrollBarStyleData();
            firstData = PScrollBarStyleData();
            PViewRendererTestBinding<PViewRendererScrollBar> firstScrollBarRenderer(*first, sharedRenderer);
            firstData.Orientation = orientation;
            firstData.Proportion = 0.001f;
            first->SetFrame(PRect::FromSize(horizontal ? PPoint(200, 22) : PPoint(26, 200)));
            PPoint minimum;
            PPoint maximum;
            firstScrollBarRenderer->GetPreferredSize(*first, &minimum, &maximum, firstScrollBarRenderer.GetContext());
            EXPECT_EQ(minimum, horizontal ? PPoint(74, 22) : PPoint(26, 48));
            EXPECT_EQ(
                firstScrollBarRenderer->GetValuePosition(*first, 0.0f, firstScrollBarRenderer.GetContext()),
                horizontal ? PPoint(36, 10) : PPoint(12, 34));
            EXPECT_EQ(
                firstScrollBarRenderer->GetValuePosition(*first, 1.0f, firstScrollBarRenderer.GetContext()),
                horizontal ? PPoint(162, 10) : PPoint(12, 164));
            PScrollBarStyleData& secondData = second->GetScrollBarStyleData();
            secondData = PScrollBarStyleData();
            PViewRendererTestBinding<PViewRendererScrollBar> secondScrollBarRenderer(*second, sharedRenderer);
            secondData.Orientation = orientation;
            secondData.Value = 0.75f;
            second->SetFrame(PRect::FromSize(horizontal ? PPoint(280, 22) : PPoint(26, 280)));
            firstScrollBarRenderer->Paint(*first, first->GetNormalizedBounds(), first->GetBounds(), firstScrollBarRenderer.GetContext());
            secondScrollBarRenderer->Paint(*second, second->GetNormalizedBounds(), second->GetBounds(), secondScrollBarRenderer.GetContext());
            EXPECT_NE(firstScrollBarRenderer.GetContext(), secondScrollBarRenderer.GetContext());
            EXPECT_NE(first->GetBackgroundBrush(), second->GetBackgroundBrush());
            EXPECT_EQ(source.GetDescription(), original);
            first->SetFrame(second->GetFrame());
            firstData.PressedPart = PScrollBarPart::Knob;
            firstScrollBarRenderer->PropertyChanged(*first, PScrollBarProperty::Frame, firstScrollBarRenderer.GetContext());
            firstScrollBarRenderer->Paint(*first, first->GetNormalizedBounds(), first->GetBounds(), firstScrollBarRenderer.GetContext());
            EXPECT_EQ(source.GetDescription(), original);
        }
    });
}

TEST(SkinnableGUIStyle, MissingXMLAndInvalidTextColorsAreRejected)
{
    RunGUITest([](PApplication& application)
    {
        const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, "Styles/Skinned/Normal/Button/ButtonNormal.png"));
        EXPECT_THROW(PSkinnableGUIStyle(directory, &application), std::system_error);
        PSkinnableGUIStyleTestDirectory paletteDirectory("InvalidPaletteTest");
        for (const char* attribute : {"color_foreground_disabled", "color_group_label", "color_button_label_checked_disabled",
                                     "color_check_box_label_normal", "color_radio_button_label_pressed", "color_tab_label_selected_hovered",
                                     "color_dropdown_text_normal", "color_dropdown_background_editable_disabled"})
        {
            paletteDirectory.WriteFile("Style.xml", [attribute](pugi::xml_node root) { root.remove_attribute(attribute); });
            EXPECT_THROW(PSkinnableGUIStyle(paletteDirectory.GetPath(), &application), std::system_error);
            paletteDirectory.WriteFile("Style.xml", [attribute](pugi::xml_node root) { root.attribute(attribute).set_value("#GGGGGG"); });
            EXPECT_THROW(PSkinnableGUIStyle(paletteDirectory.GetPath(), &application), std::system_error);
        }
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

TEST(SkinnableGUIStyle, SharedButtonRenderersReadTheActivePalette)
{
    RunGUITest([](PApplication& application)
    {
        constexpr std::array<std::array<PGUIColorID, 8>, 3> LABEL_COLORS =
        {{
            {
                PGUIColorID::ButtonLabelNormal,
                PGUIColorID::ButtonLabelHovered,
                PGUIColorID::ButtonLabelPressed,
                PGUIColorID::ButtonLabelDisabled,
                PGUIColorID::ButtonLabelCheckedNormal,
                PGUIColorID::ButtonLabelCheckedHovered,
                PGUIColorID::ButtonLabelCheckedPressed,
                PGUIColorID::ButtonLabelCheckedDisabled
            },
            {
                PGUIColorID::CheckBoxLabelNormal,
                PGUIColorID::CheckBoxLabelHovered,
                PGUIColorID::CheckBoxLabelPressed,
                PGUIColorID::CheckBoxLabelDisabled,
                PGUIColorID::CheckBoxLabelCheckedNormal,
                PGUIColorID::CheckBoxLabelCheckedHovered,
                PGUIColorID::CheckBoxLabelCheckedPressed,
                PGUIColorID::CheckBoxLabelCheckedDisabled
            },
            {
                PGUIColorID::RadioButtonLabelNormal,
                PGUIColorID::RadioButtonLabelHovered,
                PGUIColorID::RadioButtonLabelPressed,
                PGUIColorID::RadioButtonLabelDisabled,
                PGUIColorID::RadioButtonLabelCheckedNormal,
                PGUIColorID::RadioButtonLabelCheckedHovered,
                PGUIColorID::RadioButtonLabelCheckedPressed,
                PGUIColorID::RadioButtonLabelCheckedDisabled
            }
        }};
        const PBrush background(PColor(90, 90, 90), &application);
        PButtonSkin buttonSkin;
        buttonSkin.States[0].Background = background;
        PCheckBoxSkin checkBoxSkin;
        checkBoxSkin.States[0].Background = background;
        PRadioButtonSkin radioButtonSkin;
        radioButtonSkin.States[0].Background = background;
        const std::array<Ptr<const PViewRendererButtonBase>, 4> renderers =
        {
            ptr_new<PViewRendererButtonSkinned>(buttonSkin),
            ptr_new<PViewRendererCheckBoxSkinned>(checkBoxSkin),
            ptr_new<PViewRendererRadioButtonSkinned>(radioButtonSkin),
            ptr_dynamic_cast<const PViewRendererButtonBase>(PGUIStyle::CreateDefaultRenderer(PGUIComponentID::Button))
        };
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("active_palette", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 240, 60));
        PButtonStyleData& data = target->GetButtonStyleData();
        data.Label = "Palette label";
        for (size_t rendererIndex = 0; rendererIndex < renderers.size(); ++rendererIndex)
        {
            PViewRendererTestBinding<PViewRendererButtonBase> renderer(*target, renderers[rendererIndex]);
            for (uint8_t colorOffset : {uint8_t(20), uint8_t(100)})
            {
                application.SetGUIStyle(ptr_new<PGUILabelPaletteTestStyle>(&application, colorOffset));
                const std::array<PGUIColorID, 8>& colors = LABEL_COLORS[rendererIndex % LABEL_COLORS.size()];
                for (size_t index = 0; index < colors.size(); ++index)
                {
                    const size_t state = index % 4;
                    data.IsChecked = index >= 4;
                    data.IsPointerOver = state >= 1;
                    data.IsPressed = state >= 2;
                    target->SetButtonEnabled(state != 3);
                    renderer->Paint(*target, target->GetBounds(), target->GetBounds(), renderer.GetContext());
                    EXPECT_EQ(target->GetFgColor(), PColor(uint8_t(std::to_underlying(colors[index]) + colorOffset), 20, 40));
                }
            }
        }
    });
}

TEST(SkinnableGUIStyle, TabLabelsReadAllStatesFromTheActivePalette)
{
    RunGUITest([](PApplication& application)
    {
        constexpr std::array<PGUIColorID, 6> LABEL_COLORS =
        {
            PGUIColorID::TabLabelNormal,
            PGUIColorID::TabLabelHovered,
            PGUIColorID::TabLabelPressed,
            PGUIColorID::TabLabelSelectedNormal,
            PGUIColorID::TabLabelSelectedHovered,
            PGUIColorID::TabLabelSelectedPressed
        };
        const PPath directory(PStandardPaths::GetPath(PStandardPath::GUI, "Styles/Skinned/Normal"));
        const PSkinnableGUIStyle skinnedStyle(directory, &application);
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("palette_tab", PViewFlag::WillDraw);
        Ptr<PView> tabsTarget = ptr_new<PView>("palette_tab_strip", PViewFlag::WillDraw);
        target->AddChild(tabsTarget);
        target->SetTabsView(*tabsTarget);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 240, 140));
        PTabViewStyleData& data = target->GetTabViewStyleData();
        data.Titles = {"Palette tab"};
        PViewRendererTestBinding<PViewRendererTabView> renderer(*target, skinnedStyle, PGUIComponentID::TabView);
        for (uint8_t colorOffset : {uint8_t(20), uint8_t(100)})
        {
            application.SetGUIStyle(ptr_new<PGUILabelPaletteTestStyle>(&application, colorOffset));
            for (size_t index = 0; index < LABEL_COLORS.size(); ++index)
            {
                const size_t state = index % 3;
                data.Selection = (index >= 3) ? 0 : INVALID_INDEX;
                data.HighlightedTab = (state >= 1) ? 0 : INVALID_INDEX;
                data.PressedTab = (state == 2) ? 0 : INVALID_INDEX;
                tabsTarget->SetFrame(renderer->GetFrames(*target, renderer.GetContext()).TabsFrame);
                renderer->PaintTabs(*tabsTarget, *target, tabsTarget->GetBounds(), renderer.GetContext());
                EXPECT_EQ(tabsTarget->GetFgColor(), PColor(uint8_t(std::to_underlying(LABEL_COLORS[index]) + colorOffset), 20, 40));
            }
        }
    });
}

TEST(SkinnableGUIStyle, StatePrecedenceFallbackAndLabelLayoutAreStable)
{
    RunGUITest([](PApplication& application)
    {
        PButtonSkin skin;
        for (size_t index = 0; index < skin.States.size(); ++index)
        {
            skin.States[index].Background = PBrush(PColor(uint8_t(index * 50), 0, 0));
            skin.CheckedStates[index].Background = PBrush(PColor(0, uint8_t(index * 50), 0));
        }
        const Ptr<const PViewRendererButton> sharedRenderer = ptr_new<PViewRendererButtonSkinned>(skin);
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("skin_states", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        target->SetFrame(PRect(0, 0, 160, 60));
        PButtonStyleData& data = target->GetButtonStyleData();
        data = PButtonStyleData();
        PViewRendererTestBinding<PViewRendererButton> buttonRenderer(*target, sharedRenderer);
        data.Label = "Stable size";
        PPoint normalMinimum;
        PPoint normalMaximum;
        buttonRenderer->GetPreferredSize(*target, &normalMinimum, &normalMaximum, buttonRenderer.GetContext());
        EXPECT_EQ(normalMinimum, GetSkinnedTestButtonLabelSize(*target, data) + PPoint(16, 8));
        for (bool checked : {false, true})
        {
            data.IsChecked = checked;
            for (size_t index = 0; index < skin.States.size(); ++index)
            {
                data.IsPointerOver = index >= 1;
                data.IsPressed = index >= 2;
                const bool enabled = index != 3;
                target->SetButtonEnabled(enabled);
                buttonRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), buttonRenderer.GetContext());
                const PButtonSkinAppearance& appearance = checked ? skin.CheckedStates[index] : skin.States[index];
                EXPECT_EQ(target->GetBackgroundBrush(), appearance.Background);
                EXPECT_FALSE(target->GetBrush().IsValid());
                const PGUIColorID labelColor = enabled ? PGUIColorID::ButtonLabelNormal : PGUIColorID::ButtonLabelDisabled;
                EXPECT_EQ(target->GetFgColor(), target->GetStyle()->GetColor(labelColor));
                PPoint minimum;
                PPoint maximum;
                buttonRenderer->GetPreferredSize(*target, &minimum, &maximum, buttonRenderer.GetContext());
                EXPECT_EQ(minimum, normalMinimum);
                EXPECT_EQ(maximum, normalMaximum);
            }
        }
        skin.CheckedStates[std::to_underlying(PButtonSkinState::Disabled)].Background = PBrush::zero;
        const Ptr<const PViewRendererButton> fallbackRenderer = ptr_new<PViewRendererButtonSkinned>(skin);
        PViewRendererTestBinding<PViewRendererButton> fallbackButtonRenderer(*target, fallbackRenderer);
        target->SetButtonEnabled(false);
        fallbackButtonRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), fallbackButtonRenderer.GetContext());
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
        skin.States[0].Background = PBrush(PColor(0, 0, 0));
        PImage image(bitmap);
        image.SetSliceRect(PRect(15, 11, 17, 13));
        skin.CheckedStates[2].Background = PBrush(image);
        const Ptr<const PViewRendererButton> sharedRenderer = ptr_new<PViewRendererButtonSkinned>(skin);
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("skin_bindings", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(target));
        PScopeExit removeTarget([&application, &target] { application.RemoveHandler(target); });
        PButtonStyleData& data = target->GetButtonStyleData();
        data = PButtonStyleData();
        PViewRendererTestBinding<PViewRendererButton> buttonRenderer(*target, sharedRenderer);
        PPoint minimum;
        PPoint maximum;
        buttonRenderer->GetPreferredSize(*target, &minimum, &maximum, buttonRenderer.GetContext());
        EXPECT_EQ(minimum, PPoint(30, GetSkinnedTestButtonLabelSize(*target, data).y + 2));
        const PBrush standard = application.GetStandardBrush(PStandardColorID::DefaultBackground);
        target->SetBackgroundBrush(skin.States[0].Background);
        target->SetBackgroundBrushOrigin(PPoint(12, 34));
        target->SetBrushOrigin(PPoint(-7, 9));
        target->SetBrush(skin.States[0].Background);
        target->SetBackgroundBrush(PStandardColorID::DefaultBackground);
        target->SetBrush(PStandardColorID::DefaultBackground);
        EXPECT_TRUE(target->GetBackgroundBrush().IsValid());
        EXPECT_TRUE(target->GetBrush().IsValid());
        EXPECT_EQ(target->GetBackgroundBrush().GetType(), PBrushType::Solid);
        EXPECT_EQ(target->GetBrush().GetType(), PBrushType::Solid);
        EXPECT_EQ(target->GetBackgroundBrush(), standard);
        EXPECT_EQ(target->GetBrush(), standard);
        EXPECT_EQ(target->GetBackgroundBrushOrigin(), PPoint(12, 34));
        EXPECT_EQ(target->GetDrawingBrushOrigin(), PPoint(-7, 9));
        target->SetBackgroundBrush(PBrush::zero);
        target->SetBrush(PBrush::zero);
        EXPECT_FALSE(target->GetBackgroundBrush().IsValid());
        EXPECT_FALSE(target->GetBrush().IsValid());
        EXPECT_FALSE(target->GetFlags().Has(PViewFlag::Transparent));
    });
}

TEST(Brush, StandardAndNamedColorsReturnBrushValues)
{
    static_assert(std::is_same_v<decltype(std::declval<PApplication&>().GetStandardBrush(PNamedColors::white)), PBrush>);
    static_assert(std::is_same_v<decltype(std::declval<PApplication&>().GetStandardBrush(PStandardColorID::DefaultBackground)), PBrush>);
    RunGUITest([](PApplication& application)
    {
        const PBrush standard = application.GetStandardBrush(PStandardColorID::DefaultBackground);
        const PBrush named = application.GetStandardBrush(PNamedColors::white);
        EXPECT_EQ(standard, application.GetStandardBrush(PStandardColorID::DefaultBackground));
        EXPECT_EQ(named, application.GetStandardBrush(PNamedColors::white));
        EXPECT_EQ(named.GetColor(), PColor(PNamedColors::white));
        Ptr<PView> first = ptr_new<PView>("first_brush_view", PViewFlag::WillDraw);
        Ptr<PView> second = ptr_new<PView>("second_brush_view", PViewFlag::WillDraw);
        EXPECT_FALSE(first->GetBackgroundBrush().IsValid());
        EXPECT_FALSE(second->GetBackgroundBrush().IsValid());
        EXPECT_FALSE(first->GetBrush().IsValid());
        EXPECT_FALSE(second->GetBrush().IsValid());
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

TEST(Brush, AssignedViewsRetainSnapshotsUntilExplicitRebinding)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("custom_brush_view", PViewFlag::WillDraw);
        PBrush drawing(PColor(21, 43, 65), &application);
        PBrush background(PColor(65, 43, 21), &application);
        view->SetBrush(drawing);
        view->SetBackgroundBrush(background);
        for (uint8_t value = 0; value < 16; ++value)
        {
            const PBrush previousDrawing = drawing;
            const PBrush previousBackground = background;
            drawing.SetColor(PColor(value, 43, 65));
            background.SetColor(PColor(65, 43, value));
            EXPECT_EQ(view->GetBrush(), previousDrawing);
            EXPECT_EQ(view->GetBackgroundBrush(), previousBackground);
            EXPECT_NE(view->GetBrush(), drawing);
            EXPECT_NE(view->GetBackgroundBrush(), background);
            view->SetBrush(drawing);
            view->SetBackgroundBrush(background);
            EXPECT_EQ(view->GetBrush().GetColor(), PColor(value, 43, 65));
            EXPECT_EQ(view->GetBackgroundBrush().GetColor(), PColor(65, 43, value));
        }
        PBrush detached = view->GetBrush();
        detached.SetColor(PNamedColors::green);
        EXPECT_EQ(view->GetBrush(), drawing);
        view->SetBrush(PBrush::zero);
        EXPECT_FALSE(view->GetBrush().IsValid());
        view->SetBrush(drawing);
        EXPECT_EQ(view->GetBrush(), drawing);
        view->SetBackgroundBrush(PNamedColors::white);
        EXPECT_EQ(view->GetBackgroundBrush(), application.GetStandardBrush(PNamedColors::white));
        view->SetBackgroundBrush(background);
        EXPECT_EQ(view->GetBackgroundBrush(), background);
        application.Sync();
    });
}

TEST(Brush, PropertyChangesPreserveCopiesAndValidateBeforePublishing)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(4, 4, PEColorSpace::RGBA32);
        std::fill_n(reinterpret_cast<uint32_t*>(bitmap->LockRaster().Get()), 16, PColor(20, 40, 60, 128).GetColor32());
        PBrush brush(PColor(10, 20, 30));
        const PBrush original = brush;
        PImage image(bitmap);
        image.SetSliceRect(PRect(1, 1, 3, 3));
        brush.SetImage(image);
        EXPECT_EQ(original.GetColor(), PColor(10, 20, 30));
        EXPECT_EQ(original.GetType(), PBrushType::Solid);
        EXPECT_EQ(brush.GetType(), PBrushType::Image);
        EXPECT_FALSE(brush.IsOpaque());
        EXPECT_EQ(brush.GetImage().GetDescription().OpaqueSlices, 0);
        const PBrush textured = brush;
        EXPECT_THROW(brush.SetBitmap(bitmap, PIRect(0, 0, 5, 4)), std::system_error);
        EXPECT_EQ(brush, textured);
        brush.SetColor(PColor(80, 100, 120));
        EXPECT_EQ(brush.GetType(), PBrushType::Solid);
        EXPECT_TRUE(brush.IsOpaque());
        EXPECT_EQ(brush.GetBitmap(), nullptr);
        EXPECT_FALSE(brush.GetImage().IsValid());
        EXPECT_EQ(textured.GetBitmap(), bitmap);
        EXPECT_EQ(textured.GetImage(), image);
        const PBrush solid(PColor(80, 100, 120));
        EXPECT_EQ(brush, solid);
        brush.SetColor(PColor(80, 100, 120, 0));
        EXPECT_FALSE(brush.IsOpaque());
        brush.SetColor(PColor(80, 100, 120, 128));
        EXPECT_FALSE(brush.IsOpaque());
        brush.SetColor(solid.GetColor());
        EXPECT_EQ(brush, solid);
        bitmap = nullptr;
        application.Sync();
    });
}

TEST(Brush, PaletteLookupPreservesPreviouslyReturnedValues)
{
    RunGUITest([](PApplication& application)
    {
        constexpr PStandardColorID COLOR = PStandardColorID::ListViewHighlight;
        const PColor original = pget_standard_color(COLOR);
        PScopeExit restore([original] { pset_standard_color(COLOR, original); });
        const PBrush brush = application.GetStandardBrush(COLOR);
        pset_standard_color(COLOR, PColor(19, 37, 53));
        const PBrush updated = application.GetStandardBrush(COLOR);
        EXPECT_EQ(brush.GetColor(), original);
        EXPECT_EQ(updated.GetColor(), PColor(19, 37, 53));
        EXPECT_EQ(application.GetStandardBrush(COLOR), updated);
    });
}

TEST(ImageBrush, ImageCopiesAndBrushesRetainPublishedSnapshots)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(6, 6, PEColorSpace::RGBA32);
        {
            const PBitmapRasterRW raster = bitmap->LockRaster();
            std::fill_n(reinterpret_cast<uint32_t*>(raster.Get()), 36, PColor(30, 60, 90).GetColor32());
            reinterpret_cast<uint32_t*>(raster.Get())[0] = PColor(255, 0, 0, 0).GetColor32();
        }
        PImage image(bitmap);
        EXPECT_EQ(image.GetLayoutSize(), PPoint(1, 1));
        EXPECT_EQ(image.GetLayoutMode(), PImageLayoutMode::ViewRelative);
        image.SetSliceRect(PRect(1, 1, 5, 5));
        const PImage original = image;
        PBrush first(image);
        const PBrush second = first;
        const PImageID originalImageID = original.GetImageID();
        EXPECT_EQ(first.GetImage().GetImageID(), originalImageID);
        EXPECT_EQ(second.GetImage().GetImageID(), originalImageID);
        image.SetLayoutSize(PPoint(47, 19), PImageLayoutMode::Absolute);
        image.SetTiling(PImageTilingFlag::LeftEdge | PImageTilingFlag::CenterHorizontal);
        first.SetScale(PPoint(2, 3));
        EXPECT_NE(image, original);
        EXPECT_EQ(first.GetImage(), original);
        EXPECT_EQ(second.GetImage(), original);
        EXPECT_EQ(original.GetImageID(), originalImageID);
        EXPECT_EQ(first.GetImage().GetImageID(), originalImageID);
        EXPECT_EQ(first.GetBitmap(), bitmap);
        EXPECT_EQ(second.GetBitmap(), bitmap);
        EXPECT_EQ(first.GetScale(), PPoint(2, 3));
        EXPECT_EQ(second.GetScale(), PPoint(1, 1));
        EXPECT_FALSE(first.IsOpaque());
        EXPECT_EQ(image.GetDescription().OpaqueSlices, 0x01fe);
        first.SetImage(image);
        EXPECT_EQ(first.GetImage(), image);
        EXPECT_NE(first.GetImage().GetImageID(), originalImageID);
        EXPECT_EQ(first.GetImage().GetImageID(), image.GetImageID());
        EXPECT_EQ(second.GetImage().GetImageID(), originalImageID);
        EXPECT_EQ(second.GetImage().GetLayoutSize(), PPoint(1, 1));
        EXPECT_THROW(image.SetSliceRect(PRect(-1, 1, 5, 5)), std::system_error);
        EXPECT_THROW(image.SetSliceRect(PRect(1.5f, 1, 5, 5)), std::system_error);
        EXPECT_THROW(image.SetLayoutSize(PPoint(0, 19)), std::system_error);
        EXPECT_EQ(image.GetLayoutSize(), PPoint(47, 19));
        EXPECT_EQ(image.GetLayoutMode(), PImageLayoutMode::Absolute);
        EXPECT_NO_THROW(image.SetLayoutSize(PPoint(47, 19), PImageLayoutMode(0xff)));
        EXPECT_NO_THROW(image.SetTiling(PImageTilingFlag(0x40)));
        EXPECT_THROW(first.SetScale(PPoint(-1, 1)), std::system_error);
        application.Sync();
    });
}

TEST(ImageBrush, NoneAndSolidStylesRetainTheirSourceUntilColorSelection)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PBitmap> bitmap = ptr_new<PBitmap>(3, 2, PEColorSpace::RGB32);
        PBrush brush(bitmap);
        EXPECT_EQ(brush.GetImage().GetDescription().LayoutSize, PPoint(3, 2));
        EXPECT_EQ(brush.GetImage().GetDescription().LayoutMode, PImageLayoutMode::Absolute);
        brush.SetStyle(PBrushStyle::None);
        EXPECT_EQ(brush.GetStyle(), PBrushStyle::None);
        EXPECT_EQ(brush.GetType(), PBrushType::Bitmap);
        EXPECT_EQ(brush.GetBitmap(), bitmap);
        EXPECT_FALSE(brush.IsOpaque());
        brush.SetScale(PPoint(0, 0));
        EXPECT_THROW(brush.SetStyle(PBrushStyle::TexturePattern), std::system_error);
        EXPECT_EQ(brush.GetStyle(), PBrushStyle::None);
        brush.SetScale(PPoint(1, 1));
        EXPECT_THROW(brush.SetStyle(PBrushStyle(0xff)), std::system_error);
        brush.SetStyle(PBrushStyle::SolidPattern);
        EXPECT_TRUE(brush.IsOpaque());
        brush.SetStyle(PBrushStyle::TexturePattern);
        EXPECT_EQ(brush.GetBitmap(), bitmap);
        brush.SetStyle(PBrushStyle::None);
        brush.SetColor(brush.GetColor());
        EXPECT_EQ(brush.GetStyle(), PBrushStyle::SolidPattern);
        EXPECT_EQ(brush.GetType(), PBrushType::Solid);
        EXPECT_EQ(brush.GetBitmap(), nullptr);
        EXPECT_THROW(brush.SetStyle(PBrushStyle::TexturePattern), std::system_error);
        application.Sync();
    });
}

TEST(ImageBrush, ViewsKeepIndependentOriginsAndNoneSelections)
{
    RunGUITest([](PApplication& application)
    {
        Ptr<PView> view = ptr_new<PView>("origins", PViewFlag::WillDraw);
        PBrush none(PBrushStyle::None, &application);
        view->SetBackgroundBrush(none);
        view->SetBrush(none);
        view->SetBackgroundBrushOrigin(PPoint(11, 17));
        view->SetBrushOrigin(PPoint(-13, 29));
        EXPECT_EQ(view->GetBackgroundBrush(), none);
        EXPECT_EQ(view->GetBrush(), none);
        view->SetBrush(PBrush::zero);
        EXPECT_EQ(view->GetBackgroundBrush(), none);
        EXPECT_FALSE(view->GetBrush().IsValid());
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
        PImage image(bitmap);
        image.SetSliceRect(PRect(2, 2, 4, 4));
        const PImageDescription description = image.GetDescription();
        PButtonSkin skin;
        skin.States[0].Background = PBrush(image);
        const Ptr<const PViewRendererButton> sharedRenderer = ptr_new<PViewRendererButtonSkinned>(skin);
        Ptr<PViewRendererTestView> first = ptr_new<PViewRendererTestView>("first_size", PViewFlag::WillDraw);
        Ptr<PViewRendererTestView> second = ptr_new<PViewRendererTestView>("second_size", PViewFlag::WillDraw);
        ASSERT_TRUE(application.AddHandler(first));
        ASSERT_TRUE(application.AddHandler(second));
        PScopeExit removeViews([&application, &first, &second] { application.RemoveHandler(first); application.RemoveHandler(second); });
        first->SetFrame(PRect(0, 0, 120, 40));
        second->SetFrame(PRect(0, 0, 180, 60));
        PButtonStyleData& firstData = first->GetButtonStyleData();
        firstData = PButtonStyleData();
        PViewRendererTestBinding<PViewRendererButton> firstButtonRenderer(*first, sharedRenderer);
        PButtonStyleData& secondData = second->GetButtonStyleData();
        secondData = PButtonStyleData();
        PViewRendererTestBinding<PViewRendererButton> secondButtonRenderer(*second, sharedRenderer);
        first->SetButtonEnabled(true);
        firstButtonRenderer->Paint(*first, first->GetBounds(), first->GetBounds(), firstButtonRenderer.GetContext());
        second->SetButtonEnabled(true);
        secondButtonRenderer->Paint(*second, second->GetBounds(), second->GetBounds(), secondButtonRenderer.GetContext());
        const PBrush firstBrush = first->GetBackgroundBrush();
        EXPECT_EQ(firstBrush, skin.States[0].Background);
        EXPECT_EQ(firstBrush, second->GetBackgroundBrush());
        EXPECT_EQ(firstBrush.GetBitmap(), bitmap);
        EXPECT_EQ(second->GetBackgroundBrush().GetBitmap(), bitmap);
        EXPECT_EQ(firstBrush.GetImage(), image);
        EXPECT_EQ(second->GetBackgroundBrush().GetImage(), image);
        EXPECT_EQ(image.GetLayoutSize(), PPoint(1, 1));
        EXPECT_EQ(image.GetLayoutMode(), PImageLayoutMode::ViewRelative);
        first->SetFrame(PRect(0, 0, 140, 50));
        first->SetButtonEnabled(true);
        firstButtonRenderer->Paint(*first, first->GetBounds(), first->GetBounds(), firstButtonRenderer.GetContext());
        EXPECT_EQ(first->GetBackgroundBrush(), firstBrush);
        EXPECT_EQ(second->GetBackgroundBrush(), firstBrush);
        EXPECT_EQ(image.GetDescription(), description);
        image.SetLayoutSize(PPoint(0.5f, 0.5f));
        EXPECT_EQ(image.GetLayoutMode(), PImageLayoutMode::ViewRelative);
        EXPECT_EQ(firstBrush.GetImage().GetDescription(), description);
        image.SetLayoutSize(PPoint(6, 6), PImageLayoutMode::Absolute);
        first->SetButtonEnabled(true);
        firstButtonRenderer->Paint(*first, first->GetBounds(), first->GetBounds(), firstButtonRenderer.GetContext());
        EXPECT_EQ(first->GetBackgroundBrush(), firstBrush);
        EXPECT_EQ(image.GetLayoutSize(), PPoint(6, 6));
        EXPECT_EQ(image.GetLayoutMode(), PImageLayoutMode::Absolute);
        EXPECT_EQ(first->GetBackgroundBrush().GetImage().GetDescription(), description);
        EXPECT_EQ(second->GetBackgroundBrush().GetImage().GetDescription(), description);
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
            const Ptr<const PSkinnableGUIStyle> style = ptr_new<PSkinnableGUIStyle>(directory, &application);
            application.SetGUIStyle(style);
            Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("tab_frame", PViewFlag::WillDraw);
            Ptr<PView> tabsTarget = ptr_new<PView>("tab_strip", PViewFlag::WillDraw);
            target->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
            tabsTarget->SetFont(target->GetFont());
            target->SetFrame(PRect(0, 0, 240, 140));
            target->AddChild(tabsTarget);
            target->Hide();
            ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
            PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
            PTabViewStyleData& data = target->GetTabViewStyleData();
            data = PTabViewStyleData();
            target->SetTabsView(*tabsTarget);
            PViewRendererTestBinding<PViewRendererTabView> tabViewRenderer(*target, *style, PGUIComponentID::TabView);
            data.Titles = {"First long tab", "Second long tab", "Third long tab"};
            data.Selection = 0;
            data.HasTopBarClient = true;
            data.TopBarMinSize = PPoint(50, 20);
            data.TopBarMaxSize = PPoint(70, 20);
            PTabViewFrames frames = tabViewRenderer->GetFrames(*target, tabViewRenderer.GetContext());
            tabsTarget->SetFrame(frames.TabsFrame);
            const PPoint content = tabViewRenderer->GetTabsContentSize(*target, tabViewRenderer.GetContext());
            EXPECT_GT(content.x, frames.TabsFrame.Width());
            EXPECT_EQ(content.y, frames.TabsFrame.Height());
            EXPECT_FLOAT_EQ(frames.ClientFrame.left, 3.0f);
            EXPECT_FLOAT_EQ(frames.ClientFrame.right, 237.0f);
            EXPECT_FLOAT_EQ(frames.ClientFrame.bottom, 137.0f);
            EXPECT_EQ(frames.ClientFrame.top, frames.TabsFrame.bottom);
            EXPECT_LE(frames.TabsFrame.right, frames.TopBarClientFrame.left);
            EXPECT_GE(frames.TopBarClientFrame.Width(), 50.0f);
            const PPoint firstPosition(10.0f, frames.TabsFrame.Height() * 0.5f);
            EXPECT_EQ(tabViewRenderer->GetTabFromPosition(*target, firstPosition, tabViewRenderer.GetContext()), 0u);
            EXPECT_EQ(tabViewRenderer->GetTabFromPosition(*target, PPoint(-1, firstPosition.y), tabViewRenderer.GetContext()), INVALID_INDEX);
            const PTabViewProperties appearanceProperties(PTabViewProperty::Selection, PTabViewProperty::Highlight);
            for (size_t selected = 0; selected < data.Titles.size(); ++selected)
            {
                data.Selection = selected;
                for (PTabSkinState appearance : {PTabSkinState::Normal, PTabSkinState::Hovered, PTabSkinState::Pressed})
                {
                    data.HighlightedTab = (appearance == PTabSkinState::Normal) ? INVALID_INDEX : selected;
                    data.PressedTab = (appearance == PTabSkinState::Pressed) ? selected : INVALID_INDEX;
                    tabViewRenderer->PropertyChanged(*target, appearanceProperties, INVALID_INDEX, tabViewRenderer.GetContext());
                    tabsTarget->SetDrawingMode(PDrawingMode::Overlay);
                    const Ptr<const PRegion> region = ptr_new<PRegion>(PIRect(tabsTarget->GetNormalizedBounds()));
                    tabsTarget->SetDrawingRegion(region);
                    tabViewRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), tabViewRenderer.GetContext());
                    tabViewRenderer->PaintTabs(*tabsTarget, *target, tabsTarget->GetBounds(), tabViewRenderer.GetContext());
                    EXPECT_EQ(tabsTarget->GetDrawingMode(), PDrawingMode::Overlay);
                    EXPECT_EQ(tabsTarget->GetDrawingRegion(), region);
                    EXPECT_FALSE(tabsTarget->GetBrush().IsValid());
                    EXPECT_EQ(target->GetBackgroundBrush(), style->GetBrush(PGUIColorID::Background));
                    EXPECT_EQ(tabViewRenderer->GetFrames(*target, tabViewRenderer.GetContext()).ClientFrame, frames.ClientFrame);
                }
            }
            tabsTarget->SetDrawingRegion(nullptr);
            tabsTarget->ScrollTo(PPoint(frames.TabsFrame.Width() - content.x, 0.0f));
            const PPoint lastPosition(tabsTarget->GetBounds().right - 10.0f, firstPosition.y);
            EXPECT_EQ(tabViewRenderer->GetTabFromPosition(*target, lastPosition, tabViewRenderer.GetContext()), 2u);
            tabViewRenderer->PaintTabs(*tabsTarget, *target, tabsTarget->GetBounds(), tabViewRenderer.GetContext());
            const float oldWidth = content.x;
            data.Titles[1] = "A much longer renamed tab";
            tabViewRenderer->PropertyChanged(*target, PTabViewProperty::Title, 1, tabViewRenderer.GetContext());
            EXPECT_GT(tabViewRenderer->GetTabsContentSize(*target, tabViewRenderer.GetContext()).x, oldWidth);
            const PPoint smallFontContent = tabViewRenderer->GetTabsContentSize(*target, tabViewRenderer.GetContext());
            target->SetFont(ptr_new<PFont>(PFontID::e_FontLarge));
            tabsTarget->SetFont(target->GetFont());
            tabViewRenderer->PropertyChanged(*target, PTabViewProperty::Font, INVALID_INDEX, tabViewRenderer.GetContext());
            EXPECT_GT(tabViewRenderer->GetTabsContentSize(*target, tabViewRenderer.GetContext()).x, smallFontContent.x);
            EXPECT_GT(tabViewRenderer->GetTabsContentSize(*target, tabViewRenderer.GetContext()).y, smallFontContent.y);
            for (const PPoint& size : {PPoint(1200, 140), PPoint(10, 5), PPoint(0, 0)})
            {
                target->SetFrame(PRect::FromSize(size));
                frames = tabViewRenderer->GetFrames(*target, tabViewRenderer.GetContext());
                EXPECT_GE(frames.ClientFrame.Width(), 0.0f);
                EXPECT_GE(frames.ClientFrame.Height(), 0.0f);
                EXPECT_GE(frames.TabsFrame.Width(), 0.0f);
                tabsTarget->SetFrame(frames.TabsFrame);
                tabViewRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), tabViewRenderer.GetContext());
                tabViewRenderer->PaintTabs(*tabsTarget, *target, tabsTarget->GetBounds(), tabViewRenderer.GetContext());
            }
            data.Titles.clear();
            data.Selection = INVALID_INDEX;
            data.HighlightedTab = INVALID_INDEX;
            data.PressedTab = INVALID_INDEX;
            tabViewRenderer->PropertyChanged(*target, PTabViewProperty::TabDeleted, 0, tabViewRenderer.GetContext());
            EXPECT_EQ(tabViewRenderer->GetTabsContentSize(*target, tabViewRenderer.GetContext()), PPoint());
        }
        application.Sync();
    });
}

TEST(SkinnableGUIStyle, TabViewMetricsAreDataDrivenAndSharedImagesRemainUnchanged)
{
    RunGUITest([](PApplication& application)
    {
        const Ptr<PBitmap> bitmap = ptr_new<PBitmap>(40, 32, PEColorSpace::RGB32);
        PImage image(bitmap);
        image.SetSliceRect(PRect(6, 6, 34, 28));
        const PImageDescription description = image.GetDescription();
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
        const Ptr<const PViewRendererTabView> sharedRenderer = ptr_new<PViewRendererTabViewSkinned>(
            skin,
            application.GetStandardBrush(PStandardColorID::DefaultBackground));
        Ptr<PViewRendererTestView> target = ptr_new<PViewRendererTestView>("custom_tabs", PViewFlag::WillDraw);
        Ptr<PView> tabsTarget = ptr_new<PView>("custom_strip", PViewFlag::WillDraw);
        target->SetFont(ptr_new<PFont>(PFontID::e_FontSmall));
        tabsTarget->SetFont(target->GetFont());
        target->SetFrame(PRect(0, 0, 600, 200));
        target->AddChild(tabsTarget);
        target->Hide();
        ASSERT_TRUE(application.AddView(target, PViewDockType::TopLevelView));
        PScopeExit removeTarget([&application, &target] { application.RemoveView(target); });
        PTabViewStyleData& data = target->GetTabViewStyleData();
        data = PTabViewStyleData();
        target->SetTabsView(*tabsTarget);
        PViewRendererTestBinding<PViewRendererTabView> tabViewRenderer(*target, sharedRenderer);
        data.Titles = {"A", "B"};
        const PTabViewFrames frames = tabViewRenderer->GetFrames(*target, tabViewRenderer.GetContext());
        EXPECT_EQ(frames.ClientFrame, PRect(7, 39, 591, 194));
        tabsTarget->SetFrame(frames.TabsFrame);
        EXPECT_EQ(tabViewRenderer->GetTabsContentSize(*target, tabViewRenderer.GetContext()), PPoint(152, 39));
        PPoint minimum;
        PPoint maximum;
        target->SetTabContentSizeLimits(PPoint(200, 100), PPoint(400, 300));
        tabViewRenderer->GetPreferredSize(*target, &minimum, &maximum, tabViewRenderer.GetContext());
        EXPECT_EQ(minimum, PPoint(216, 145));
        EXPECT_EQ(maximum, PPoint(416, 345));
        EXPECT_EQ(tabViewRenderer->GetTabFromPosition(*target, PPoint(75, 20), tabViewRenderer.GetContext()), 1u);
        data.Selection = 0;
        EXPECT_EQ(tabViewRenderer->GetTabFromPosition(*target, PPoint(75, 20), tabViewRenderer.GetContext()), 0u);
        EXPECT_EQ(tabViewRenderer->GetTabFromPosition(*target, PPoint(20, 37), tabViewRenderer.GetContext()), 0u);
        EXPECT_EQ(tabViewRenderer->GetTabFromPosition(*target, PPoint(110, 37), tabViewRenderer.GetContext()), INVALID_INDEX);
        const Ptr<const PRegion> region = ptr_new<PRegion>(PIRect(tabsTarget->GetNormalizedBounds()));
        tabsTarget->SetDrawingRegion(region);
        tabViewRenderer->Paint(*target, target->GetBounds(), target->GetBounds(), tabViewRenderer.GetContext());
        tabViewRenderer->PaintTabs(*tabsTarget, *target, tabsTarget->GetBounds(), tabViewRenderer.GetContext());
        EXPECT_EQ(tabsTarget->GetDrawingRegion(), region);
        EXPECT_EQ(image.GetDescription(), description);
        Ptr<PViewRendererTestView> otherTarget = ptr_new<PViewRendererTestView>("other_tabs", PViewFlag::WillDraw);
        Ptr<PView> otherTabsTarget = ptr_new<PView>("other_strip", PViewFlag::WillDraw);
        otherTarget->SetFont(target->GetFont());
        otherTabsTarget->SetFont(target->GetFont());
        otherTarget->SetFrame(PRect(0, 0, 180, 80));
        otherTarget->AddChild(otherTabsTarget);
        otherTarget->Hide();
        ASSERT_TRUE(application.AddView(otherTarget, PViewDockType::TopLevelView));
        PScopeExit removeOtherTarget([&application, &otherTarget] { application.RemoveView(otherTarget); });
        PTabViewStyleData& otherData = otherTarget->GetTabViewStyleData();
        otherData.Titles = {"A different view"};
        otherTarget->SetTabsView(*otherTabsTarget);
        PViewRendererTestBinding<PViewRendererTabView> otherTabViewRenderer(*otherTarget, sharedRenderer);
        otherTabsTarget->SetFrame(otherTabViewRenderer->GetFrames(*otherTarget, otherTabViewRenderer.GetContext()).TabsFrame);
        otherTabViewRenderer->Paint(*otherTarget, otherTarget->GetBounds(), otherTarget->GetBounds(), otherTabViewRenderer.GetContext());
        otherTabViewRenderer->PaintTabs(*otherTabsTarget, *otherTarget, otherTabsTarget->GetBounds(), otherTabViewRenderer.GetContext());
        EXPECT_NE(tabViewRenderer.GetContext(), otherTabViewRenderer.GetContext());
        EXPECT_EQ(image.GetDescription(), description);
        application.Sync();
    });
}
