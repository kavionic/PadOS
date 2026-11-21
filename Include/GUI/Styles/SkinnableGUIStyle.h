// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <utility>
#include <vector>

#include <GUI/Brush.h>
#include <GUI/Style/GUIStyle.h>
#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Style/TabViewStyleData.h>
#include <GUI/Widgets/TextBox.h>
#include <Utils/String.h>

class PPath;

namespace pugi
{
class xml_node;
}

enum class PButtonSkinState : uint8_t
{
    Normal,
    Hovered,
    Pressed,
    Disabled,
    Count
};

struct PButtonSkinAppearance
{
    Ptr<const PBrush> Background;
    PColor LabelColor = PColor(0, 0, 0);
};

struct PButtonSkin
{
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> CheckedStates;
    // Text insets are independent of the image's nine-slice borders.
    PRect LabelPadding = PRect(8.0f, 4.0f, 8.0f, 4.0f);
    PPoint PressedLabelOffset = PPoint(1.0f, 1.0f);

    PButtonSkin();
};

struct PButtonIndicatorSkin
{
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)> CheckedStates;
    // Indicator images use absolute layout with dimensions (Size, Size).
    float Size = 32.0f;
    float LabelSpacing = 8.0f;
    float LabelVerticalOffset = -1.0f;
};

using PCheckBoxSkin = PButtonIndicatorSkin;

struct PRadioButtonSkin : PButtonIndicatorSkin
{
    float LabelVerticalSpacing = 4.0f;
};

struct PSliderSkinColors
{
    PColor Before;
    PColor After;
    PColor Label;
    PColor Tick;
    PColor TickHighlight;

    bool operator==(const PSliderSkinColors& rhs) const = default;
};

struct PSliderKnobAppearance
{
    Ptr<const PBitmap> Bitmap;
    // Bitmap pixel aligned with the track; rounded to the nearest native pixel.
    PPoint Anchor;

    PRect GetBounds() const;
};

struct PSliderKnobSkin
{
    std::array<PSliderKnobAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    PSliderKnobAppearance Shadow;

    PRect GetBounds() const;
};

struct PSliderOrientationSkin
{
    Ptr<const PImage> Track;
    // Plain, leading point, trailing point, both points.
    std::array<PSliderKnobSkin, 4> Knobs;
};

struct PSliderSkin
{
    std::array<PSliderSkinColors, std::to_underlying(PButtonSkinState::Count)> Colors;
    PSliderOrientationSkin Horizontal;
    PSliderOrientationSkin Vertical;
};

struct PScrollBarSkinAppearance
{
    Ptr<const PImage> Image;
    Ptr<const PBitmap> Glyph;
    PPoint GlyphOffset;
};

struct PScrollBarSkinPart
{
    std::array<PScrollBarSkinAppearance, std::to_underlying(PButtonSkinState::Count)> States;
};

struct PScrollBarOrientationSkin
{
    PScrollBarSkinPart Track;
    PScrollBarSkinPart Knob;
    PScrollBarSkinPart Decrement;
    PScrollBarSkinPart Increment;
    PRect KnobInsets = PRect(2.0f, 2.0f, 2.0f, 2.0f);
    float Thickness = 20.0f;
    float ArrowLength = 18.0f;
    float MinimumKnobLength = 24.0f;
    bool DoubleArrows = true;
};

struct PScrollBarSkin
{
    PScrollBarOrientationSkin Horizontal;
    PScrollBarOrientationSkin Vertical;
};

enum class PTabSkinState : uint8_t
{
    Normal,
    Hovered,
    Pressed,
    Count
};

struct PTabSkinAppearance
{
    Ptr<const PImage> Image;
    PColor LabelColor = PColor(0, 0, 0);
    PPoint LabelOffset;
};

struct PTabViewSkin
{
    std::array<PTabSkinAppearance, std::to_underlying(PTabSkinState::Count)> States;
    std::array<PTabSkinAppearance, std::to_underlying(PTabSkinState::Count)> SelectedStates;
    Ptr<const PImage> Frame;
    Ptr<const PBitmap> OverflowLeft;
    Ptr<const PBitmap> OverflowRight;
    PRect FrameInsets = PRect(3.0f, 3.0f, 3.0f, 3.0f);
    PRect LabelPadding = PRect(12.0f, 4.0f, 12.0f, 6.0f);
    PPoint MinimumTabSize = PPoint(40.0f, 28.0f);
    float TabSpacing = 2.0f;
    float InactiveTopInset = 3.0f;
    float OverflowWidth = 12.0f;
};

enum class PDropdownMenuRowState : uint8_t
{
    Normal,
    Selected,
    Pressed,
    Count
};

struct PDropdownMenuAppearance
{
    Ptr<const PBrush> Background;
    Ptr<const PBitmap> Arrow;
    Ptr<const PTextBoxStyle> Editor;
    PPoint ArrowOffset;
};

struct PDropdownMenuRowAppearance
{
    Ptr<const PImage> Image;
    PColor LabelColor = PColor(0, 0, 0);
    PPoint LabelOffset;
};

