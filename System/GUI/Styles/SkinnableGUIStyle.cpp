// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <string_view>
#include <App/Application.h>
#include <GUI/BitmapLoader.h>
#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Style/DropdownMenuStyleData.h>
#include <GUI/Style/DropdownMenuPopupStyleData.h>
#include <GUI/Style/GroupViewStyleData.h>
#include <GUI/Style/ProgressBarStyleData.h>
#include <GUI/Style/SliderStyleData.h>
#include <GUI/Style/SliderStyleLayout.h>
#include <GUI/Styles/SkinnableGUIStyle.h>
#include <GUI/View.h>
#include <GUI/Widgets/TextBox.h>
#include <GUI/ViewFactory.h>
#include <Math/Misc.h>
#include <Storage/Path.h>
#include <System/ExceptionHandling.h>
#include <Utils/XMLObjectParser.h>

struct PSkinnableGUIStyle::SkinnedSliderStyleData : PtrTarget
{
    PSliderStyleLayout Layout;
    const PSliderKnobSkin* Knob = nullptr;
    Ptr<const PImage> TrackSource;
    Ptr<PImage> BeforeImage;
    Ptr<PImage> AfterImage;
    Ptr<PBrush> BeforeBrush;
    Ptr<PBrush> AfterBrush;
    PPoint TrackSize;
    PColor BeforeColor;
    PColor AfterColor;
    const PSliderSkinColors* Appearance = nullptr;
};

struct PSkinnableGUIStyle::SkinnedScrollBarPart
{
    std::array<Ptr<PImage>, std::to_underlying(PButtonSkinState::Count)> Images;
    std::array<Ptr<PBrush>, std::to_underlying(PButtonSkinState::Count)> Brushes;
};

struct PSkinnableGUIStyle::SkinnedScrollBarStyleData : PtrTarget
{
    SkinnedScrollBarPart Track;
    SkinnedScrollBarPart Knob;
    std::array<SkinnedScrollBarPart, 4> Arrows;
    PScrollBarPart PressedPart = PScrollBarPart::None;
    PScrollBarPart HoveredPart = PScrollBarPart::None;
    bool HasVisualState = false;
};

struct PSkinnableGUIStyle::SkinnedScrollBarGeometry
{
    PRect Frame;
    PRect TrackFrame;
    PRect KnobArea;
    PRect KnobFrame;
    std::array<PRect, 4> ArrowFrames;
};

struct PSkinnableGUIStyle::SkinnedImage
{
    Ptr<PImage> Image;
    Ptr<PBrush> Brush;
};

struct PSkinnableGUIStyle::SkinnedTabViewTab
{
    std::array<SkinnedImage, std::to_underlying(PTabSkinState::Count) * 2> Appearances;
    float Left = 0.0f;
    float Width = 0.0f;
    float LabelWidth = 0.0f;
};

struct PSkinnableGUIStyle::SkinnedTabViewStyleData : PtrTarget
{
    std::vector<SkinnedTabViewTab> Tabs;
    SkinnedImage Frame;
    PFontHeight FontHeight;
    PRect PreviousBounds;
    PRect PreviousTabsFrame;
    size_t PreviousSelection = INVALID_INDEX;
    size_t PreviousHighlight = INVALID_INDEX;
    size_t PreviousPressed = INVALID_INDEX;
    float TabHeight = 0.0f;
    float TotalWidth = 0.0f;
    bool MetricsValid = false;
    bool HasVisualState = false;
};

struct PSkinnableGUIStyle::SkinnedTabViewGeometry
{
    PTabViewFrames Frames;
    PRect Bounds;
    PRect Panel;
    PRect OverflowLeft;
    PRect OverflowRight;
    float HeaderHeight = 0.0f;
    bool Overflow = false;
};

struct PSkinnableGUIStyle::SkinnedDropdownMenuStyleData : PtrTarget
{
    float ItemWidth = 0.0f;
    bool MetricsValid = false;
};

struct PSkinnableGUIStyle::SkinnedDropdownMenuPopupStyleData : PtrTarget
{
    std::array<SkinnedImage, std::to_underlying(PDropdownMenuRowState::Count)> Rows;
    PFontHeight FontHeight;
    PPoint ContentSize;
    size_t PreviousSelection = INVALID_INDEX;
    size_t PreviousPressed = INVALID_INDEX;
    float ItemWidth = 0.0f;
    float ItemHeight = 0.0f;
    bool MetricsValid = false;
    bool HasVisualState = false;
};

struct PSkinnableGUIStyle::SkinnedGroupViewStyleData : PtrTarget
{
    SkinnedImage Frame;
    PPoint LabelSize;
    PPoint MinimumSize;
    float LabelAscender = 0.0f;
    float HeaderHeight = 0.0f;
    bool MetricsValid = false;
};