struct PDropdownMenuSkin
{
    std::array<PDropdownMenuAppearance, std::to_underlying(PButtonSkinState::Count)> States;
    std::array<PDropdownMenuRowAppearance, std::to_underlying(PDropdownMenuRowState::Count)> Rows;
    Ptr<const PBrush> PopupFrame;
    Ptr<const PBrush> PopupBackground;
    PRect EditorInsets = PRect(10.0f, 4.0f, 6.0f, 5.0f);
    PRect PopupInsets = PRect(4.0f, 4.0f, 4.0f, 4.0f);
    PRect RowPadding = PRect(8.0f, 4.0f, 8.0f, 5.0f);
    PPoint MinimumSize = PPoint(72.0f, 32.0f);
    float ArrowWidth = 30.0f;
    float MinimumRowHeight = 32.0f;
};

struct PGroupViewSkin
{
    Ptr<const PImage> Frame;
    PRect FrameInsets;
    PRect LabelPadding = PRect(14.0f, 0.0f, 14.0f, 0.0f);
    PPoint MinimumSize = PPoint(28.0f, 28.0f);
    PColor LabelColor = PColor(0, 0, 0);
    float LabelGap = 4.0f;
};

struct PProgressBarOrientationSkin
{
    Ptr<const PImage> Track;
    Ptr<const PImage> Fill;
    PRect FillInsets = PRect(3.0f, 3.0f, 3.0f, 3.0f);
    float MinimumLength = 50.0f;
    float Thickness = 24.0f;
};

struct PProgressBarSkin
{
    PProgressBarOrientationSkin Horizontal;
    PProgressBarOrientationSkin Vertical;
};

class PSkinnableGUIStyle : public PGUIStyle
{
public:
    // Load Style.xml, widget XML files and their assets for this instance.
    explicit PSkinnableGUIStyle(const PPath& directory, PApplication* application = nullptr);

    static PString GetName();
    static std::vector<PString> GetVariants();
    static Ptr<const PGUIStyle> CreateStyle(PApplication* application, const PString& variant);

    virtual Ptr<const PBrush> GetBackgroundBrush() const override;

private:
    static constexpr std::array<const char*, std::to_underlying(PButtonSkinState::Count)> STATE_NAMES = {"Normal", "Hovered", "Pressed", "Disabled"};

    static constexpr std::array<const char*, std::to_underlying(PTabSkinState::Count)> TAB_STATE_NAMES = {"Normal", "Hovered", "Pressed"};

    static constexpr std::array<const char*, std::to_underlying(PDropdownMenuRowState::Count)> DROPDOWN_ROW_STATE_NAMES = {"Normal", "Selected", "Pressed"};

    static void ValidateIndicatorSkin(const PButtonIndicatorSkin& skin);
    static void ValidateSliderSkin(const PSliderSkin& skin);
    static void ValidateScrollBarSkin(const PScrollBarSkin& skin);
    static void ValidateTabViewSkin(const PTabViewSkin& skin);
    static void ValidateDropdownMenuSkin(const PDropdownMenuSkin& skin);
    static void ValidateGroupViewSkin(const PGroupViewSkin& skin);
    static void ValidateProgressBarSkin(const PProgressBarSkin& skin);
    static Ptr<const PBrush> LoadBackgroundBrush(const PPath& directory, PApplication* application);
    static PButtonSkin LoadButtonSkin(const PPath& directory, PApplication* application);
    static PCheckBoxSkin LoadCheckBoxSkin(const PPath& directory, PApplication* application);
    static PRadioButtonSkin LoadRadioButtonSkin(const PPath& directory, PApplication* application);
    static PSliderSkin LoadSliderSkin(const PPath& directory, PApplication* application);
    static PSliderOrientationSkin LoadSliderOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PSliderKnobAppearance LoadSliderKnobAppearance(
        const pugi::xml_node& node,
        const PPath& directory,
        PApplication* application,
        const PPoint& anchor);
    static PScrollBarSkin LoadScrollBarSkin(const PPath& directory, PApplication* application);
    static PScrollBarOrientationSkin LoadScrollBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PScrollBarSkinPart LoadScrollBarPart(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PTabViewSkin LoadTabViewSkin(const PPath& directory, PApplication* application);
    static PDropdownMenuSkin LoadDropdownMenuSkin(const PPath& directory, PApplication* application);
    static PGroupViewSkin LoadGroupViewSkin(const PPath& directory, PApplication* application);
    static PProgressBarSkin LoadProgressBarSkin(const PPath& directory, PApplication* application);
    static PProgressBarOrientationSkin LoadProgressBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static Ptr<const PImage> LoadSkinImage(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static void LoadIndicatorSkin(const pugi::xml_node& root, const PPath& directory, PApplication* application, PButtonIndicatorSkin& skin);
    static Ptr<PBitmap> LoadSkinBitmap(const pugi::xml_node& node, const PPath& directory, PApplication* application);
    static PButtonSkinAppearance LoadAppearance(
        const pugi::xml_node& node,
        const PPath& directory,
        PApplication* application,
        const PPoint& imageSize = PPoint(1.0f),
        PImageLayoutMode imageLayout = PImageLayoutMode::ViewRelative);

    Ptr<const PBrush> m_BackgroundBrush;
};