struct PSkinnableGUIStyle::SkinnedProgressBarStyleData : PtrTarget
{
    std::array<SkinnedImage, 2> Tracks;
    std::array<SkinnedImage, 2> Fills;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename ValueType>
static ValueType ParseSkinAttribute(const pugi::xml_node& node, const char* name)
{
    const pugi::xml_attribute attribute = node.attribute(name);
    ValueType value;
    if (!attribute || !p_xml_object_parser::parse(attribute.value(), value)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Invalid skin attribute '{}' on '{}'", name, node.name()));
    }
    return value;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkin::PButtonSkin()
{
    for (size_t i = 0; i < States.size(); ++i)
    {
        const PStandardColorID color = (i == std::to_underlying(PButtonSkinState::Disabled))
            ? PStandardColorID::ButtonLabelDisabled : PStandardColorID::ButtonLabelNormal;
        States[i].LabelColor = pget_standard_color(color);
        CheckedStates[i].LabelColor = States[i].LabelColor;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSliderKnobAppearance::GetBounds() const
{
    if (Bitmap != nullptr) {
        return PRect::FromSize(PPoint(Bitmap->GetBounds().Size())) - Anchor.GetRounded();
    } else {
        return PRect();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSliderKnobSkin::GetBounds() const
{
    PRect bounds;
    for (const PSliderKnobAppearance& appearance : States)
    {
        const PRect frame = appearance.GetBounds();
        if (frame.IsValid()) {
            bounds = bounds.IsValid() ? bounds | frame : frame;
        }
    }
    const PRect shadowBounds = Shadow.GetBounds();
    if (shadowBounds.IsValid()) {
        bounds = bounds.IsValid() ? bounds | shadowBounds : shadowBounds;
    }
    return bounds;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle() : PSkinnableGUIStyle(PButtonSkin())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(const PButtonSkin& buttonSkin) : PSkinnableGUIStyle(buttonSkin, PCheckBoxSkin())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(const PButtonSkin& buttonSkin, const PCheckBoxSkin& checkBoxSkin)
    : PSkinnableGUIStyle(buttonSkin, checkBoxSkin, PRadioButtonSkin())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(const PButtonSkin& buttonSkin, const PCheckBoxSkin& checkBoxSkin, const PRadioButtonSkin& radioButtonSkin)
    : PSkinnableGUIStyle(buttonSkin, checkBoxSkin, radioButtonSkin, PSliderSkin())
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(
    const PButtonSkin& buttonSkin,
    const PCheckBoxSkin& checkBoxSkin,
    const PRadioButtonSkin& radioButtonSkin,
    const PSliderSkin& sliderSkin,
    PApplication* application)
    : PSkinnableGUIStyle(buttonSkin, checkBoxSkin, radioButtonSkin, sliderSkin, PScrollBarSkin(), application)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(
    const PButtonSkin& buttonSkin,
    const PCheckBoxSkin& checkBoxSkin,
    const PRadioButtonSkin& radioButtonSkin,
    const PSliderSkin& sliderSkin,
    const PScrollBarSkin& scrollBarSkin,
    PApplication* application)
    : PSkinnableGUIStyle(buttonSkin, checkBoxSkin, radioButtonSkin, sliderSkin, scrollBarSkin, PTabViewSkin(), application)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(
    const PButtonSkin& buttonSkin,
    const PCheckBoxSkin& checkBoxSkin,
    const PRadioButtonSkin& radioButtonSkin,
    const PSliderSkin& sliderSkin,
    const PScrollBarSkin& scrollBarSkin,
    const PTabViewSkin& tabViewSkin,
    PApplication* application)
    : PSkinnableGUIStyle(buttonSkin, checkBoxSkin, radioButtonSkin, sliderSkin, scrollBarSkin, tabViewSkin, PDropdownMenuSkin(), application)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(
    const PButtonSkin& buttonSkin,
    const PCheckBoxSkin& checkBoxSkin,
    const PRadioButtonSkin& radioButtonSkin,
    const PSliderSkin& sliderSkin,
    const PScrollBarSkin& scrollBarSkin,
    const PTabViewSkin& tabViewSkin,
    const PDropdownMenuSkin& dropdownMenuSkin,
    PApplication* application)
    : PSkinnableGUIStyle(buttonSkin, checkBoxSkin, radioButtonSkin, sliderSkin, scrollBarSkin, tabViewSkin, dropdownMenuSkin, PGroupViewSkin(), application)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(
    const PButtonSkin& buttonSkin,
    const PCheckBoxSkin& checkBoxSkin,
    const PRadioButtonSkin& radioButtonSkin,
    const PSliderSkin& sliderSkin,
    const PScrollBarSkin& scrollBarSkin,
    const PTabViewSkin& tabViewSkin,
    const PDropdownMenuSkin& dropdownMenuSkin,
    const PGroupViewSkin& groupViewSkin,
    PApplication* application)
    : PSkinnableGUIStyle(
        buttonSkin,
        checkBoxSkin,
        radioButtonSkin,
        sliderSkin,
        scrollBarSkin,
        tabViewSkin,
        dropdownMenuSkin,
        groupViewSkin,
        PProgressBarSkin(),
        application)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(
    const PButtonSkin& buttonSkin,
    const PCheckBoxSkin& checkBoxSkin,
    const PRadioButtonSkin& radioButtonSkin,
    const PSliderSkin& sliderSkin,
    const PScrollBarSkin& scrollBarSkin,
    const PTabViewSkin& tabViewSkin,
    const PDropdownMenuSkin& dropdownMenuSkin,
    const PGroupViewSkin& groupViewSkin,
    const PProgressBarSkin& progressBarSkin,
    PApplication* application)
    : m_ButtonSkin(buttonSkin)
    , m_CheckBoxSkin(checkBoxSkin)
    , m_RadioButtonSkin(radioButtonSkin)
    , m_SliderSkin(sliderSkin)
    , m_ScrollBarSkin(scrollBarSkin)
    , m_TabViewSkin(tabViewSkin)
    , m_DropdownMenuSkin(dropdownMenuSkin)
    , m_GroupViewSkin(groupViewSkin)
    , m_ProgressBarSkin(progressBarSkin)
{
    if (buttonSkin.LabelPadding.left < 0.0f || buttonSkin.LabelPadding.top < 0.0f
        || buttonSkin.LabelPadding.right < 0.0f || buttonSkin.LabelPadding.bottom < 0.0f) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    ValidateIndicatorSkin(checkBoxSkin);
    ValidateIndicatorSkin(radioButtonSkin);
    ValidateSliderSkin(sliderSkin);
    ValidateScrollBarSkin(scrollBarSkin);
    ValidateTabViewSkin(tabViewSkin);
    ValidateDropdownMenuSkin(dropdownMenuSkin);
    ValidateGroupViewSkin(groupViewSkin);
    ValidateProgressBarSkin(progressBarSkin);
    if (!PMath::is_finite(radioButtonSkin.LabelVerticalSpacing)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
    if (application == nullptr) {
        application = PApplication::GetCurrentApplication();
    }
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    m_BackgroundBrush = application->GetStandardBrush(PStandardColorID::DefaultBackground);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::PSkinnableGUIStyle(const PPath& directory, PApplication* application)
    : PSkinnableGUIStyle(
        LoadButtonSkin(directory, application),
        LoadCheckBoxSkin(directory, application),
        LoadRadioButtonSkin(directory, application),
        LoadSliderSkin(directory, application),
        LoadScrollBarSkin(directory, application),
        LoadTabViewSkin(directory, application),
        LoadDropdownMenuSkin(directory, application),
        LoadGroupViewSkin(directory, application),
        LoadProgressBarSkin(directory, application),
        application)
{
    m_BackgroundBrush = LoadBackgroundBrush(directory, application);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PSkinnableGUIStyle::GetBackgroundBrush() const
{
    return m_BackgroundBrush;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ButtonPaint(PView& target, const PRect& bounds, bool isEnabled, PButtonStyleData& data) const
{
    const Ptr<const PBrush> normal = m_ButtonSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background;
    if (normal == nullptr)
    {
        PDefaultGUIStyle::ButtonPaint(target, bounds, isEnabled, data);
        return;
    }
    data.UpdateMetrics(target);
    const PButtonSkinState state = GetButtonState(isEnabled, data);
    const PButtonSkinAppearance& appearance = data.IsChecked
        ? m_ButtonSkin.CheckedStates[std::to_underlying(state)] : m_ButtonSkin.States[std::to_underlying(state)];
    target.SetBackgroundBrush((appearance.Background != nullptr) ? appearance.Background : normal);
    target.SetBackgroundBrushOrigin(bounds.TopLeft());
    target.SetBrush(nullptr);

    const PRect& padding = m_ButtonSkin.LabelPadding;
    const PRect labelFrame = bounds.GetResized(padding.left, padding.top, -padding.right, -padding.bottom);
    PPoint position(
        std::round(labelFrame.left + (labelFrame.Width() - data.LabelSize.x) * 0.5f),
        std::round(labelFrame.top + (labelFrame.Height() - data.LabelSize.y) * 0.5f));
    if (data.LabelAlignment == PAlignment::Left) {
        position.x = labelFrame.left;
    } else if (data.LabelAlignment == PAlignment::Right) {
        position.x = labelFrame.right - data.LabelSize.x;
    }
    if (isEnabled && data.IsPressed) {
        position += m_ButtonSkin.PressedLabelOffset;
    }
    position.y += target.GetFontHeight().ascender;
    target.SetFgColor(appearance.LabelColor);
    const PDrawingMode previousMode = target.GetDrawingMode();
    target.SetDrawingMode(PDrawingMode::Copy);
    target.DrawString(data.Label, position, bounds);
    target.SetDrawingMode(previousMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_ButtonSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background == nullptr)
    {
        PDefaultGUIStyle::ButtonGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    data.UpdateMetrics(target);
    const PRect& padding = m_ButtonSkin.LabelPadding;
    PPoint size = GetButtonImageMinimumSize();
    size.x = std::max(size.x, data.LabelSize.x + padding.left + padding.right);
    size.y = std::max(size.y, data.LabelSize.y + padding.top + padding.bottom);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::CheckBoxPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const
{
    if (m_CheckBoxSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background == nullptr)
    {
        PDefaultGUIStyle::CheckBoxPaint(target, frame, isEnabled, data);
        return;
    }
    const PFontHeight fontHeight = target.GetFontHeight();
    const float labelTop = std::round(frame.top + (frame.Height() - fontHeight.ascender - fontHeight.descender) * 0.5f);
    const float labelOffset = m_CheckBoxSkin.Size + m_CheckBoxSkin.LabelSpacing;
    const float contentLeft = frame.left - (data.Label.empty() ? 0.0f : std::min(0.0f, labelOffset));
    const PPoint labelPosition(contentLeft + labelOffset, labelTop + fontHeight.ascender + m_CheckBoxSkin.LabelVerticalOffset);
    const PPoint indicatorPosition(contentLeft, std::round(frame.top + (frame.Height() - m_CheckBoxSkin.Size) * 0.5f));
    const bool overlaps = !data.Label.empty() && m_CheckBoxSkin.LabelSpacing < 0.0f;
    PaintIndicatorButton(target, frame, isEnabled, data, m_CheckBoxSkin, indicatorPosition, labelPosition, overlaps);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::CheckBoxGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_CheckBoxSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background == nullptr)
    {
        PDefaultGUIStyle::CheckBoxGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    PPoint size(m_CheckBoxSkin.Size);
    if (!data.Label.empty())
    {
        data.UpdateMetrics(target);
        const PFontHeight fontHeight = target.GetFontHeight();
        size.x = GetSpacedExtent(m_CheckBoxSkin.Size, data.LabelSize.x, m_CheckBoxSkin.LabelSpacing);
        size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender + 2.0f * std::abs(m_CheckBoxSkin.LabelVerticalOffset));
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::RadioButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const
{
    if (m_RadioButtonSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background == nullptr)
    {
        PDefaultGUIStyle::RadioButtonPaint(target, frame, isEnabled, data);
        return;
    }
    const PRadioButtonSkin& skin = m_RadioButtonSkin;
    const PFontHeight fontHeight = target.GetFontHeight();
    const float labelHeight = fontHeight.ascender + fontHeight.descender;
    const float labelTop = std::round(frame.top + (frame.Height() - labelHeight) * 0.5f);
    PPoint labelPosition(frame.left + skin.Size + skin.LabelSpacing, labelTop + fontHeight.ascender + skin.LabelVerticalOffset);
    PPoint indicatorPosition(frame.left, std::round(frame.top + (frame.Height() - skin.Size) * 0.5f));
    bool overlaps = false;
    if (data.Label.empty())
    {
        indicatorPosition.x = std::round(frame.left + (frame.Width() - skin.Size) * 0.5f);
    }
    else
    {
        data.UpdateMetrics(target);
        if (data.LabelAlignment == PAlignment::Top || data.LabelAlignment == PAlignment::Bottom)
        {
            const float labelPadding = std::abs(skin.LabelVerticalOffset);
            const float labelExtent = labelHeight + 2.0f * labelPadding;
            const bool labelFirst = data.LabelAlignment == PAlignment::Top;
            const float secondOffset = (labelFirst ? labelExtent : skin.Size) + skin.LabelVerticalSpacing;
            const float contentHeight = GetSpacedExtent(skin.Size, labelExtent, skin.LabelVerticalSpacing);
            const float contentTop = std::round(frame.top + (frame.Height() - contentHeight) * 0.5f) - std::min(0.0f, secondOffset);
            overlaps = skin.LabelVerticalSpacing < 0.0f;
            indicatorPosition.x = std::round(frame.left + (frame.Width() - skin.Size) * 0.5f);
            labelPosition.x = std::round(frame.left + (frame.Width() - data.LabelSize.x) * 0.5f);
            if (labelFirst)
            {
                labelPosition.y = contentTop + labelPadding + fontHeight.ascender + skin.LabelVerticalOffset;
                indicatorPosition.y = contentTop + secondOffset;
            }
            else
            {
                indicatorPosition.y = contentTop;
                labelPosition.y = contentTop + secondOffset + labelPadding + fontHeight.ascender + skin.LabelVerticalOffset;
            }
        }
        else
        {
            const bool labelFirst = data.LabelAlignment == PAlignment::Left;
            const float secondOffset = (labelFirst ? data.LabelSize.x : skin.Size) + skin.LabelSpacing;
            const float contentLeft = frame.left - std::min(0.0f, secondOffset);
            indicatorPosition.x = contentLeft + (labelFirst ? secondOffset : 0.0f);
            labelPosition.x = contentLeft + (labelFirst ? 0.0f : secondOffset);
            overlaps = skin.LabelSpacing < 0.0f;
        }
    }
    PaintIndicatorButton(target, frame, isEnabled, data, skin, indicatorPosition, labelPosition, overlaps);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::RadioButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_RadioButtonSkin.States[std::to_underlying(PButtonSkinState::Normal)].Background == nullptr)
    {
        PDefaultGUIStyle::RadioButtonGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    const PRadioButtonSkin& skin = m_RadioButtonSkin;
    PPoint size(skin.Size);
    if (!data.Label.empty())
    {
        data.UpdateMetrics(target);
        const PFontHeight fontHeight = target.GetFontHeight();
        const float labelHeight = fontHeight.ascender + fontHeight.descender + 2.0f * std::abs(skin.LabelVerticalOffset);
        if (data.LabelAlignment == PAlignment::Top || data.LabelAlignment == PAlignment::Bottom)
        {
            size.x = std::max(size.x, data.LabelSize.x);
            size.y = GetSpacedExtent(skin.Size, labelHeight, skin.LabelVerticalSpacing);
        }
        else
        {
            size.x = GetSpacedExtent(skin.Size, data.LabelSize.x, skin.LabelSpacing);
            size.y = std::max(size.y, labelHeight);
        }
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::SliderPaint(PView& target, const PRect& frame, const PSliderStyleData& data) const
{
    if (!HasSliderSkin())
    {
        PDefaultGUIStyle::SliderPaint(target, frame, data);
        return;
    }
    SkinnedSliderStyleData& state = GetSliderStyleData(data);
    PSliderStyleLayout& layout = state.Layout;
    const PSliderSkinColors& colors = m_SliderSkin.Colors[std::to_underlying(GetSliderState(data))];
    const PSliderOrientationSkin& skin = (data.Orientation == POrientation::Horizontal) ? m_SliderSkin.Horizontal : m_SliderSkin.Vertical;
    const PRect trackFrame = layout.GetTrackFrame(target, frame.Bounds(), data) + frame.TopLeft();
    const PPoint knobPosition = PSliderStyleLayout::GetPosition(trackFrame, data.Orientation, data.Value);
    const PSliderKnobSkin& knob = *state.Knob;
    const PSliderKnobAppearance& normal = knob.States[std::to_underlying(PButtonSkinState::Normal)];
    const PSliderKnobAppearance& selected = knob.States[std::to_underlying(GetSliderState(data))];
    const PSliderKnobAppearance& appearance = (selected.Bitmap != nullptr) ? selected : normal;
    const PRect knobFrame = appearance.GetBounds() + knobPosition;
    PRect beforeFrame = trackFrame;
    PRect afterFrame = trackFrame;
    if (trackFrame.IsValid())
    {
        if (data.Orientation == POrientation::Horizontal)
        {
            beforeFrame.right = std::clamp(knobPosition.x, trackFrame.left, trackFrame.right);
            afterFrame.left = beforeFrame.right;
        }
        else
        {
            beforeFrame.bottom = std::clamp(knobPosition.y, trackFrame.top, trackFrame.bottom);
            afterFrame.top = beforeFrame.bottom;
        }
    }
    target.SetBackgroundBrush(nullptr);
    target.SetBrush(nullptr);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    layout.ClearBackground(target, frame, trackFrame, knobFrame, data);
    PSliderStyleLayout::DrawTicks(target, trackFrame, data, colors.Tick, colors.TickHighlight);
    layout.DrawLabels(target, frame, trackFrame, data, colors.Label);
    if (trackFrame.IsValid())
    {
        UpdateSliderTrackImages(state, skin, colors, data, trackFrame.Size());
        target.SetBrushOrigin(trackFrame.TopLeft());
        if (beforeFrame.IsValid()) {
            target.FillRect(beforeFrame, state.BeforeBrush);
        }
        if (afterFrame.IsValid()) {
            target.FillRect(afterFrame, state.AfterBrush);
        }
    }
    const PSliderKnobAppearance& shadow = (knob.Shadow.Bitmap != nullptr) ? knob.Shadow : normal;
    const PRect shadowBounds = shadow.GetBounds();
    for (float value : data.ShadowValues)
    {
        const PRect shadowFrame = shadowBounds + PSliderStyleLayout::GetPosition(trackFrame, data.Orientation, value);
        DrawSkinSliderKnob(target, shadowFrame, beforeFrame, afterFrame, state, shadow.Bitmap);
    }
    DrawSkinSliderKnob(target, knobFrame, beforeFrame, afterFrame, state, appearance.Bitmap);
    const PRect labelFrame = layout.GetValueLabelFrame(target, data) + frame.TopLeft();
    if (labelFrame.IsValid())
    {
        target.SetFgColor(colors.Label);
        target.DrawString(data.ValueLabel, labelFrame.TopLeft() + PPoint(0.0f, target.GetFontHeight().ascender), labelFrame);
    }
    if (!layout.HasVisualState()) {
        layout.UpdateDamage(target, data, PSliderProperties(), false);
    }
    state.Appearance = &colors;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::SliderGetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (HasSliderSkin()) {
        GetSliderStyleData(data).Layout.GetPreferredSize(target, data, minSize, maxSize);
    } else {
        PDefaultGUIStyle::SliderGetPreferredSize(target, data, minSize, maxSize);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::SliderPropertyChanged(PView& target, PSliderStyleData& data, PSliderProperties properties) const
{
    if (HasSliderSkin())
    {
        SkinnedSliderStyleData& state = GetSliderStyleData(data);
        const PSliderSkinColors& colors = m_SliderSkin.Colors[std::to_underlying(GetSliderState(data))];
        if (state.Appearance != nullptr && *state.Appearance != colors) {
            target.Invalidate();
        }
        state.Appearance = &colors;
        state.Layout.PropertyChanged(target, data, properties);
    }
    else
    {
        PDefaultGUIStyle::SliderPropertyChanged(target, data, properties);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSkinnableGUIStyle::SliderGetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue) const
{
    if (HasSliderSkin()) {
        return GetSliderStyleData(data).Layout.GetValuePosition(target, data, relativeValue);
    } else {
        return PDefaultGUIStyle::SliderGetValuePosition(target, data, relativeValue);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSkinnableGUIStyle::SliderGetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position) const
{
    if (HasSliderSkin()) {
        return GetSliderStyleData(data).Layout.GetValueFromPosition(target, data, position);
    } else {
        return PDefaultGUIStyle::SliderGetValueFromPosition(target, data, position);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ScrollBarPaint(PView& target, const PRect& frame, const PScrollBarStyleData& data) const
{
    if (!HasScrollBarSkin())
    {
        PDefaultGUIStyle::ScrollBarPaint(target, frame, data);
        return;
    }
    if (!frame.IsValid()) {
        return;
    }
    const PScrollBarOrientationSkin& skin = GetScrollBarOrientation(data.Orientation);
    const SkinnedScrollBarGeometry geometry = GetScrollBarGeometry(frame, data, data.Value);
    SkinnedScrollBarStyleData& state = data.GetOrCreateStyleData<SkinnedScrollBarStyleData>();
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreDrawing([&target, previousMode] { target.SetBrush(nullptr); target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(GetScrollBarBrush(state.Track, skin.Track, GetScrollBarState(data, PScrollBarPart::None), frame.Size()));
    target.SetBackgroundBrushOrigin(frame.TopLeft());
    ClearScrollBarTrack(target, geometry);
    PaintScrollBarPart(target, geometry.KnobFrame, skin.Knob, state.Knob, GetScrollBarState(data, PScrollBarPart::Knob));
    for (size_t index = 0; index < geometry.ArrowFrames.size(); ++index)
    {
        const PScrollBarSkinPart& part = (index % 2 == 0) ? skin.Decrement : skin.Increment;
        PaintScrollBarPart(target, geometry.ArrowFrames[index], part, state.Arrows[index], GetScrollBarState(data, SCROLL_BAR_ARROW_PARTS[index]));
    }
    state.PressedPart = data.PressedPart;
    state.HoveredPart = data.HoveredPart;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ScrollBarGetPreferredSize(const PView& target, const PScrollBarStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (!HasScrollBarSkin())
    {
        PDefaultGUIStyle::ScrollBarGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    const PScrollBarOrientationSkin& skin = GetScrollBarOrientation(data.Orientation);
    const bool horizontal = data.Orientation == POrientation::Horizontal;
    const float insets = horizontal ? skin.KnobInsets.left + skin.KnobInsets.right : skin.KnobInsets.top + skin.KnobInsets.bottom;
    const float minimumLength = 2.0f * skin.ArrowLength + skin.MinimumKnobLength + insets;
    *minSize = horizontal ? PPoint(minimumLength, skin.Thickness) : PPoint(skin.Thickness, minimumLength);
    *maxSize = horizontal ? PPoint(COORD_MAX, skin.Thickness) : PPoint(skin.Thickness, COORD_MAX);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ScrollBarPropertyChanged(PView& target, PScrollBarStyleData& data, PScrollBarProperties properties) const
{
    if (!HasScrollBarSkin())
    {
        PDefaultGUIStyle::ScrollBarPropertyChanged(target, data, properties);
        return;
    }
    SkinnedScrollBarStyleData& state = data.GetOrCreateStyleData<SkinnedScrollBarStyleData>();
    const PScrollBarProperties trackProperties(PScrollBarProperty::Value, PScrollBarProperty::Proportion);
    const PScrollBarProperties incremental = trackProperties | PScrollBarProperty::Pressed | PScrollBarProperty::Hovered;
    if (!state.HasVisualState || !(properties & ~incremental).Empty())
    {
        target.Invalidate();
    }
    else
    {
        const SkinnedScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
        if (properties.HasAny(trackProperties) && geometry.TrackFrame.IsValid()) {
            target.Invalidate(geometry.TrackFrame);
        }
        const auto invalidatePart = [&target, &geometry](PScrollBarPart part)
        {
            const PRect partFrame = GetScrollBarPartFrame(geometry, part);
            if (partFrame.IsValid()) {
                target.Invalidate(partFrame);
            }
        };
        if (properties.Has(PScrollBarProperty::Pressed))
        {
            invalidatePart(state.PressedPart);
            invalidatePart(data.PressedPart);
        }
        if (properties.Has(PScrollBarProperty::Hovered))
        {
            invalidatePart(state.HoveredPart);
            invalidatePart(data.HoveredPart);
        }
    }
    state.PressedPart = data.PressedPart;
    state.HoveredPart = data.HoveredPart;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarPart PSkinnableGUIStyle::ScrollBarHitTest(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const
{
    if (!HasScrollBarSkin()) {
        return PDefaultGUIStyle::ScrollBarHitTest(target, data, position);
    }
    if (!data.IsEnabled || !data.HasRange) {
        return PScrollBarPart::None;
    }
    const SkinnedScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
    for (size_t index = 0; index < geometry.ArrowFrames.size(); ++index)
    {
        if (geometry.ArrowFrames[index].DoIntersect(position)) {
            return SCROLL_BAR_ARROW_PARTS[index];
        }
    }
    if (geometry.KnobFrame.DoIntersect(position)) {
        return PScrollBarPart::Knob;
    }
    if (geometry.KnobArea.DoIntersect(position) && geometry.KnobFrame.IsValid())
    {
        const bool before = (data.Orientation == POrientation::Horizontal) ? position.x < geometry.KnobFrame.left : position.y < geometry.KnobFrame.top;
        return before ? PScrollBarPart::PageDecrement : PScrollBarPart::PageIncrement;
    }
    return PScrollBarPart::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSkinnableGUIStyle::ScrollBarGetValuePosition(const PView& target, const PScrollBarStyleData& data, float relativeValue) const
{
    if (!HasScrollBarSkin()) {
        return PDefaultGUIStyle::ScrollBarGetValuePosition(target, data, relativeValue);
    }
    const SkinnedScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, relativeValue);
    return geometry.KnobFrame.IsValid() ? geometry.KnobFrame.Center() : target.GetNormalizedBounds().Center();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSkinnableGUIStyle::ScrollBarGetValueFromPosition(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const
{
    if (!HasScrollBarSkin()) {
        return PDefaultGUIStyle::ScrollBarGetValueFromPosition(target, data, position);
    }
    const SkinnedScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
    if (!geometry.KnobFrame.IsValid()) {
        return data.Value;
    }
    const bool horizontal = data.Orientation == POrientation::Horizontal;
    const float start = horizontal ? geometry.KnobArea.left : geometry.KnobArea.top;
    const float length = horizontal ? geometry.KnobArea.Width() : geometry.KnobArea.Height();
    const float knobLength = horizontal ? geometry.KnobFrame.Width() : geometry.KnobFrame.Height();
    const float travel = length - knobLength;
    const float offset = (horizontal ? position.x : position.y) - start - knobLength * 0.5f;
    return (travel > 0.0f) ? std::clamp(offset / travel, 0.0f, 1.0f) : data.Value;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuPaint(PView& target, const PRect& frame, const PDropdownMenuStyleData& data) const
{
    if (m_DropdownMenuSkin.States[0].Background == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuPaint(target, frame, data);
        return;
    }
    const PDropdownMenuAppearance& appearance = m_DropdownMenuSkin.States[std::to_underlying(GetDropdownMenuState(data))];
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetBackgroundBrush(appearance.Background);
    target.SetBackgroundBrushOrigin(frame.TopLeft());
    target.SetBrush(nullptr);
    target.SetDrawingMode(PDrawingMode::Copy);
    target.EraseRect(frame);
    const PRect arrowFrame = GetDropdownMenuArrowFrame(frame);
    const PRect source(appearance.Arrow->GetBounds());
    const PPoint position = (arrowFrame.TopLeft() + (arrowFrame.Size() - source.Size()) * 0.5f + appearance.ArrowOffset).GetRounded();
    const PRect visible = PRect(position, position + source.Size()) & arrowFrame;
    if (visible.IsValid())
    {
        target.SetDrawingMode(PDrawingMode::Overlay);
        target.DrawBitmap(appearance.Arrow, visible - position + source.TopLeft(), visible.TopLeft());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuGetPreferredSize(const PView& target, const PDropdownMenuStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_DropdownMenuSkin.States[0].Background == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    const SkinnedDropdownMenuStyleData& state = GetDropdownMenuStyleData(target, data);
    const float textWidth = (data.Items.empty() && data.Text != nullptr) ? target.GetStringWidth(*data.Text) : state.ItemWidth;
    const PRect& insets = m_DropdownMenuSkin.EditorInsets;
    *minSize = PPoint(
        std::max(m_DropdownMenuSkin.MinimumSize.x, data.EditorSize.x + textWidth + insets.left + insets.right + m_DropdownMenuSkin.ArrowWidth),
        std::max(m_DropdownMenuSkin.MinimumSize.y, data.EditorSize.y + insets.top + insets.bottom));
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuPropertyChanged(PView& target, PDropdownMenuStyleData& data, PDropdownMenuProperties properties) const
{
    if (m_DropdownMenuSkin.States[0].Background == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuPropertyChanged(target, data, properties);
        return;
    }
    SkinnedDropdownMenuStyleData& state = data.GetOrCreateStyleData<SkinnedDropdownMenuStyleData>();
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Font))) {
        state.MetricsValid = false;
    }
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Text, PDropdownMenuProperty::Font))) {
        target.PreferredSizeChanged();
    }
    target.Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::DropdownMenuGetEditorFrame(const PView& target, const PDropdownMenuStyleData& data) const
{
    if (m_DropdownMenuSkin.States[0].Background == nullptr) {
        return PDefaultGUIStyle::DropdownMenuGetEditorFrame(target, data);
    }
    PRect frame = target.GetBounds();
    frame.right = GetDropdownMenuArrowFrame(frame).left;
    return InsetSkinFrame(frame, m_DropdownMenuSkin.EditorInsets);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PTextBoxStyle> PSkinnableGUIStyle::DropdownMenuGetEditorStyle(const PDropdownMenuStyleData& data) const
{
    if (m_DropdownMenuSkin.States[0].Background == nullptr) {
        return PDefaultGUIStyle::DropdownMenuGetEditorStyle(data);
    } else {
        return m_DropdownMenuSkin.States[std::to_underlying(GetDropdownMenuState(data))].Editor;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PSkinnableGUIStyle::DropdownMenuHitTest(const PView& target, const PDropdownMenuStyleData& data, const PPoint& position) const
{
    if (m_DropdownMenuSkin.States[0].Background == nullptr) {
        return PDefaultGUIStyle::DropdownMenuHitTest(target, data, position);
    }
    const PRect frame = data.IsReadOnly ? target.GetBounds() : GetDropdownMenuArrowFrame(target.GetBounds());
    return data.IsEnabled && frame.DoIntersect(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuPopupFramePaint(PView& target, const PRect& frame) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuPopupFramePaint(target, frame);
        return;
    }
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_BackgroundBrush);
    target.SetBrush(nullptr);
    target.SetBrushOrigin(frame.TopLeft());
    target.FillRect(frame, m_DropdownMenuSkin.PopupFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuPopupFrameGetPreferredSize(const PView& target, const PPoint& contentSize, PPoint* minSize, PPoint* maxSize) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuPopupFrameGetPreferredSize(target, contentSize, minSize, maxSize);
        return;
    }
    const PRect& insets = m_DropdownMenuSkin.PopupInsets;
    *minSize = PPoint(std::max(0.0f, contentSize.x + insets.left + insets.right), std::max(0.0f, contentSize.y + insets.top + insets.bottom));
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::DropdownMenuPopupFrameGetContentFrame(const PView& target) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr) {
        return PDefaultGUIStyle::DropdownMenuPopupFrameGetContentFrame(target);
    } else {
        return InsetSkinFrame(target.GetBounds(), m_DropdownMenuSkin.PopupInsets);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuPopupPaint(PView& target, const PRect& updateRect, const PDropdownMenuPopupStyleData& data) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuPopupPaint(target, updateRect, data);
        return;
    }
    SkinnedDropdownMenuPopupStyleData& state = GetDropdownMenuPopupStyleData(target, data);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); target.SetBrush(nullptr); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_DropdownMenuSkin.PopupBackground);
    target.SetBrush(nullptr);
    const PRect damage = updateRect & target.GetBounds();
    target.EraseRect(damage);
    const float itemCount = float(data.Items.size());
    const size_t first = size_t(std::clamp(std::floor(damage.top / state.ItemHeight), 0.0f, itemCount));
    const size_t end = size_t(std::clamp(std::ceil(damage.bottom / state.ItemHeight), 0.0f, itemCount));
    for (size_t index = first; index < end; ++index) {
        PaintDropdownMenuRow(target, data, state, index);
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressed = data.PressedItem;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuPopupGetPreferredSize(const PView& target, const PDropdownMenuPopupStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuPopupGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    *minSize = GetDropdownMenuPopupStyleData(target, data).ContentSize;
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DropdownMenuPopupPropertyChanged(PView& target, PDropdownMenuPopupStyleData& data, PDropdownMenuPopupProperties properties) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr)
    {
        PDefaultGUIStyle::DropdownMenuPopupPropertyChanged(target, data, properties);
        return;
    }
    SkinnedDropdownMenuPopupStyleData& state = data.GetOrCreateStyleData<SkinnedDropdownMenuPopupStyleData>();
    if (properties.HasAny(PDropdownMenuPopupProperties(PDropdownMenuPopupProperty::Items, PDropdownMenuPopupProperty::Font)))
    {
        state.MetricsValid = false;
        target.ContentSizeChanged();
        target.PreferredSizeChanged();
    }
    const PDropdownMenuPopupProperties incremental(PDropdownMenuPopupProperty::Selection, PDropdownMenuPopupProperty::Pressed);
    if (!state.HasVisualState || !(properties & ~incremental).Empty())
    {
        target.Invalidate();
    }
    else
    {
        for (size_t index : {state.PreviousSelection, state.PreviousPressed, data.Selection, data.PressedItem})
        {
            if (index < data.Items.size()) {
                target.Invalidate(DropdownMenuPopupGetItemFrame(target, data, index));
            }
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressed = data.PressedItem;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::DropdownMenuPopupGetItemFrame(const PView& target, const PDropdownMenuPopupStyleData& data, size_t index) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr) {
        return PDefaultGUIStyle::DropdownMenuPopupGetItemFrame(target, data, index);
    }
    if (index >= data.Items.size()) {
        return PRect();
    }
    const float height = GetDropdownMenuPopupStyleData(target, data).ItemHeight;
    const PRect bounds = target.GetBounds();
    return PRect(bounds.left, float(index) * height, bounds.right, float(index + 1) * height);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PSkinnableGUIStyle::DropdownMenuPopupGetItemFromPosition(const PView& target, const PDropdownMenuPopupStyleData& data, const PPoint& position) const
{
    if (m_DropdownMenuSkin.PopupFrame == nullptr) {
        return PDefaultGUIStyle::DropdownMenuPopupGetItemFromPosition(target, data, position);
    }
    if (data.Items.empty()) {
        return INVALID_INDEX;
    }
    const float height = GetDropdownMenuPopupStyleData(target, data).ItemHeight;
    return size_t(std::clamp(std::floor(position.y / height), 0.0f, float(data.Items.size() - 1)));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::GroupViewPaint(PView& target, const PRect& updateRect, const PGroupViewStyleData& data) const
{
    if (m_GroupViewSkin.Frame == nullptr)
    {
        PDefaultGUIStyle::GroupViewPaint(target, updateRect, data);
        return;
    }
    SkinnedGroupViewStyleData& state = GetGroupViewStyleData(target, data);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBrush(nullptr);
    const PRect bounds = target.GetBounds();
    PRect frame = InsetSkinFrame(bounds, m_GroupViewSkin.FrameInsets);
    frame.top = std::min(frame.bottom, frame.top + std::floor(state.HeaderHeight * 0.5f));
    PaintGroupViewFrame(target, updateRect, frame, state);
    if (!data.Label.empty())
    {
        const PRect& padding = m_GroupViewSkin.LabelPadding;
        const PPoint position = (bounds.TopLeft() + PPoint(padding.left, padding.top + state.LabelAscender)).GetRounded();
        const PRect gap(
            position.x - m_GroupViewSkin.LabelGap,
            position.y - state.LabelAscender,
            position.x + state.LabelSize.x + m_GroupViewSkin.LabelGap,
            position.y - state.LabelAscender + state.LabelSize.y);
        const PRect damage = gap & bounds & updateRect;
        if (damage.IsValid()) {
            target.EraseRect(damage);
        }
        target.SetFgColor(m_GroupViewSkin.LabelColor);
        target.DrawString(data.Label, position, state.LabelSize.x);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::GroupViewGetPreferredSize(const PView& target, const PGroupViewStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_GroupViewSkin.Frame == nullptr)
    {
        PDefaultGUIStyle::GroupViewGetPreferredSize(target, data, minSize, maxSize);
    }
    else
    {
        *minSize = GetGroupViewStyleData(target, data).MinimumSize;
        *maxSize = PPoint(LAYOUT_MAX_SIZE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::GroupViewPropertyChanged(PView& target, PGroupViewStyleData& data, PGroupViewProperties properties) const
{
    if (m_GroupViewSkin.Frame == nullptr)
    {
        PDefaultGUIStyle::GroupViewPropertyChanged(target, data, properties);
        return;
    }
    if (properties.HasAny(PGroupViewProperties(PGroupViewProperty::Label, PGroupViewProperty::Font)))
    {
        data.GetOrCreateStyleData<SkinnedGroupViewStyleData>().MetricsValid = false;
        target.PreferredSizeChanged();
    }
    target.Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ProgressBarPaint(PView& target, const PRect& frame, const PProgressBarStyleData& data) const
{
    if (m_ProgressBarSkin.Horizontal.Track == nullptr)
    {
        PDefaultGUIStyle::ProgressBarPaint(target, frame, data);
        return;
    }
    const PRect bounds = frame.GetFloored();
    if (!bounds.IsValid()) {
        return;
    }
    const PProgressBarOrientationSkin& skin = GetProgressBarOrientation(data.Orientation);
    SkinnedProgressBarStyleData& state = data.GetOrCreateStyleData<SkinnedProgressBarStyleData>();
    const size_t orientationIndex = (data.Orientation == POrientation::Horizontal) ? 0 : 1;
    const Ptr<const PBrush> track = GetSkinBrush(state.Tracks[orientationIndex], *skin.Track, bounds.Size());
    const Ptr<const PBrush> previousBackground = target.GetBackgroundBrush();
    const PPoint previousBackgroundOrigin = target.GetBackgroundBrushOrigin();
    const PPoint previousDrawingOrigin = target.GetDrawingBrushOrigin();
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreState([&target, previousBackground, previousBackgroundOrigin, previousDrawingOrigin, previousMode]
    {
        target.SetBackgroundBrush(previousBackground);
        target.SetBackgroundBrushOrigin(previousBackgroundOrigin);
        target.SetBrushOrigin(previousDrawingOrigin);
        target.SetDrawingMode(previousMode);
    });
    // Resolve the fill's transparent edges against the track, then restore the
    // view's background so this skin does not outlive a later style change.
    target.SetBackgroundBrush(track);
    target.SetBackgroundBrushOrigin(bounds.TopLeft());
    target.SetDrawingMode(PDrawingMode::Copy);
    const PRect fillFrame = GetProgressBarFillFrame(bounds, skin, data);
    const PRect visibleFill = fillFrame & bounds;
    if (visibleFill.IsValid())
    {
        const std::array<PRect, 4> exposedTrack =
        {
            PRect(bounds.left, bounds.top, bounds.right, visibleFill.top),
            PRect(bounds.left, visibleFill.bottom, bounds.right, bounds.bottom),
            PRect(bounds.left, visibleFill.top, visibleFill.left, visibleFill.bottom),
            PRect(visibleFill.right, visibleFill.top, bounds.right, visibleFill.bottom)
        };
        for (const PRect& section : exposedTrack)
        {
            if (section.IsValid()) {
                target.EraseRect(section);
            }
        }
        const Ptr<const PBrush> fill = GetSkinBrush(state.Fills[orientationIndex], *skin.Fill, fillFrame.Size());
        target.SetBrushOrigin(fillFrame.TopLeft());
        target.FillRect(visibleFill, fill);
    }
    else
    {
        target.EraseRect(bounds);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ProgressBarGetPreferredSize(const PView& target, const PProgressBarStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (m_ProgressBarSkin.Horizontal.Track == nullptr)
    {
        PDefaultGUIStyle::ProgressBarGetPreferredSize(target, data, minSize, maxSize);
        return;
    }
    const PProgressBarOrientationSkin& skin = GetProgressBarOrientation(data.Orientation);
    const bool horizontal = data.Orientation == POrientation::Horizontal;
    *minSize = horizontal ? PPoint(skin.MinimumLength, skin.Thickness) : PPoint(skin.Thickness, skin.MinimumLength);
    *maxSize = horizontal ? PPoint(LAYOUT_MAX_SIZE, skin.Thickness) : PPoint(skin.Thickness, LAYOUT_MAX_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ProgressBarPropertyChanged(PView& target, PProgressBarStyleData& data, PProgressBarProperties properties) const
{
    if (m_ProgressBarSkin.Horizontal.Track == nullptr)
    {
        PDefaultGUIStyle::ProgressBarPropertyChanged(target, data, properties);
        return;
    }
    if (properties.Has(PProgressBarProperty::Font)) {
        target.PreferredSizeChanged();
    }
    if (properties == PProgressBarProperties(PProgressBarProperty::Value))
    {
        const PRect bounds = target.GetNormalizedBounds().GetFloored();
        const PRect damage = InsetSkinFrame(bounds, GetProgressBarOrientation(data.Orientation).FillInsets).GetFloored() & bounds;
        if (damage.IsValid()) {
            target.Invalidate(damage);
        }
    }
    else
    {
        target.Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::TabViewPaint(PView& target, const PView& tabsTarget, const PRect& updateRect, const PTabViewStyleData& data) const
{
    if (m_TabViewSkin.Frame == nullptr)
    {
        PDefaultGUIStyle::TabViewPaint(target, tabsTarget, updateRect, data);
        return;
    }
    SkinnedTabViewStyleData& state = GetTabViewStyleData(target, data);
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); target.SetBrush(nullptr); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_BackgroundBrush);
    target.SetBrush(nullptr);
    target.EraseRect(PRect(geometry.Bounds.left, geometry.Bounds.top, geometry.Bounds.right, geometry.Panel.top) & updateRect);
    if (geometry.Panel.IsValid())
    {
        const Ptr<const PBrush> brush = GetSkinBrush(state.Frame, *m_TabViewSkin.Frame, geometry.Panel.Size());
        target.SetBrushOrigin(geometry.Panel.TopLeft());
        target.FillRect(geometry.Panel & updateRect, brush);
    }
    if (geometry.Overflow)
    {
        if (tabsTarget.GetBounds().left > 0.0f) {
            PaintTabViewOverflow(target, geometry.OverflowLeft, m_TabViewSkin.OverflowLeft);
        }
        if (tabsTarget.GetBounds().right < state.TotalWidth) {
            PaintTabViewOverflow(target, geometry.OverflowRight, m_TabViewSkin.OverflowRight);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::TabViewTabsPaint(PView& target, const PView& tabView, const PRect& updateRect, const PTabViewStyleData& data) const
{
    if (m_TabViewSkin.Frame == nullptr)
    {
        PDefaultGUIStyle::TabViewTabsPaint(target, tabView, updateRect, data);
        return;
    }
    SkinnedTabViewStyleData& state = GetTabViewStyleData(tabView, data);
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(tabView, data);
    const PRect bounds = target.GetBounds();
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); target.SetBrush(nullptr); });
    target.SetDrawingMode(PDrawingMode::Copy);
    target.SetBackgroundBrush(m_BackgroundBrush);
    target.SetBrush(nullptr);
    target.EraseRect(updateRect);
    if (geometry.Panel.IsValid())
    {
        const Ptr<const PBrush> brush = GetSkinBrush(state.Frame, *m_TabViewSkin.Frame, geometry.Panel.Size());
        target.SetBrushOrigin(geometry.Panel.TopLeft() - geometry.Frames.TabsFrame.TopLeft() - target.GetScrollOffset());
        target.FillRect(PRect(bounds.left, geometry.HeaderHeight, bounds.right, bounds.bottom) & updateRect, brush);
    }
    for (size_t index = 0; index < state.Tabs.size(); ++index)
    {
        if (index != data.Selection && GetSkinnedTabFrame(state, geometry, index, false).DoIntersect(updateRect)) {
            PaintSkinnedTab(target, data, state, geometry, index);
        }
    }
    if (data.Selection < state.Tabs.size() && GetSkinnedTabFrame(state, geometry, data.Selection, true).DoIntersect(updateRect)) {
        PaintSkinnedTab(target, data, state, geometry, data.Selection);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::TabViewGetPreferredSize(
    const PView& target,
    const PTabViewStyleData& data,
    const PPoint& contentMinSize,
    const PPoint& contentMaxSize,
    PPoint* minSize,
    PPoint* maxSize) const
{
    if (m_TabViewSkin.Frame == nullptr)
    {
        PDefaultGUIStyle::TabViewGetPreferredSize(target, data, contentMinSize, contentMaxSize, minSize, maxSize);
        return;
    }
    const SkinnedTabViewStyleData& state = GetTabViewStyleData(target, data);
    const PRect& insets = m_TabViewSkin.FrameInsets;
    const float headerHeight = std::max(state.TabHeight, data.TopBarMinSize.y);
    const float topBarSpacing = (data.HasTopBarClient && !data.Titles.empty()) ? m_TabViewSkin.TabSpacing : 0.0f;
    const float horizontalBorder = insets.left + insets.right;
    const float verticalBorder = headerHeight + insets.top + insets.bottom;
    minSize->x = std::max(contentMinSize.x, state.TotalWidth + topBarSpacing + data.TopBarMinSize.x) + horizontalBorder;
    minSize->y = contentMinSize.y + verticalBorder;
    maxSize->x = std::max(contentMaxSize.x, state.TotalWidth + topBarSpacing + data.TopBarMaxSize.x) + horizontalBorder;
    maxSize->y = contentMaxSize.y + verticalBorder;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::TabViewPropertyChanged(PView& target, PView& tabsTarget, PTabViewStyleData& data, PTabViewProperties properties, size_t tabIndex) const
{
    if (m_TabViewSkin.Frame == nullptr)
    {
        PDefaultGUIStyle::TabViewPropertyChanged(target, tabsTarget, data, properties, tabIndex);
        return;
    }
    SkinnedTabViewStyleData& state = data.GetOrCreateStyleData<SkinnedTabViewStyleData>();
    const PTabViewProperties metricProperties(PTabViewProperty::TabInserted, PTabViewProperty::TabDeleted, PTabViewProperty::Title, PTabViewProperty::Font);
    const bool sizeChanged = properties.HasAny(metricProperties | PTabViewProperty::TopBar);
    if (properties.HasAny(metricProperties)) {
        state.MetricsValid = false;
    }
    if (sizeChanged) {
        target.PreferredSizeChanged();
    }
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data);
    const bool frameChanged = geometry.Bounds != state.PreviousBounds || geometry.Frames.TabsFrame != state.PreviousTabsFrame;
    if (!state.HasVisualState || sizeChanged || frameChanged)
    {
        target.Invalidate();
        tabsTarget.Invalidate();
    }
    else
    {
        const auto invalidateTab = [this, &tabsTarget, &state, &geometry](size_t index)
        {
            if (index < state.Tabs.size()) {
                tabsTarget.Invalidate(GetSkinnedTabFrame(state, geometry, index, true));
            }
        };
        if (properties.Has(PTabViewProperty::Selection))
        {
            invalidateTab(state.PreviousSelection);
            invalidateTab(data.Selection);
        }
        if (properties.Has(PTabViewProperty::Highlight))
        {
            invalidateTab(state.PreviousHighlight);
            invalidateTab(data.HighlightedTab);
            if (state.PreviousPressed != state.PreviousHighlight) {
                invalidateTab(state.PreviousPressed);
            }
            if (data.PressedTab != data.HighlightedTab) {
                invalidateTab(data.PressedTab);
            }
        }
        if (properties.Has(PTabViewProperty::Scroll) && geometry.Overflow)
        {
            target.Invalidate(geometry.OverflowLeft);
            target.Invalidate(geometry.OverflowRight);
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousHighlight = data.HighlightedTab;
    state.PreviousPressed = data.PressedTab;
    // Scrolling can notify the style while the scroll container is laying out.
    if (!properties.Has(PTabViewProperty::Scroll))
    {
        state.PreviousBounds = geometry.Bounds;
        state.PreviousTabsFrame = geometry.Frames.TabsFrame;
        state.HasVisualState = true;
    }
    if (sizeChanged) {
        tabsTarget.ContentSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabViewFrames PSkinnableGUIStyle::TabViewGetFrames(const PView& target, const PTabViewStyleData& data) const
{
    if (m_TabViewSkin.Frame == nullptr) {
        return PDefaultGUIStyle::TabViewGetFrames(target, data);
    } else {
        return GetSkinnedTabViewGeometry(target, data).Frames;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSkinnableGUIStyle::TabViewGetTabsContentSize(const PView& target, const PTabViewStyleData& data) const
{
    if (m_TabViewSkin.Frame == nullptr)
    {
        return PDefaultGUIStyle::TabViewGetTabsContentSize(target, data);
    }
    else
    {
        const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data);
        return PPoint(data.GetStyleData<SkinnedTabViewStyleData>()->TotalWidth, geometry.Frames.TabsFrame.Height());
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PSkinnableGUIStyle::TabViewGetTabFromPosition(const PView& target, const PView& tabsTarget, const PTabViewStyleData& data, const PPoint& position) const
{
    if (m_TabViewSkin.Frame == nullptr) {
        return PDefaultGUIStyle::TabViewGetTabFromPosition(target, tabsTarget, data, position);
    }
    if (!tabsTarget.GetBounds().DoIntersect(position)) {
        return INVALID_INDEX;
    }
    const SkinnedTabViewGeometry geometry = GetSkinnedTabViewGeometry(target, data);
    const SkinnedTabViewStyleData& state = *data.GetStyleData<SkinnedTabViewStyleData>();
    if (data.Selection < state.Tabs.size() && GetSkinnedTabFrame(state, geometry, data.Selection, true).DoIntersect(position)) {
        return data.Selection;
    }
    for (size_t index = state.Tabs.size(); index > 0; --index)
    {
        if (GetSkinnedTabFrame(state, geometry, index - 1, false).DoIntersect(position)) {
            return index - 1;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinState PSkinnableGUIStyle::GetButtonState(bool isEnabled, const PButtonStyleData& data)
{
    PButtonSkinState state = PButtonSkinState::Normal;
    if (!isEnabled) {
        state = PButtonSkinState::Disabled;
    } else if (data.IsPressed) {
        state = PButtonSkinState::Pressed;
    } else if (data.IsPointerOver) {
        state = PButtonSkinState::Hovered;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateIndicatorSkin(const PButtonIndicatorSkin& skin)
{
    if (!PMath::is_finite(skin.Size) || skin.Size <= 0.0f || !PMath::is_finite(skin.LabelSpacing) || !PMath::is_finite(skin.LabelVerticalOffset)) {
        PERROR_THROW_CODE(PErrorCode::INVAL);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateSliderSkin(const PSliderSkin& skin)
{
    for (const PSliderOrientationSkin* orientation : {&skin.Horizontal, &skin.Vertical})
    {
        for (const PSliderKnobSkin& knob : orientation->Knobs)
        {
            for (const PSliderKnobAppearance& appearance : knob.States)
            {
                if (!PMath::is_finite(appearance.Anchor.x) || !PMath::is_finite(appearance.Anchor.y)) {
                    PERROR_THROW_CODE(PErrorCode::INVAL);
                }
            }
            if (!PMath::is_finite(knob.Shadow.Anchor.x) || !PMath::is_finite(knob.Shadow.Anchor.y)) {
                PERROR_THROW_CODE(PErrorCode::INVAL);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateScrollBarSkin(const PScrollBarSkin& skin)
{
    for (const PScrollBarOrientationSkin* orientation : {&skin.Horizontal, &skin.Vertical})
    {
        if (!PMath::is_finite(orientation->Thickness) || orientation->Thickness <= 0.0f
            || !PMath::is_finite(orientation->ArrowLength) || orientation->ArrowLength < 0.0f
            || !PMath::is_finite(orientation->MinimumKnobLength) || orientation->MinimumKnobLength <= 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid scrollbar dimensions");
        }
        const PRect& insets = orientation->KnobInsets;
        for (float inset : {insets.left, insets.top, insets.right, insets.bottom})
        {
            if (!PMath::is_finite(inset) || inset < 0.0f) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Scrollbar knob insets must be finite and nonnegative");
            }
        }
        for (const PScrollBarSkinPart* part : {&orientation->Track, &orientation->Knob, &orientation->Decrement, &orientation->Increment})
        {
            if (orientation->Track.States[0].Image != nullptr && part->States[0].Image == nullptr) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Scrollbar parts require a normal image");
            }
            for (const PScrollBarSkinAppearance& appearance : part->States)
            {
                if (!PMath::is_finite(appearance.GlyphOffset.x) || !PMath::is_finite(appearance.GlyphOffset.y)) {
                    PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Scrollbar glyph offsets must be finite");
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateTabViewSkin(const PTabViewSkin& skin)
{
    const PRect& insets = skin.FrameInsets;
    for (float inset : {insets.left, insets.top, insets.right, insets.bottom})
    {
        if (!PMath::is_finite(inset) || inset < 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid tab-view frame inset");
        }
    }
    const PRect& padding = skin.LabelPadding;
    for (float value : {padding.left, padding.top, padding.right, padding.bottom, skin.TabSpacing, skin.InactiveTopInset})
    {
        if (!PMath::is_finite(value)) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Tab-view spacing must be finite");
        }
    }
    if (!PMath::is_finite(skin.MinimumTabSize.x) || !PMath::is_finite(skin.MinimumTabSize.y)
        || skin.MinimumTabSize.x <= 0.0f || skin.MinimumTabSize.y <= 0.0f
        || !PMath::is_finite(skin.OverflowWidth) || skin.OverflowWidth < 0.0f) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid tab-view dimensions");
    }
    for (const auto* appearances : {&skin.States, &skin.SelectedStates})
    {
        if (skin.Frame != nullptr && (*appearances)[0].Image == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Tab-view skins require normal and selected tab images");
        }
        for (const PTabSkinAppearance& appearance : *appearances)
        {
            if (!PMath::is_finite(appearance.LabelOffset.x) || !PMath::is_finite(appearance.LabelOffset.y)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Tab label offsets must be finite");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateDropdownMenuSkin(const PDropdownMenuSkin& skin)
{
    for (const PRect& insets : {skin.EditorInsets, skin.PopupInsets, skin.RowPadding})
    {
        for (float value : {insets.left, insets.top, insets.right, insets.bottom})
        {
            if (!PMath::is_finite(value)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown spacing must be finite");
            }
        }
    }
    for (float value : {skin.MinimumSize.x, skin.MinimumSize.y, skin.ArrowWidth, skin.MinimumRowHeight})
    {
        if (!PMath::is_finite(value) || value <= 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown dimensions must be finite and positive");
        }
    }
    if (skin.States[0].Background != nullptr)
    {
        for (const PDropdownMenuAppearance& appearance : skin.States)
        {
            if (appearance.Background == nullptr || appearance.Arrow == nullptr || appearance.Editor == nullptr) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown states require field, arrow and editor appearances");
            }
            if (!PMath::is_finite(appearance.ArrowOffset.x) || !PMath::is_finite(appearance.ArrowOffset.y)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown arrow offsets must be finite");
            }
        }
        if (skin.PopupFrame == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown skins require a popup frame");
        }
    }
    if (skin.PopupFrame != nullptr)
    {
        if (skin.PopupBackground == nullptr) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Dropdown popups require a background");
        }
        for (const PDropdownMenuRowAppearance& appearance : skin.Rows)
        {
            if (appearance.Image == nullptr || !PMath::is_finite(appearance.LabelOffset.x) || !PMath::is_finite(appearance.LabelOffset.y)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Invalid dropdown popup row appearance");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateGroupViewSkin(const PGroupViewSkin& skin)
{
    for (const PRect& spacing : {skin.FrameInsets, skin.LabelPadding})
    {
        for (float value : {spacing.left, spacing.top, spacing.right, spacing.bottom})
        {
            if (!PMath::is_finite(value)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Group view spacing must be finite");
            }
        }
    }
    if (!PMath::is_finite(skin.LabelGap)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Group view label gap must be finite");
    }
    for (float value : {skin.MinimumSize.x, skin.MinimumSize.y})
    {
        if (!PMath::is_finite(value) || value <= 0.0f) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Group view minimum dimensions must be finite and positive");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ValidateProgressBarSkin(const PProgressBarSkin& skin)
{
    const bool hasImages = skin.Horizontal.Track != nullptr || skin.Horizontal.Fill != nullptr
        || skin.Vertical.Track != nullptr || skin.Vertical.Fill != nullptr;
    for (const PProgressBarOrientationSkin* orientation : {&skin.Horizontal, &skin.Vertical})
    {
        if (hasImages && (orientation->Track == nullptr || orientation->Fill == nullptr)) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Progress bar skins require a track and fill for both orientations");
        }
        const PRect& insets = orientation->FillInsets;
        for (float value : {insets.left, insets.top, insets.right, insets.bottom})
        {
            if (!PMath::is_finite(value)) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Progress bar fill insets must be finite");
            }
        }
        for (float value : {orientation->MinimumLength, orientation->Thickness})
        {
            if (!PMath::is_finite(value) || value <= 0.0f) {
                PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Progress bar dimensions must be finite and positive");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PSkinnableGUIStyle::LoadBackgroundBrush(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("Style.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load style '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "Style") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Style.xml must have a Style root element");
    }
    const PString texture = root.attribute("background_texture").as_string();
    if (!texture.empty())
    {
        PPath texturePath(directory);
        texturePath.Append(texture);
        const Ptr<PBitmap> bitmap = PBitmapLoader::Load(texturePath, application);
        if (bitmap != nullptr) {
            return ptr_new<PBrush>(bitmap);
        }
    }
    return ptr_new<PBrush>(ParseSkinAttribute<PColor>(root, "background_color"), application);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkin PSkinnableGUIStyle::LoadButtonSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("Button.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load button skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "Button") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Button.xml must have a Button root element");
    }

    PButtonSkin skin;
    skin.LabelPadding = ParseSkinAttribute<PRect>(root, "label_padding");
    skin.PressedLabelOffset = ParseSkinAttribute<PPoint>(root, "pressed_label_offset");
    const PRect& padding = skin.LabelPadding;
    if (!PMath::is_finite(padding.left) || !PMath::is_finite(padding.top) || !PMath::is_finite(padding.right) || !PMath::is_finite(padding.bottom)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Button label padding must be finite");
    }
    if (!PMath::is_finite(skin.PressedLabelOffset.x) || !PMath::is_finite(skin.PressedLabelOffset.y)) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Pressed button label offset must be finite");
    }

    const pugi::xml_node checked = root.child("Checked");
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        skin.States[index] = LoadAppearance(root.child(STATE_NAMES[index]), directory, application);
        skin.CheckedStates[index] = LoadAppearance(checked.child(STATE_NAMES[index]), directory, application);
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PCheckBoxSkin PSkinnableGUIStyle::LoadCheckBoxSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("CheckBox.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load checkbox skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "CheckBox") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "CheckBox.xml must have a CheckBox root element");
    }

    PCheckBoxSkin skin;
    LoadIndicatorSkin(root, directory, application, skin);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRadioButtonSkin PSkinnableGUIStyle::LoadRadioButtonSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("RadioButton.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load radio button skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "RadioButton") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "RadioButton.xml must have a RadioButton root element");
    }

    PRadioButtonSkin skin;
    skin.LabelVerticalSpacing = ParseSkinAttribute<float>(root, "label_vertical_spacing");
    LoadIndicatorSkin(root, directory, application, skin);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderSkin PSkinnableGUIStyle::LoadSliderSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("Slider.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load slider skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "Slider") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Slider.xml must have a Slider root element");
    }
    PSliderSkin skin;
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        const pugi::xml_node node = root.child(STATE_NAMES[index]);
        PSliderSkinColors& colors = skin.Colors[index];
        colors.Before = ParseSkinAttribute<PColor>(node, "before_color");
        colors.After = ParseSkinAttribute<PColor>(node, "after_color");
        colors.Label = ParseSkinAttribute<PColor>(node, "text_color");
        colors.Tick = ParseSkinAttribute<PColor>(node, "tick_color");
        colors.TickHighlight = ParseSkinAttribute<PColor>(node, "tick_highlight");
    }
    skin.Horizontal = LoadSliderOrientation(root.child("Horizontal"), directory, application);
    skin.Vertical = LoadSliderOrientation(root.child("Vertical"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderOrientationSkin PSkinnableGUIStyle::LoadSliderOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PSliderOrientationSkin skin;
    const pugi::xml_node trackNode = node.child("Track");
    const Ptr<PBitmap> bitmap = LoadSkinBitmap(trackNode, directory, application);
    if (bitmap->GetColorSpace() != PEColorSpace::RGB32 && bitmap->GetColorSpace() != PEColorSpace::RGBA32) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Slider track masks must decode to RGB32 or RGBA32");
    }
    Ptr<PImage> track = ptr_new<PImage>(bitmap);
    const PRect borders = ParseSkinAttribute<PRect>(trackNode, "borders");
    const PIRect bounds = bitmap->GetBounds();
    track->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));
    track->SetLayoutSize(PPoint(bounds.Size()), PImageLayoutMode::Absolute);
    skin.Track = track;
    const bool horizontal = std::string_view(node.name()) == "Horizontal";
    const std::array<const char*, 4> shapes = {"Plain", horizontal ? "Up" : "Left", horizontal ? "Down" : "Right", "Both"};
    for (size_t shape = 0; shape < shapes.size(); ++shape)
    {
        const pugi::xml_node shapeNode = node.child(shapes[shape]);
        const PPoint anchor = ParseSkinAttribute<PPoint>(shapeNode, "anchor");
        for (size_t state = 0; state < STATE_NAMES.size(); ++state)
        {
            skin.Knobs[shape].States[state] = LoadSliderKnobAppearance(shapeNode.child(STATE_NAMES[state]), directory, application, anchor);
        }
        skin.Knobs[shape].Shadow = LoadSliderKnobAppearance(shapeNode.child("Shadow"), directory, application, anchor);
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderKnobAppearance PSkinnableGUIStyle::LoadSliderKnobAppearance(
    const pugi::xml_node& node,
    const PPath& directory,
    PApplication* application,
    const PPoint& anchor)
{
    PSliderKnobAppearance appearance;
    appearance.Bitmap = LoadSkinBitmap(node, directory, application);
    appearance.Anchor = node.attribute("anchor") ? ParseSkinAttribute<PPoint>(node, "anchor") : anchor;
    return appearance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarSkin PSkinnableGUIStyle::LoadScrollBarSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("ScrollBar.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load scrollbar skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "ScrollBar") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "ScrollBar.xml must have a ScrollBar root element");
    }
    PScrollBarSkin skin;
    skin.Horizontal = LoadScrollBarOrientation(root.child("Horizontal"), directory, application);
    skin.Vertical = LoadScrollBarOrientation(root.child("Vertical"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarOrientationSkin PSkinnableGUIStyle::LoadScrollBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PScrollBarOrientationSkin skin;
    skin.Thickness = ParseSkinAttribute<float>(node, "thickness");
    skin.ArrowLength = ParseSkinAttribute<float>(node, "arrow_length");
    skin.MinimumKnobLength = ParseSkinAttribute<float>(node, "minimum_knob_length");
    skin.KnobInsets = ParseSkinAttribute<PRect>(node, "knob_insets");
    skin.DoubleArrows = ParseSkinAttribute<bool>(node, "double_arrows");
    skin.Track = LoadScrollBarPart(node.child("Track"), directory, application);
    skin.Knob = LoadScrollBarPart(node.child("Knob"), directory, application);
    skin.Decrement = LoadScrollBarPart(node.child("Decrement"), directory, application);
    skin.Increment = LoadScrollBarPart(node.child("Increment"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarSkinPart PSkinnableGUIStyle::LoadScrollBarPart(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PScrollBarSkinPart part;
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        const pugi::xml_node state = node.child(STATE_NAMES[index]);
        const Ptr<PBitmap> bitmap = LoadSkinBitmap(state, directory, application);
        const PRect borders = ParseSkinAttribute<PRect>(state, "borders");
        const PIRect bounds = bitmap->GetBounds();
        Ptr<PImage> image = ptr_new<PImage>(bitmap);
        image->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));
        image->SetLayoutSize(PPoint(bounds.Size()), PImageLayoutMode::Absolute);
        part.States[index].Image = image;
        const pugi::xml_node glyph = state.child("Glyph");
        if (glyph)
        {
            part.States[index].Glyph = LoadSkinBitmap(glyph, directory, application);
            part.States[index].GlyphOffset = ParseSkinAttribute<PPoint>(glyph, "offset");
        }
    }
    return part;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabViewSkin PSkinnableGUIStyle::LoadTabViewSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("TabView.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load tab-view skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "TabView") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "TabView.xml must have a TabView root element");
    }
    PTabViewSkin skin;
    skin.FrameInsets = ParseSkinAttribute<PRect>(root, "frame_insets");
    skin.LabelPadding = ParseSkinAttribute<PRect>(root, "label_padding");
    skin.MinimumTabSize = ParseSkinAttribute<PPoint>(root, "minimum_tab_size");
    skin.TabSpacing = ParseSkinAttribute<float>(root, "tab_spacing");
    skin.InactiveTopInset = ParseSkinAttribute<float>(root, "inactive_top_inset");
    skin.OverflowWidth = ParseSkinAttribute<float>(root, "overflow_width");
    skin.Frame = LoadSkinImage(root.child("Frame"), directory, application);
    skin.OverflowLeft = LoadSkinBitmap(root.child("OverflowLeft"), directory, application);
    skin.OverflowRight = LoadSkinBitmap(root.child("OverflowRight"), directory, application);
    for (size_t selected = 0; selected < 2; ++selected)
    {
        const pugi::xml_node group = root.child((selected == 0) ? "Tab" : "Selected");
        for (size_t index = 0; index < TAB_STATE_NAMES.size(); ++index)
        {
            const pugi::xml_node node = group.child(TAB_STATE_NAMES[index]);
            PTabSkinAppearance& appearance = (selected == 0) ? skin.States[index] : skin.SelectedStates[index];
            appearance.Image = LoadSkinImage(node, directory, application);
            appearance.LabelColor = ParseSkinAttribute<PColor>(node, "text_color");
            appearance.LabelOffset = ParseSkinAttribute<PPoint>(node, "text_offset");
        }
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDropdownMenuSkin PSkinnableGUIStyle::LoadDropdownMenuSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("DropdownMenu.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load dropdown skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "DropdownMenu") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "DropdownMenu.xml must have a DropdownMenu root element");
    }
    PDropdownMenuSkin skin;
    skin.EditorInsets = ParseSkinAttribute<PRect>(root, "editor_insets");
    skin.MinimumSize = ParseSkinAttribute<PPoint>(root, "minimum_size");
    skin.ArrowWidth = ParseSkinAttribute<float>(root, "arrow_width");
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        const pugi::xml_node node = root.child(STATE_NAMES[index]);
        const PButtonSkinAppearance field = LoadAppearance(node, directory, application);
        PDropdownMenuAppearance& appearance = skin.States[index];
        appearance.Background = field.Background;
        const pugi::xml_node arrow = node.child("Arrow");
        appearance.Arrow = LoadSkinBitmap(arrow, directory, application);
        appearance.ArrowOffset = ParseSkinAttribute<PPoint>(arrow, "offset");
        Ptr<PTextBoxStyle> editor = ptr_new<PTextBoxStyle>();
        editor->BackgroundColor = ParseSkinAttribute<PColor>(node, "background_color");
        editor->ReadOnlyBackgroundColor = editor->BackgroundColor;
        editor->DisabledBackgroundColor = editor->BackgroundColor;
        editor->TextColor = field.LabelColor;
        editor->ReadOnlyTextColor = field.LabelColor;
        editor->DisabledTextColor = field.LabelColor;
        editor->Borders = PRect();
        editor->HasFrame = false;
        appearance.Editor = editor;
    }
    const pugi::xml_node popup = root.child("Popup");
    skin.PopupInsets = ParseSkinAttribute<PRect>(popup, "frame_insets");
    skin.RowPadding = ParseSkinAttribute<PRect>(popup, "row_padding");
    skin.MinimumRowHeight = ParseSkinAttribute<float>(popup, "minimum_row_height");
    skin.PopupBackground = ptr_new<PBrush>(ParseSkinAttribute<PColor>(popup, "background_color"), application);
    Ptr<PImage> frame = ptr_new<PImage>(*LoadSkinImage(popup.child("Frame"), directory, application));
    frame->SetLayoutSize(PPoint(1.0f), PImageLayoutMode::ViewRelative);
    skin.PopupFrame = ptr_new<PBrush>(frame);
    for (size_t index = 0; index < DROPDOWN_ROW_STATE_NAMES.size(); ++index)
    {
        const pugi::xml_node node = popup.child(DROPDOWN_ROW_STATE_NAMES[index]);
        PDropdownMenuRowAppearance& appearance = skin.Rows[index];
        appearance.Image = LoadSkinImage(node, directory, application);
        appearance.LabelColor = ParseSkinAttribute<PColor>(node, "text_color");
        appearance.LabelOffset = ParseSkinAttribute<PPoint>(node, "text_offset");
    }
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PGroupViewSkin PSkinnableGUIStyle::LoadGroupViewSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("GroupView.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load group view skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "GroupView") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "GroupView.xml must have a GroupView root element");
    }
    PGroupViewSkin skin;
    skin.Frame = LoadSkinImage(root.child("Frame"), directory, application);
    skin.FrameInsets = ParseSkinAttribute<PRect>(root, "frame_insets");
    skin.LabelPadding = ParseSkinAttribute<PRect>(root, "label_padding");
    skin.LabelGap = ParseSkinAttribute<float>(root, "label_gap");
    skin.LabelColor = ParseSkinAttribute<PColor>(root, "text_color");
    skin.MinimumSize = ParseSkinAttribute<PPoint>(root, "minimum_size");
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBarSkin PSkinnableGUIStyle::LoadProgressBarSkin(const PPath& directory, PApplication* application)
{
    PPath path(directory);
    path.Append("ProgressBar.xml");
    PXMLDocument document;
    if (!document.Load(path.GetPath())) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, PString::format_string("Unable to load progress bar skin '{}'", path.GetPath()));
    }
    const pugi::xml_node root = document.GetDocumentElement();
    if (std::string_view(root.name()) != "ProgressBar") {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "ProgressBar.xml must have a ProgressBar root element");
    }
    PProgressBarSkin skin;
    skin.Horizontal = LoadProgressBarOrientation(root.child("Horizontal"), directory, application);
    skin.Vertical = LoadProgressBarOrientation(root.child("Vertical"), directory, application);
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PProgressBarOrientationSkin PSkinnableGUIStyle::LoadProgressBarOrientation(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    PProgressBarOrientationSkin skin;
    skin.Track = LoadSkinImage(node.child("Track"), directory, application);
    skin.Fill = LoadSkinImage(node.child("Fill"), directory, application);
    skin.FillInsets = ParseSkinAttribute<PRect>(node, "fill_insets");
    skin.MinimumLength = ParseSkinAttribute<float>(node, "minimum_length");
    skin.Thickness = ParseSkinAttribute<float>(node, "thickness");
    return skin;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PImage> PSkinnableGUIStyle::LoadSkinImage(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    const Ptr<PBitmap> bitmap = LoadSkinBitmap(node, directory, application);
    const PRect borders = ParseSkinAttribute<PRect>(node, "borders");
    const PIRect bounds = bitmap->GetBounds();
    Ptr<PImage> image = ptr_new<PImage>(bitmap);
    image->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));
    image->SetLayoutSize(PPoint(bounds.Size()), PImageLayoutMode::Absolute);
    return image;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::LoadIndicatorSkin(const pugi::xml_node& root, const PPath& directory, PApplication* application, PButtonIndicatorSkin& skin)
{
    skin.Size = ParseSkinAttribute<float>(root, "size");
    skin.LabelSpacing = ParseSkinAttribute<float>(root, "label_spacing");
    skin.LabelVerticalOffset = ParseSkinAttribute<float>(root, "label_vertical_offset");

    const pugi::xml_node checked = root.child("Checked");
    for (size_t index = 0; index < STATE_NAMES.size(); ++index)
    {
        skin.States[index] = LoadAppearance(root.child(STATE_NAMES[index]), directory, application, PPoint(skin.Size), PImageLayoutMode::Absolute);
        skin.CheckedStates[index] = LoadAppearance(checked.child(STATE_NAMES[index]), directory, application, PPoint(skin.Size), PImageLayoutMode::Absolute);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PBitmap> PSkinnableGUIStyle::LoadSkinBitmap(const pugi::xml_node& node, const PPath& directory, PApplication* application)
{
    const PString filename = ParseSkinAttribute<PString>(node, "image");
    if (filename.empty()) {
        PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Skin image filename must not be empty");
    }
    PPath path(directory);
    path.Append(filename);
    Ptr<PBitmap> bitmap = PBitmapLoader::Load(path, application);
    if (bitmap == nullptr) {
        PERROR_THROW_CODE_WHAT(PErrorCode(get_last_error()), PString::format_string("Unable to load skin image '{}'", path.GetPath()));
    }
    return bitmap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinAppearance PSkinnableGUIStyle::LoadAppearance(
    const pugi::xml_node& node,
    const PPath& directory,
    PApplication* application,
    const PPoint& imageSize,
    PImageLayoutMode imageLayout)
{
    const PRect borders = ParseSkinAttribute<PRect>(node, "borders");
    const PColor labelColor = ParseSkinAttribute<PColor>(node, "text_color");
    Ptr<PBitmap> bitmap = LoadSkinBitmap(node, directory, application);
    Ptr<PImage> image = ptr_new<PImage>(bitmap);
    image->SetLayoutSize(imageSize, imageLayout);
    const PIRect bounds = bitmap->GetBounds();
    image->SetSliceRect(PRect(borders.left, borders.top, float(bounds.right) - borders.right, float(bounds.bottom) - borders.bottom));

    PButtonSkinAppearance appearance;
    appearance.Background = ptr_new<PBrush>(image);
    appearance.LabelColor = labelColor;
    return appearance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::PaintIndicatorButton(
    PView& target,
    const PRect& frame,
    bool isEnabled,
    PButtonStyleData& data,
    const PButtonIndicatorSkin& skin,
    const PPoint& indicatorPosition,
    const PPoint& labelPosition,
    bool labelOverlapsIndicator)
{
    const PButtonSkinState state = GetButtonState(isEnabled, data);
    const PButtonSkinAppearance& appearance = data.IsChecked ? skin.CheckedStates[std::to_underlying(state)] : skin.States[std::to_underlying(state)];
    const Ptr<const PBrush>& normal = skin.States[std::to_underlying(PButtonSkinState::Normal)].Background;
    const Ptr<const PBrush>& brush = (appearance.Background != nullptr) ? appearance.Background : normal;
    target.SetBackgroundBrush(nullptr);
    target.SetBrush(nullptr);
    target.SetFgColor(appearance.LabelColor);
    target.SetBrushOrigin(indicatorPosition);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreMode([&target, previousMode] { target.SetDrawingMode(previousMode); });
    target.SetDrawingMode(PDrawingMode::Copy);
    const PRect indicatorFrame(indicatorPosition, indicatorPosition + PPoint(skin.Size));
    if (labelOverlapsIndicator)
    {
        PaintOverlappingIndicator(target, frame, data.Label, labelPosition, indicatorFrame, brush);
    }
    else
    {
        target.DrawString(data.Label, labelPosition, frame);
        target.FillRect(indicatorFrame, brush);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::PaintOverlappingIndicator(
    PView& target,
    const PRect& frame,
    const PString& label,
    const PPoint& labelPosition,
    const PRect& indicatorFrame,
    Ptr<const PBrush> brush)
{
    const PPoint scrollOffset = target.GetScrollOffset();
    const PIRect paintFrame = PIRect(frame + scrollOffset) & PIRect(target.GetNormalizedBounds());
    if (!paintFrame.IsValid()) {
        return;
    }
    const PIRect imageFrame(indicatorFrame + scrollOffset);
    const Ptr<const PRegion> previousRegion = target.GetDrawingRegion();
    Ptr<PRegion> outside = (previousRegion != nullptr) ? ptr_new<PRegion>(*previousRegion, paintFrame, false) : ptr_new<PRegion>(paintFrame);
    const Ptr<const PRegion> inside = ptr_new<PRegion>(*outside, imageFrame, false);
    outside->Exclude(imageFrame);
    PScopeExit restoreRegion([&target, previousRegion] { target.SetDrawingRegion(previousRegion); });

    // Composite text against the image only within its bounds; image brushes repeat.
    target.SetDrawingRegion(outside);
    target.DrawString(label, labelPosition, frame);
    target.SetDrawingRegion(inside);
    target.DrawString(std::move(brush), label, labelPosition, frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSkinnableGUIStyle::GetSpacedExtent(float firstSize, float secondSize, float spacing)
{
    return std::max({firstSize, secondSize, firstSize + spacing + secondSize, -spacing});
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSkinnableGUIStyle::GetButtonImageMinimumSize() const
{
    PPoint size;
    for (const std::array<PButtonSkinAppearance, std::to_underlying(PButtonSkinState::Count)>* states
        : {&m_ButtonSkin.States, &m_ButtonSkin.CheckedStates})
    {
        for (const PButtonSkinAppearance& appearance : *states)
        {
            if (appearance.Background != nullptr)
            {
                const Ptr<const PImage> image = appearance.Background->GetImage();
                if (image != nullptr)
                {
                    const PImageDescription& description = image->GetDescription();
                    const PRect& slice = description.SliceRect;
                    const PIRect& source = description.SourceRect;
                    const PPoint scale = appearance.Background->GetScale();
                    size.x = std::max(size.x, (slice.left - float(source.left) + float(source.right) - slice.right) * scale.x);
                    size.y = std::max(size.y, (slice.top - float(source.top) + float(source.bottom) - slice.bottom) * scale.y);
                }
            }
        }
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PSkinnableGUIStyle::HasSliderSkin() const
{
    return m_SliderSkin.Horizontal.Track != nullptr && m_SliderSkin.Vertical.Track != nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::SkinnedSliderStyleData& PSkinnableGUIStyle::GetSliderStyleData(const PSliderStyleData& data) const
{
    SkinnedSliderStyleData& state = data.GetOrCreateStyleData<SkinnedSliderStyleData>();
    const PSliderOrientationSkin& orientation = (data.Orientation == POrientation::Horizontal) ? m_SliderSkin.Horizontal : m_SliderSkin.Vertical;
    const PSliderKnobSkin& knob = orientation.Knobs[GetSliderKnobIndex(data)];
    if (state.Knob != &knob)
    {
        state.Layout.SetKnobBounds(knob.GetBounds());
        state.Knob = &knob;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinState PSkinnableGUIStyle::GetSliderState(const PSliderStyleData& data)
{
    if (!data.IsEnabled) {
        return PButtonSkinState::Disabled;
    } else if (data.IsPressed) {
        return PButtonSkinState::Pressed;
    } else if (data.IsPointerOver) {
        return PButtonSkinState::Hovered;
    } else {
        return PButtonSkinState::Normal;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PSkinnableGUIStyle::GetSliderKnobIndex(const PSliderStyleData& data)
{
    return (data.KnobPointUp ? size_t(1) : size_t(0)) + (data.KnobPointDown ? size_t(2) : size_t(0));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t PSkinnableGUIStyle::ShadeSliderComponent(uint8_t component, uint8_t shade)
{
    return (shade < 128) ? uint8_t((uint32_t(component) * shade + 64) / 128) : uint8_t(component + (uint32_t(255 - component) * (shade - 128) + 63) / 127);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PImage> PSkinnableGUIStyle::CreateSliderTrackImage(const PImage& source, PColor color)
{
    const Ptr<const PBitmap> mask = source.GetBitmap();
    const PIRect bounds = mask->GetBounds();
    Ptr<PBitmap> bitmap = ptr_new<PBitmap>(bounds.Width(), bounds.Height(), PEColorSpace::RGBA32, PBitmap::SHARE_FRAMEBUFFER, mask->GetApplication());
    if (!bitmap->IsValid()) {
        PERROR_THROW_CODE(PErrorCode(get_last_error()));
    }
    const PBitmapRasterRO input = mask->LockRaster();
    const PBitmapRasterRW output = bitmap->LockRaster();
    for (size_t y = 0; y < size_t(bounds.Height()); ++y)
    {
        const uint32_t* sourcePixels = reinterpret_cast<const uint32_t*>(input.Get() + y * size_t(mask->GetBytesPerRow()));
        uint32_t* destinationPixels = reinterpret_cast<uint32_t*>(output.Get() + y * size_t(bitmap->GetBytesPerRow()));
        for (size_t x = 0; x < size_t(bounds.Width()); ++x)
        {
            const PColor sample = (mask->GetColorSpace() == PEColorSpace::RGBA32) ? PColor::FromRGB32A(sourcePixels[x]) : PColor::FromRGB32(sourcePixels[x]);
            const uint8_t shade = sample.GetRed();
            destinationPixels[x] = PColor(
                ShadeSliderComponent(color.GetRed(), shade),
                ShadeSliderComponent(color.GetGreen(), shade),
                ShadeSliderComponent(color.GetBlue(), shade),
                uint8_t((uint32_t(sample.GetAlpha()) * color.GetAlpha() + 127) / 255)).GetColor32();
        }
    }
    return ptr_new<PImage>(bitmap, source.GetDescription());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::UpdateSliderTrackImages(
    SkinnedSliderStyleData& state,
    const PSliderOrientationSkin& skin,
    const PSliderSkinColors& colors,
    const PSliderStyleData& data,
    const PPoint& size)
{
    const PColor before = data.HasCustomSliderColors ? data.SliderColor1 : colors.Before;
    const PColor after = data.HasCustomSliderColors ? data.SliderColor2 : colors.After;
    if (state.TrackSource != skin.Track || state.BeforeColor != before || state.AfterColor != after)
    {
        Ptr<PImage> beforeImage = CreateSliderTrackImage(*skin.Track, before);
        Ptr<PImage> afterImage = CreateSliderTrackImage(*skin.Track, after);
        Ptr<PBrush> beforeBrush = ptr_new<PBrush>(beforeImage);
        Ptr<PBrush> afterBrush = ptr_new<PBrush>(afterImage);
        state.BeforeImage = std::move(beforeImage);
        state.AfterImage = std::move(afterImage);
        state.BeforeBrush = std::move(beforeBrush);
        state.AfterBrush = std::move(afterBrush);
        state.TrackSource = skin.Track;
        state.BeforeColor = before;
        state.AfterColor = after;
        state.TrackSize = PPoint();
    }
    if (state.TrackSize != size)
    {
        state.BeforeImage->SetLayoutSize(size, PImageLayoutMode::Absolute);
        state.AfterImage->SetLayoutSize(size, PImageLayoutMode::Absolute);
        state.TrackSize = size;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::DrawSkinSliderKnob(
    PView& target,
    const PRect& knobFrame,
    const PRect& beforeFrame,
    const PRect& afterFrame,
    const SkinnedSliderStyleData& state,
    const Ptr<const PBitmap>& bitmap)
{
    if (bitmap == nullptr) {
        return;
    }
    const PPoint scroll = target.GetScrollOffset();
    const PIRect bitmapFrame(knobFrame + scroll);
    const PIRect frame = bitmapFrame & PIRect(target.GetNormalizedBounds());
    const PIRect before(beforeFrame + scroll);
    const PIRect after(afterFrame + scroll);
    const PIRect track = (before | after) & frame;
    std::array<PIRect, 6> sections;
    if (track.IsValid())
    {
        sections[0] = PIRect(frame.left, frame.top, frame.right, track.top);
        sections[1] = PIRect(frame.left, track.bottom, frame.right, frame.bottom);
        sections[2] = PIRect(frame.left, track.top, track.left, track.bottom);
        sections[3] = PIRect(track.right, track.top, frame.right, track.bottom);
        sections[4] = frame & before;
        sections[5] = frame & after;
    }
    else
    {
        sections[0] = frame;
    }
    const PIPoint sourceOffset = bitmap->GetBounds().TopLeft() - bitmapFrame.TopLeft();
    PScopeExit restoreBrush([&target] { target.SetBrush(nullptr); });
    for (size_t index = 0; index < sections.size(); ++index)
    {
        const PIRect& section = sections[index];
        if (section.IsValid())
        {
            if (index == 4) {
                target.SetBrush(state.BeforeBrush);
            } else if (index == 5) {
                target.SetBrush(state.AfterBrush);
            } else {
                target.SetBrush(nullptr);
            }
            target.DrawBitmap(bitmap, PRect(section + sourceOffset), PPoint(section.TopLeft()) - scroll);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PSkinnableGUIStyle::HasScrollBarSkin() const
{
    return m_ScrollBarSkin.Horizontal.Track.States[0].Image != nullptr && m_ScrollBarSkin.Vertical.Track.States[0].Image != nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PScrollBarOrientationSkin& PSkinnableGUIStyle::GetScrollBarOrientation(POrientation orientation) const
{
    return (orientation == POrientation::Horizontal) ? m_ScrollBarSkin.Horizontal : m_ScrollBarSkin.Vertical;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::SkinnedScrollBarGeometry PSkinnableGUIStyle::GetScrollBarGeometry(
    const PRect& frame,
    const PScrollBarStyleData& data,
    float relativeValue) const
{
    SkinnedScrollBarGeometry geometry;
    geometry.Frame = frame;
    if (!frame.IsValid()) {
        return geometry;
    }
    const PScrollBarOrientationSkin& skin = GetScrollBarOrientation(data.Orientation);
    const bool horizontal = data.Orientation == POrientation::Horizontal;
    const float start = horizontal ? frame.left : frame.top;
    const float end = horizontal ? frame.right : frame.bottom;
    const float arrowLength = std::min(skin.ArrowLength, std::floor((end - start) * 0.5f));
    float trackStart = start + arrowLength;
    float trackEnd = end - arrowLength;
    const auto makeFrame = [horizontal, &frame](float leading, float trailing)
    {
        return horizontal ? PRect(leading, frame.top, trailing, frame.bottom) : PRect(frame.left, leading, frame.right, trailing);
    };
    geometry.ArrowFrames[0] = makeFrame(start, trackStart);
    geometry.ArrowFrames[3] = makeFrame(trackEnd, end);
    const float insets = horizontal ? skin.KnobInsets.left + skin.KnobInsets.right : skin.KnobInsets.top + skin.KnobInsets.bottom;
    if (skin.DoubleArrows && arrowLength > 0.0f && trackEnd - trackStart >= 2.0f * arrowLength + skin.MinimumKnobLength + insets)
    {
        geometry.ArrowFrames[1] = makeFrame(trackStart, trackStart + arrowLength);
        geometry.ArrowFrames[2] = makeFrame(trackEnd - arrowLength, trackEnd);
        trackStart += arrowLength;
        trackEnd -= arrowLength;
    }
    geometry.TrackFrame = makeFrame(trackStart, trackEnd);
    geometry.KnobArea = geometry.TrackFrame.GetResized(skin.KnobInsets.left, skin.KnobInsets.top, -skin.KnobInsets.right, -skin.KnobInsets.bottom);
    if (geometry.KnobArea.IsValid())
    {
        const float areaLength = horizontal ? geometry.KnobArea.Width() : geometry.KnobArea.Height();
        const float proportional = std::round(areaLength * std::clamp(data.Proportion, 0.0f, 1.0f));
        const float knobLength = data.HasRange ? std::min(areaLength, std::max(skin.MinimumKnobLength, proportional)) : areaLength;
        const float travel = areaLength - knobLength;
        const float offset = std::min(travel, std::round(travel * std::clamp(relativeValue, 0.0f, 1.0f)));
        geometry.KnobFrame = geometry.KnobArea;
        if (horizontal)
        {
            geometry.KnobFrame.left += offset;
            geometry.KnobFrame.right = geometry.KnobFrame.left + knobLength;
        }
        else
        {
            geometry.KnobFrame.top += offset;
            geometry.KnobFrame.bottom = geometry.KnobFrame.top + knobLength;
        }
    }
    return geometry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::GetScrollBarPartFrame(const SkinnedScrollBarGeometry& geometry, PScrollBarPart part)
{
    if (part == PScrollBarPart::Knob) {
        return geometry.KnobFrame;
    }
    if (part == PScrollBarPart::PageDecrement || part == PScrollBarPart::PageIncrement) {
        return geometry.Frame;
    }
    for (size_t index = 0; index < SCROLL_BAR_ARROW_PARTS.size(); ++index)
    {
        if (SCROLL_BAR_ARROW_PARTS[index] == part) {
            return geometry.ArrowFrames[index];
        }
    }
    return PRect();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinState PSkinnableGUIStyle::GetScrollBarState(const PScrollBarStyleData& data, PScrollBarPart part)
{
    if (!data.IsEnabled || !data.HasRange) {
        return PButtonSkinState::Disabled;
    }
    const bool track = part == PScrollBarPart::None;
    const bool pressed = track
        ? data.PressedPart == PScrollBarPart::PageDecrement || data.PressedPart == PScrollBarPart::PageIncrement : data.PressedPart == part;
    const bool hovered = track
        ? data.HoveredPart == PScrollBarPart::PageDecrement || data.HoveredPart == PScrollBarPart::PageIncrement : data.HoveredPart == part;
    if (pressed) {
        return PButtonSkinState::Pressed;
    } else if (hovered) {
        return PButtonSkinState::Hovered;
    } else {
        return PButtonSkinState::Normal;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PSkinnableGUIStyle::GetScrollBarBrush(SkinnedScrollBarPart& cache, const PScrollBarSkinPart& part, PButtonSkinState state, const PPoint& size)
{
    const size_t index = std::to_underlying(state);
    if (cache.Images[index] == nullptr)
    {
        const Ptr<const PImage>& source = (part.States[index].Image != nullptr) ? part.States[index].Image : part.States[0].Image;
        cache.Images[index] = ptr_new<PImage>(*source);
        cache.Brushes[index] = ptr_new<PBrush>(cache.Images[index]);
    }
    if (cache.Images[index]->GetLayoutSize() != size) {
        cache.Images[index]->SetLayoutSize(size, PImageLayoutMode::Absolute);
    }
    return cache.Brushes[index];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::PaintScrollBarPart(
    PView& target,
    const PRect& frame,
    const PScrollBarSkinPart& part,
    SkinnedScrollBarPart& cache,
    PButtonSkinState state)
{
    if (!frame.IsValid()) {
        return;
    }
    const Ptr<const PBrush> brush = GetScrollBarBrush(cache, part, state, frame.Size());
    const size_t index = std::to_underlying(state);
    const PScrollBarSkinAppearance& appearance = (part.States[index].Image != nullptr) ? part.States[index] : part.States[0];
    target.SetBrushOrigin(frame.TopLeft());
    target.FillRect(frame, brush);
    if (appearance.Glyph != nullptr)
    {
        const PRect source(appearance.Glyph->GetBounds());
        const PPoint position = (frame.TopLeft() + (frame.Size() - source.Size()) * 0.5f + appearance.GlyphOffset).GetRounded();
        const PRect visible = PRect(position, position + source.Size()) & frame;
        if (visible.IsValid())
        {
            target.SetBrush(brush);
            target.DrawBitmap(appearance.Glyph, visible - position + source.TopLeft(), visible.TopLeft());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::ClearScrollBarTrack(PView& target, const SkinnedScrollBarGeometry& geometry)
{
    const PRect& track = geometry.TrackFrame;
    const PRect& knob = geometry.KnobFrame;
    if (!track.IsValid()) {
        return;
    }
    if (knob.IsValid())
    {
        const std::array<PRect, 4> clearFrames =
        {
            PRect(track.left, track.top, track.right, knob.top),
            PRect(track.left, knob.bottom, track.right, track.bottom),
            PRect(track.left, knob.top, knob.left, knob.bottom),
            PRect(knob.right, knob.top, track.right, knob.bottom)
        };
        for (const PRect& frame : clearFrames)
        {
            if (frame.IsValid()) {
                target.EraseRect(frame);
            }
        }
    }
    else
    {
        target.EraseRect(track);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::SkinnedTabViewStyleData& PSkinnableGUIStyle::GetTabViewStyleData(const PView& target, const PTabViewStyleData& data) const
{
    SkinnedTabViewStyleData& state = data.GetOrCreateStyleData<SkinnedTabViewStyleData>();
    if (!state.MetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        const PRect& padding = m_TabViewSkin.LabelPadding;
        const float labelHeight = state.FontHeight.ascender + state.FontHeight.descender;
        state.TabHeight = std::ceil(std::max(m_TabViewSkin.MinimumTabSize.y, labelHeight + padding.top + padding.bottom));
        state.Tabs.resize(data.Titles.size());
        state.TotalWidth = 0.0f;
        float left = 0.0f;
        for (size_t index = 0; index < data.Titles.size(); ++index)
        {
            SkinnedTabViewTab& tab = state.Tabs[index];
            tab.LabelWidth = target.GetStringWidth(data.Titles[index]);
            tab.Width = std::ceil(std::max(m_TabViewSkin.MinimumTabSize.x, tab.LabelWidth + padding.left + padding.right));
            tab.Left = left;
            state.TotalWidth = std::max(state.TotalWidth, left + tab.Width);
            left += std::max(0.0f, tab.Width + m_TabViewSkin.TabSpacing);
        }
        state.MetricsValid = true;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::SkinnedTabViewGeometry PSkinnableGUIStyle::GetSkinnedTabViewGeometry(const PView& target, const PTabViewStyleData& data) const
{
    const SkinnedTabViewStyleData& state = GetTabViewStyleData(target, data);
    const PRect& insets = m_TabViewSkin.FrameInsets;
    SkinnedTabViewGeometry geometry;
    geometry.Bounds = target.GetNormalizedBounds();
    geometry.HeaderHeight = std::min(geometry.Bounds.bottom, std::max(state.TabHeight, data.TopBarMinSize.y));
    geometry.Panel = PRect(0.0f, geometry.HeaderHeight, geometry.Bounds.right, geometry.Bounds.bottom);
    const float left = std::min(insets.left, geometry.Bounds.right);
    const float right = std::max(left, geometry.Bounds.right - insets.right);
    float tabsRight = right;
    if (data.HasTopBarClient)
    {
        const float spacing = data.Titles.empty() ? 0.0f : m_TabViewSkin.TabSpacing;
        const float availableWidth = right - left;
        const float extraWidth = availableWidth - state.TotalWidth - spacing;
        const float width = std::min(availableWidth, std::clamp(extraWidth, data.TopBarMinSize.x, data.TopBarMaxSize.x));
        const float height = std::min(geometry.HeaderHeight, data.TopBarMaxSize.y);
        geometry.Frames.TopBarClientFrame = PRect(right - width, 0.0f, right, height);
        tabsRight = std::clamp(geometry.Frames.TopBarClientFrame.left - spacing, left, right);
    }
    geometry.Overflow = state.TotalWidth > tabsRight - left;
    const float margin = geometry.Overflow ? std::min(m_TabViewSkin.OverflowWidth, (tabsRight - left) * 0.5f) : 0.0f;
    const float clientTop = std::min(geometry.HeaderHeight + insets.top, geometry.Bounds.bottom);
    const float clientBottom = std::max(clientTop, geometry.Bounds.bottom - insets.bottom);
    geometry.Frames.ClientFrame = PRect(left, clientTop, right, clientBottom);
    geometry.Frames.TabsFrame = PRect(left + margin, 0.0f, tabsRight - margin, clientTop);
    geometry.OverflowLeft = PRect(left, 0.0f, left + margin, geometry.HeaderHeight);
    geometry.OverflowRight = PRect(tabsRight - margin, 0.0f, tabsRight, geometry.HeaderHeight);
    return geometry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::GetSkinnedTabFrame(const SkinnedTabViewStyleData& state, const SkinnedTabViewGeometry& geometry, size_t index, bool selected) const
{
    const SkinnedTabViewTab& tab = state.Tabs[index];
    const float top = selected ? 0.0f : std::clamp(m_TabViewSkin.InactiveTopInset, 0.0f, geometry.HeaderHeight);
    const float bottom = selected ? geometry.Frames.TabsFrame.bottom : geometry.HeaderHeight;
    return PRect(tab.Left, top, tab.Left + tab.Width, bottom);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabSkinState PSkinnableGUIStyle::GetTabViewState(const PTabViewStyleData& data, size_t index)
{
    if (index == data.PressedTab) {
        return PTabSkinState::Pressed;
    } else if (index == data.HighlightedTab) {
        return PTabSkinState::Hovered;
    } else {
        return PTabSkinState::Normal;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PSkinnableGUIStyle::GetSkinBrush(SkinnedImage& cache, const PImage& source, const PPoint& size)
{
    if (cache.Image == nullptr)
    {
        cache.Image = ptr_new<PImage>(source);
        cache.Brush = ptr_new<PBrush>(cache.Image);
    }
    if (cache.Image->GetLayoutSize() != size) {
        cache.Image->SetLayoutSize(size, PImageLayoutMode::Absolute);
    }
    return cache.Brush;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::PaintSkinnedTab(
    PView& target,
    const PTabViewStyleData& data,
    SkinnedTabViewStyleData& state,
    const SkinnedTabViewGeometry& geometry,
    size_t index) const
{
    const bool selected = index == data.Selection;
    const PRect frame = GetSkinnedTabFrame(state, geometry, index, selected);
    if (!frame.IsValid()) {
        return;
    }
    const auto& appearances = selected ? m_TabViewSkin.SelectedStates : m_TabViewSkin.States;
    const size_t stateIndex = std::to_underlying(GetTabViewState(data, index));
    const PTabSkinAppearance& appearance = (appearances[stateIndex].Image != nullptr) ? appearances[stateIndex] : appearances[0];
    const size_t cacheIndex = stateIndex + (selected ? std::to_underlying(PTabSkinState::Count) : 0);
    SkinnedTabViewTab& tab = state.Tabs[index];
    const Ptr<const PBrush> brush = GetSkinBrush(tab.Appearances[cacheIndex], *appearance.Image, frame.Size());
    const PRect& padding = m_TabViewSkin.LabelPadding;
    const float labelHeight = state.FontHeight.ascender + state.FontHeight.descender;
    const PPoint baseline = (PPoint(
        frame.left + padding.left + (frame.Width() - padding.left - padding.right - tab.LabelWidth) * 0.5f,
        padding.top + (geometry.HeaderHeight - padding.top - padding.bottom - labelHeight) * 0.5f + state.FontHeight.ascender)
        + appearance.LabelOffset).GetRounded();
    target.SetBrushOrigin(frame.TopLeft());
    target.SetFgColor(appearance.LabelColor);
    const PRect labelBounds(baseline.x, baseline.y - state.FontHeight.ascender, baseline.x + tab.LabelWidth, baseline.y + state.FontHeight.descender);
    if ((labelBounds & frame) == labelBounds)
    {
        target.DrawString(brush, data.Titles[index], baseline, frame);
    }
    else
    {
        // Only unusual padding/offsets or a very short view need extra clipping.
        const PIRect clip = PIRect(frame + target.GetScrollOffset()) & PIRect(target.GetNormalizedBounds());
        const Ptr<const PRegion> previousRegion = target.GetDrawingRegion();
        const Ptr<const PRegion> region = (previousRegion != nullptr) ? ptr_new<PRegion>(*previousRegion, clip, false) : ptr_new<PRegion>(clip);
        PScopeExit restoreRegion([&target, previousRegion] { target.SetDrawingRegion(previousRegion); });
        target.SetDrawingRegion(region);
        target.DrawString(brush, data.Titles[index], baseline, frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::PaintTabViewOverflow(PView& target, const PRect& frame, const Ptr<const PBitmap>& bitmap)
{
    if (frame.IsValid() && bitmap != nullptr)
    {
        const PRect source(bitmap->GetBounds());
        const PPoint position = (frame.TopLeft() + (frame.Size() - source.Size()) * 0.5f).GetRounded();
        const PRect visible = PRect(position, position + source.Size()) & frame;
        if (visible.IsValid()) {
            target.DrawBitmap(bitmap, visible - position + source.TopLeft(), visible.TopLeft());
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonSkinState PSkinnableGUIStyle::GetDropdownMenuState(const PDropdownMenuStyleData& data)
{
    if (!data.IsEnabled) {
        return PButtonSkinState::Disabled;
    } else if (data.IsOpen) {
        return PButtonSkinState::Pressed;
    } else if (data.IsPointerOver) {
        return PButtonSkinState::Hovered;
    } else {
        return PButtonSkinState::Normal;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::GetDropdownMenuArrowFrame(const PRect& frame) const
{
    return PRect(std::max(frame.left, frame.right - m_DropdownMenuSkin.ArrowWidth), frame.top, frame.right, frame.bottom);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::InsetSkinFrame(const PRect& frame, const PRect& insets)
{
    PRect result = frame.GetResized(insets.left, insets.top, -insets.right, -insets.bottom);
    result.left = std::min(result.left, frame.right);
    result.top = std::min(result.top, frame.bottom);
    result.right = std::max(result.left, result.right);
    result.bottom = std::max(result.top, result.bottom);
    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::SkinnedDropdownMenuStyleData& PSkinnableGUIStyle::GetDropdownMenuStyleData(const PView& target, const PDropdownMenuStyleData& data)
{
    SkinnedDropdownMenuStyleData& state = data.GetOrCreateStyleData<SkinnedDropdownMenuStyleData>();
    if (!state.MetricsValid)
    {
        state.ItemWidth = 0.0f;
        for (const PString& item : data.Items) {
            state.ItemWidth = std::max(state.ItemWidth, target.GetStringWidth(item));
        }
        state.MetricsValid = true;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::SkinnedDropdownMenuPopupStyleData& PSkinnableGUIStyle::GetDropdownMenuPopupStyleData(
    const PView& target,
    const PDropdownMenuPopupStyleData& data) const
{
    SkinnedDropdownMenuPopupStyleData& state = data.GetOrCreateStyleData<SkinnedDropdownMenuPopupStyleData>();
    if (!state.MetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        state.ItemWidth = 0.0f;
        for (const PString& item : data.Items) {
            state.ItemWidth = std::max(state.ItemWidth, target.GetStringWidth(item));
        }
        const PRect& padding = m_DropdownMenuSkin.RowPadding;
        const float textHeight = state.FontHeight.ascender + state.FontHeight.descender;
        state.ItemHeight = std::max(m_DropdownMenuSkin.MinimumRowHeight, std::ceil(textHeight + padding.top + padding.bottom));
        state.ContentSize = PPoint(std::max(0.0f, state.ItemWidth + padding.left + padding.right), float(data.Items.size()) * state.ItemHeight);
        state.MetricsValid = true;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::PaintDropdownMenuRow(
    PView& target,
    const PDropdownMenuPopupStyleData& data,
    SkinnedDropdownMenuPopupStyleData& state,
    size_t index) const
{
    PDropdownMenuRowState rowState = PDropdownMenuRowState::Normal;
    if (index == data.PressedItem) {
        rowState = PDropdownMenuRowState::Pressed;
    } else if (index == data.Selection) {
        rowState = PDropdownMenuRowState::Selected;
    }
    const size_t appearanceIndex = std::to_underlying(rowState);
    const PDropdownMenuRowAppearance& appearance = m_DropdownMenuSkin.Rows[appearanceIndex];
    const PRect frame = DropdownMenuPopupGetItemFrame(target, data, index);
    if (!frame.IsValid()) {
        return;
    }
    const Ptr<const PBrush> brush = GetSkinBrush(state.Rows[appearanceIndex], *appearance.Image, frame.Size());
    const PRect& padding = m_DropdownMenuSkin.RowPadding;
    const float textHeight = state.FontHeight.ascender + state.FontHeight.descender;
    const PPoint baseline(
        std::round(frame.left + padding.left + appearance.LabelOffset.x),
        std::round(frame.top + padding.top + (frame.Height() - padding.top - padding.bottom - textHeight) * 0.5f
            + state.FontHeight.ascender + appearance.LabelOffset.y));
    target.SetBrushOrigin(frame.TopLeft());
    target.SetFgColor(appearance.LabelColor);
    const bool fits = baseline.x >= frame.left && baseline.x + state.ItemWidth <= frame.right
        && baseline.y - state.FontHeight.ascender >= frame.top && baseline.y + state.FontHeight.descender <= frame.bottom;
    if (fits)
    {
        target.DrawString(brush, data.Items[index], baseline, frame);
    }
    else
    {
        const PIRect clip = PIRect(frame + target.GetScrollOffset()) & PIRect(target.GetNormalizedBounds());
        const Ptr<const PRegion> previousRegion = target.GetDrawingRegion();
        const Ptr<const PRegion> region = (previousRegion != nullptr) ? ptr_new<PRegion>(*previousRegion, clip, false) : ptr_new<PRegion>(clip);
        PScopeExit restoreRegion([&target, previousRegion] { target.SetDrawingRegion(previousRegion); });
        target.SetDrawingRegion(region);
        target.DrawString(brush, data.Items[index], baseline, frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSkinnableGUIStyle::SkinnedGroupViewStyleData& PSkinnableGUIStyle::GetGroupViewStyleData(const PView& target, const PGroupViewStyleData& data) const
{
    SkinnedGroupViewStyleData& state = data.GetOrCreateStyleData<SkinnedGroupViewStyleData>();
    if (!state.MetricsValid)
    {
        state.LabelSize = PPoint();
        state.LabelAscender = 0.0f;
        state.HeaderHeight = 0.0f;
        const PRect& padding = m_GroupViewSkin.LabelPadding;
        if (!data.Label.empty())
        {
            const PFontHeight fontHeight = target.GetFontHeight();
            state.LabelSize = PPoint(target.GetStringWidth(data.Label), fontHeight.ascender + fontHeight.descender);
            state.LabelAscender = fontHeight.ascender;
            state.HeaderHeight = std::max(0.0f, state.LabelSize.y + padding.top + padding.bottom);
        }
        const PImageDescription& description = m_GroupViewSkin.Frame->GetDescription();
        const PPoint frameBorders = PPoint(description.SourceRect.Size()) - description.SliceRect.Size();
        const PRect& insets = m_GroupViewSkin.FrameInsets;
        state.MinimumSize = PPoint(
            std::max(m_GroupViewSkin.MinimumSize.x, frameBorders.x + insets.left + insets.right),
            std::max(m_GroupViewSkin.MinimumSize.y, frameBorders.y + insets.top + insets.bottom + std::floor(state.HeaderHeight * 0.5f)));
        if (!data.Label.empty())
        {
            state.MinimumSize.x = std::max(state.MinimumSize.x, state.LabelSize.x + padding.left + padding.right);
            state.MinimumSize.y = std::max(state.MinimumSize.y, state.HeaderHeight);
        }
        state.MetricsValid = true;
    }
    return state;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSkinnableGUIStyle::PaintGroupViewFrame(PView& target, const PRect& updateRect, const PRect& frame, SkinnedGroupViewStyleData& state) const
{
    const PRect bounds = target.GetBounds();
    const PRect visible = frame & bounds;
    if (!visible.IsValid())
    {
        target.EraseRect(updateRect);
        return;
    }
    const Ptr<const PBrush> brush = GetSkinBrush(state.Frame, *m_GroupViewSkin.Frame, frame.Size());
    target.SetBrushOrigin(frame.TopLeft());
    const PRect frameDamage = visible & updateRect;
    if (frameDamage.IsValid()) {
        target.FillRect(frameDamage, brush);
    }
    const std::array<PRect, 4> outside =
    {
        PRect(bounds.left, bounds.top, bounds.right, visible.top),
        PRect(bounds.left, visible.top, visible.left, visible.bottom),
        PRect(visible.right, visible.top, bounds.right, visible.bottom),
        PRect(bounds.left, visible.bottom, bounds.right, bounds.bottom)
    };
    for (const PRect& strip : outside)
    {
        const PRect damage = strip & updateRect;
        if (damage.IsValid()) {
            target.EraseRect(damage);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const PProgressBarOrientationSkin& PSkinnableGUIStyle::GetProgressBarOrientation(POrientation orientation) const
{
    return (orientation == POrientation::Horizontal) ? m_ProgressBarSkin.Horizontal : m_ProgressBarSkin.Vertical;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSkinnableGUIStyle::GetProgressBarFillFrame(const PRect& bounds, const PProgressBarOrientationSkin& skin, const PProgressBarStyleData& data)
{
    PRect fillFrame = InsetSkinFrame(bounds, skin.FillInsets).GetFloored();
    const float value = PMath::is_finite(data.Value) ? std::clamp(data.Value, 0.0f, 1.0f) : 0.0f;
    if (data.Orientation == POrientation::Horizontal) {
        fillFrame.right = fillFrame.left + std::ceil(fillFrame.Width() * value);
    } else {
        fillFrame.top = fillFrame.bottom - std::ceil(fillFrame.Height() * value);
    }
    return fillFrame;
}
