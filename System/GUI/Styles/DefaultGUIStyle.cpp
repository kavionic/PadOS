// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <vector>

#include <App/Application.h>
#include <GUI/Styles/DefaultGUIStyle.h>
#include <GUI/Style/ButtonStyleData.h>
#include <GUI/Style/DropdownMenuStyleData.h>
#include <GUI/Style/DropdownMenuPopupStyleData.h>
#include <GUI/Style/GroupViewStyleData.h>
#include <GUI/Style/ProgressBarStyleData.h>
#include <GUI/Style/TabViewStyleData.h>
#include <GUI/Style/SliderStyleData.h>
#include <GUI/Style/SliderStyleLayout.h>
#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Bitmap.h>
#include <GUI/View.h>
#include <GUI/Widgets/TextBox.h>
#include <System/ExceptionHandling.h>

struct PDefaultGUIStyle::SliderStyleData : PtrTarget
{
    PSliderStyleLayout Layout;
};

struct PDefaultGUIStyle::ScrollBarStyleData : PtrTarget
{
    Ptr<PBitmap> DecrementArrowBitmap;
    Ptr<PBitmap> IncrementArrowBitmap;
    PScrollBarPart PressedPart = PScrollBarPart::None;
    bool HasVisualState = false;
};

struct PDefaultGUIStyle::ScrollBarGeometry
{
    PRect ArrowFrames[4];
    PRect TrackFrame;
    PRect KnobFrame;
};

struct PDefaultGUIStyle::DropdownMenuStyleData : PtrTarget
{
    Ptr<PBitmap> ArrowBitmap;
    float ItemWidth = 0.0f;
    bool AreMetricsValid = false;
};

struct PDefaultGUIStyle::DropdownMenuPopupStyleData : PtrTarget
{
    PPoint ContentSize;
    PFontHeight FontHeight;
    float ItemHeight = 0.0f;
    size_t PreviousSelection = INVALID_INDEX;
    size_t PreviousPressedItem = INVALID_INDEX;
    bool AreMetricsValid = false;
    bool HasVisualState = false;
};

struct PDefaultGUIStyle::GroupViewStyleData : PtrTarget
{
    PPoint LabelSize;
    float LabelAscender = 0.0f;
    bool AreMetricsValid = false;
};

struct PDefaultGUIStyle::ProgressBarStyleData : PtrTarget
{
    PRect PreviousFrame;
};

struct PDefaultGUIStyle::TabViewStyleData : PtrTarget
{
    std::vector<float> TabWidths;
    PFontHeight FontHeight;
    PRect PreviousFrame;
    PRect PreviousTabsFrame;
    float GlyphHeight = 0.0f;
    float TabHeight = 0.0f;
    float TotalTabsWidth = 0.0f;
    float PreviousTabsRight = 0.0f;
    bool AreMetricsValid = false;
    bool HasVisualState = false;
    bool PreviousOverflow = false;
};

struct PDefaultGUIStyle::TabViewGeometry
{
    PTabViewFrames Frames;
    PRect Bounds;
    PRect HeaderFrame;
    float TabHeight = 0.0f;
    float TabsRight = 0.0f;
    bool Overflow = false;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDefaultGUIStyle::PDefaultGUIStyle(PApplication* application)
    : m_Application(ptr_weak_cast((application != nullptr) ? application : PApplication::GetCurrentApplication()))
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PBrush> PDefaultGUIStyle::GetBackgroundBrush() const
{
    PApplication* application = ptr_raw_pointer_cast(m_Application);
    if (application == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NOENT);
    }
    return application->GetStandardBrush(PStandardColorID::DefaultBackground);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ButtonPaint(PView& target, const PRect& bounds, bool isEnabled, PButtonStyleData& data) const
{
    data.UpdateMetrics(target);

    const PStandardColorID backgroundColor =
        (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::ButtonBackground;
    target.SetBackgroundBrush(backgroundColor);

    if (isEnabled) {
        target.DrawFrame(bounds, (data.IsPressed ? FRAME_RECESSED : FRAME_RAISED) | FRAME_TRANSPARENT);
    } else {
        target.DrawFrame(bounds, FRAME_DISABLED | FRAME_TRANSPARENT);
    }
    PPoint labelPosition(std::round((bounds.Width() - data.LabelSize.x) * 0.5f), std::round((bounds.Height() - data.LabelSize.y) * 0.5f));

    if (data.IsPressed) {
        labelPosition += PPoint(1.0f, 1.0f);
    }

    labelPosition.y += target.GetFontHeight().ascender;
    target.SetFgColor(isEnabled ? PStandardColorID::ButtonLabelNormal : PStandardColorID::ButtonLabelDisabled);
    target.SetBrush(backgroundColor);
    target.DrawString(data.Label, labelPosition, bounds.GetFloored().GetResized(2.0f, 2.0f, -2.0f, -2.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    data.UpdateMetrics(target);
    const PPoint size(data.LabelSize.x + 16.0f, data.LabelSize.y + 8.0f);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::CheckBoxPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const
{
    const PStandardColorID backgroundColor =
        (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    const PFontHeight fontHeight = target.GetFontHeight();
    const float baseline = frame.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender;
    target.SetFgColor(0, 0, 0);
    target.SetBrush(PStandardColorID::DefaultBackground);
    target.DrawString(target.GetBrush(), data.Label, PPoint(CHECK_BOX_SIZE + CHECK_BOX_LABEL_SPACING, baseline), frame);

    target.SetBackgroundBrush(backgroundColor);

    const float buttonTop = (frame.Height() - CHECK_BOX_SIZE) * 0.5f;
    const PRect buttonFrame(0.0f, buttonTop, CHECK_BOX_SIZE, buttonTop + CHECK_BOX_SIZE);
    target.DrawFrame(buttonFrame, FRAME_RECESSED);

    if (data.IsChecked)
    {
        target.SetFgColor(0, 0, 0);
        PPoint point1(buttonFrame.left + CHECK_BOX_CHECK_BORDER, buttonFrame.top + CHECK_BOX_CHECK_BORDER);
        PPoint point2(buttonFrame.right - CHECK_BOX_CHECK_BORDER, buttonFrame.bottom - CHECK_BOX_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            target.DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
        point1 = PPoint(buttonFrame.left + CHECK_BOX_CHECK_BORDER, buttonFrame.bottom - CHECK_BOX_CHECK_BORDER);
        point2 = PPoint(buttonFrame.right - CHECK_BOX_CHECK_BORDER, buttonFrame.top + CHECK_BOX_CHECK_BORDER);

        for (int i = 0; i < 7; ++i) {
            target.DrawLine(point1 + PPoint(float(i - 3), 0.0f), point2 + PPoint(float(i - 3), 0.0f));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::CheckBoxGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    PPoint size(CHECK_BOX_SIZE, CHECK_BOX_SIZE);
    if (!data.Label.empty())
    {
        data.UpdateMetrics(target);
        const PFontHeight fontHeight = target.GetFontHeight();
        size.x += CHECK_BOX_LABEL_SPACING + data.LabelSize.x;
        size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender);
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::RadioButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const
{
    const PStandardColorID backgroundColor =
        (isEnabled && data.IsPointerOver) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground;
    const PRect buttonFrame = DrawRadioButtonLabel(target, frame, data);

    target.SetBackgroundBrush(PStandardColorID::DefaultBackground);
    target.SetFgColor(PStandardColorID::Shadow);

    float radius = std::floor(RADIO_BUTTON_SIZE * 0.5f);
    const PPoint center(std::floor(buttonFrame.left + buttonFrame.Width() * 0.5f), std::floor(buttonFrame.top + buttonFrame.Height() * 0.5f));

    radius -= 1.0f;
    target.FillCircle(center, radius);

    target.SetFgColor(backgroundColor);
    radius -= 3.0f;
    target.FillCircle(center, radius);

    if (data.IsChecked)
    {
        target.SetFgColor(PStandardColorID::Shadow);
        radius -= 3.0f;
        target.FillCircle(center, radius);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::RadioButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    PPoint size(RADIO_BUTTON_SIZE, RADIO_BUTTON_SIZE);
    if (!data.Label.empty())
    {
        data.UpdateMetrics(target);
        const PFontHeight fontHeight = target.GetFontHeight();
        if (data.LabelAlignment == PAlignment::Top || data.LabelAlignment == PAlignment::Bottom)
        {
            size.x = std::max(size.x, data.LabelSize.x);
            size.y += RADIO_BUTTON_LABEL_VERTICAL_SPACING + fontHeight.ascender + fontHeight.descender;
        }
        else
        {
            size.x += RADIO_BUTTON_LABEL_HORIZONTAL_SPACING + data.LabelSize.x;
            size.y = std::max(size.y, fontHeight.ascender + fontHeight.descender);
        }
    }
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::SliderPaint(PView& target, const PRect& frame, const PSliderStyleData& data) const
{
    target.SetBackgroundBrush(PStandardColorID::SliderTrackNormal);

    PSliderStyleLayout& layout = GetSliderLayout(data);
    const PRect& bounds = frame;

    const PRect sliderFrame = layout.GetTrackFrame(target, bounds, data);
    const PPoint knobPosition = PSliderStyleLayout::GetPosition(sliderFrame, data.Orientation, data.Value);
    const PRect knobBounds = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame);
    const PRect knobFrame = knobBounds + knobPosition;

    layout.ClearBackground(target, bounds, sliderFrame, knobFrame, data);
    const PRect labelFrame = layout.GetValueLabelFrame(target, data);

    PSliderStyleLayout::DrawTicks(target, sliderFrame, data, pget_standard_color(PStandardColorID::Shadow), pget_standard_color(PStandardColorID::Shine));
    layout.DrawLabels(target, bounds, sliderFrame, data, PColor(0, 0, 0));

    DrawSliderTrack(target, sliderFrame, knobFrame, data);
    for (float value : data.ShadowValues) {
        DrawSliderKnob(target, sliderFrame, data, PStandardColorID::SliderKnobShadow, value);
    }

    const PStandardColorID knobColor = data.IsPressed ? PStandardColorID::SliderKnobPressed : PStandardColorID::SliderKnobNormal;
    DrawSliderKnob(target, sliderFrame, data, knobColor, data.Value);
    if (labelFrame.IsValid())
    {
        target.SetFgColor(0, 0, 0);
        target.SetBrush(nullptr);
        target.DrawString(data.ValueLabel, labelFrame.TopLeft() + PPoint(0.0f, target.GetFontHeight().ascender), labelFrame);
    }
    if (!layout.HasVisualState()) {
        layout.UpdateDamage(target, data, PSliderProperties(), false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::SliderGetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    GetSliderLayout(data).GetPreferredSize(target, data, minSize, maxSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::SliderPropertyChanged(PView& target, PSliderStyleData& data, PSliderProperties properties) const
{
    GetSliderLayout(data).PropertyChanged(target, data, properties);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PDefaultGUIStyle::SliderGetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue) const
{
    return GetSliderLayout(data).GetValuePosition(target, data, relativeValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PDefaultGUIStyle::SliderGetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position) const
{
    return GetSliderLayout(data).GetValueFromPosition(target, data, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ScrollBarPaint(PView& target, const PRect& frame, const PScrollBarStyleData& data) const
{
    const ScrollBarGeometry geometry = GetScrollBarGeometry(frame, data, data.Value);
    DrawScrollBarKnob(target, geometry.KnobFrame, data.Orientation);
    DrawScrollBarTrack(target, geometry, data.Orientation);
    DrawScrollBarArrows(target, geometry, data);
    ScrollBarStyleData& state = data.GetOrCreateStyleData<ScrollBarStyleData>();
    state.PressedPart = data.PressedPart;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ScrollBarGetPreferredSize(const PView& target, const PScrollBarStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (data.Orientation == POrientation::Horizontal)
    {
        *minSize = PPoint(0.0f, SCROLL_BAR_THICKNESS);
        *maxSize = PPoint(COORD_MAX, SCROLL_BAR_THICKNESS);
    }
    else
    {
        *minSize = PPoint(SCROLL_BAR_THICKNESS, 0.0f);
        *maxSize = PPoint(SCROLL_BAR_THICKNESS, COORD_MAX);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ScrollBarPropertyChanged(PView& target, PScrollBarStyleData& data, PScrollBarProperties properties) const
{
    ScrollBarStyleData& state = data.GetOrCreateStyleData<ScrollBarStyleData>();

    const PScrollBarProperties  trackProperties(PScrollBarProperty::Value, PScrollBarProperty::Proportion, PScrollBarProperty::Range);
    const PScrollBarProperties  incrementalProperties = trackProperties | PScrollBarProperty::Pressed | PScrollBarProperty::Hovered;
    const bool                  propertiesRequireFullUpdate = !(properties & ~incrementalProperties).Empty();

    if (!state.HasVisualState || propertiesRequireFullUpdate)
    {
        target.Invalidate();
    }
    else
    {
        const ScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
        if (properties.HasAny(trackProperties) && geometry.TrackFrame.IsValid()) {
            target.Invalidate(geometry.TrackFrame);
        }
        if (properties.Has(PScrollBarProperty::Pressed))
        {
            for (size_t i = 0; i < std::size(geometry.ArrowFrames); ++i)
            {
                const PScrollBarPart part = GetScrollBarArrowPart(i);
                if ((part == state.PressedPart || part == data.PressedPart) && geometry.ArrowFrames[i].IsValid()) {
                    target.Invalidate(geometry.ArrowFrames[i]);
                }
            }
        }
    }
    state.PressedPart = data.PressedPart;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarPart PDefaultGUIStyle::ScrollBarHitTest(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const
{
    if (!data.IsEnabled) {
        return PScrollBarPart::None;
    }
    const ScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
    for (size_t i = 0; i < std::size(geometry.ArrowFrames); ++i)
    {
        if (geometry.ArrowFrames[i].DoIntersect(position)) {
            return GetScrollBarArrowPart(i);
        }
    }
    if (geometry.KnobFrame.DoIntersect(position)) {
        return PScrollBarPart::Knob;
    }
    if (geometry.TrackFrame.DoIntersect(position))
    {
        const bool isBeforeKnob = (data.Orientation == POrientation::Horizontal)
            ? position.x < geometry.KnobFrame.left : position.y < geometry.KnobFrame.top;
        return isBeforeKnob ? PScrollBarPart::PageDecrement : PScrollBarPart::PageIncrement;
    }
    return PScrollBarPart::None;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PDefaultGUIStyle::ScrollBarGetValuePosition(const PView& target, const PScrollBarStyleData& data, float relativeValue) const
{
    return GetScrollBarGeometry(target.GetNormalizedBounds(), data, relativeValue).KnobFrame.Center();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PDefaultGUIStyle::ScrollBarGetValueFromPosition(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const
{
    const ScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, data.Value);
    if (!geometry.TrackFrame.IsValid()) {
        return data.Value;
    }
    const bool isHorizontal = data.Orientation == POrientation::Horizontal;
    const float trackStart = isHorizontal ? geometry.TrackFrame.left : geometry.TrackFrame.top;
    const float trackLength = isHorizontal ? geometry.TrackFrame.Width() : geometry.TrackFrame.Height();
    const float knobLength = isHorizontal ? geometry.KnobFrame.Width() : geometry.KnobFrame.Height();
    const float travel = trackLength - knobLength;
    const float offset = (isHorizontal ? position.x : position.y) - trackStart - knobLength * 0.5f;
    return (travel > 0.0f) ? std::clamp(offset / travel, 0.0f, 1.0f) : data.Value;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuPaint(PView& target, const PRect& frame, const PDropdownMenuStyleData& data) const
{
    DropdownMenuStyleData& state = data.GetOrCreateStyleData<DropdownMenuStyleData>();
    if (state.ArrowBitmap == nullptr)
    {
        state.ArrowBitmap = ptr_new<PBitmap>(
            DROPDOWN_MENU_ARROW_WIDTH,
            DROPDOWN_MENU_ARROW_HEIGHT,
            PEColorSpace::MONO1,
            DROPDOWN_MENU_ARROW_BITMAP_RASTER,
            sizeof(uint32_t),
            0,
            target.GetApplication());
    }
    const PRect buttonFrame = GetDropdownMenuArrowFrame(frame);
    target.SetBackgroundBrush(PStandardColorID::DefaultBackground);
    target.DrawFrame(buttonFrame, data.IsOpen ? FRAME_RECESSED : FRAME_RAISED);
    const PPoint center = buttonFrame.Center().GetRounded();
    const PRect arrowFrame = PRect::FromSize(float(DROPDOWN_MENU_ARROW_WIDTH), float(DROPDOWN_MENU_ARROW_HEIGHT));
    target.SetDrawingMode(PDrawingMode::Overlay);
    if (data.IsEnabled)
    {
        target.SetFgColor(0, 0, 0);
        target.DrawBitmap(state.ArrowBitmap, arrowFrame, center - PPoint(9.0f, 4.0f));
    }
    else
    {
        target.SetFgColor(255, 255, 255);
        target.DrawBitmap(state.ArrowBitmap, arrowFrame, center - PPoint(8.0f, 4.0f));
        target.SetFgColor(110, 110, 110);
        target.DrawBitmap(state.ArrowBitmap, arrowFrame, center - PPoint(9.0f, 5.0f));
    }
    target.SetDrawingMode(PDrawingMode::Copy);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuGetPreferredSize(const PView& target, const PDropdownMenuStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    UpdateDropdownMenuMetrics(target, data);
    const DropdownMenuStyleData& state = *data.GetStyleData<DropdownMenuStyleData>();
    PPoint size = data.EditorSize;
    size.x += (data.Items.empty() && data.Text != nullptr) ? target.GetStringWidth(*data.Text) : state.ItemWidth;
    size.x += DROPDOWN_MENU_LABEL_PADDING + std::round(size.y * DROPDOWN_MENU_ARROW_ASPECT_RATIO);
    *minSize = size;
    *maxSize = size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuPropertyChanged(PView& target, PDropdownMenuStyleData& data, PDropdownMenuProperties properties) const
{
    DropdownMenuStyleData& state = data.GetOrCreateStyleData<DropdownMenuStyleData>();
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Font))) {
        state.AreMetricsValid = false;
    }
    if (properties.HasAny(PDropdownMenuProperties(PDropdownMenuProperty::Items, PDropdownMenuProperty::Text, PDropdownMenuProperty::Font))) {
        target.PreferredSizeChanged();
    }
    target.Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::DropdownMenuGetEditorFrame(const PView& target, const PDropdownMenuStyleData& data) const
{
    PRect frame = target.GetBounds();
    frame.right = GetDropdownMenuArrowFrame(frame).left;
    return frame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<const PTextBoxStyle> PDefaultGUIStyle::DropdownMenuGetEditorStyle(const PDropdownMenuStyleData& data) const
{
    return PTextBox::GetDefaultStyle();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PDefaultGUIStyle::DropdownMenuHitTest(const PView& target, const PDropdownMenuStyleData& data, const PPoint& position) const
{
    const PRect hitFrame = data.IsReadOnly ? target.GetBounds() : GetDropdownMenuArrowFrame(target.GetBounds());
    return data.IsEnabled && hitFrame.DoIntersect(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuPopupFramePaint(PView& target, const PRect& frame) const
{
    target.SetBackgroundBrush(PStandardColorID::DropdownMenuBackground);
    target.DrawFrame(frame, FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuPopupFrameGetPreferredSize(const PView& target, const PPoint& contentSize, PPoint* minSize, PPoint* maxSize) const
{
    *minSize = contentSize + PPoint(DROPDOWN_MENU_POPUP_BORDER_X * 2.0f, DROPDOWN_MENU_POPUP_BORDER_Y * 2.0f);
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::DropdownMenuPopupFrameGetContentFrame(const PView& target) const
{
    const PRect bounds = target.GetBounds();
    const float horizontalBorder = std::min(DROPDOWN_MENU_POPUP_BORDER_X, std::max(0.0f, bounds.Width() * 0.5f));
    const float verticalBorder = std::min(DROPDOWN_MENU_POPUP_BORDER_Y, std::max(0.0f, bounds.Height() * 0.5f));
    return bounds.GetResized(horizontalBorder, verticalBorder, -horizontalBorder, -verticalBorder);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuPopupPaint(PView& target, const PRect& updateRect, const PDropdownMenuPopupStyleData& data) const
{
    UpdateDropdownMenuPopupMetrics(target, data);
    DropdownMenuPopupStyleData& state = *data.GetStyleData<DropdownMenuPopupStyleData>();
    target.SetBackgroundBrush(PStandardColorID::DropdownMenuBackground);
    const PRect bounds = target.GetBounds();
    if (bounds.top < 0.0f) {
        target.EraseRect(PRect(bounds.left, bounds.top, bounds.right, std::min(bounds.bottom, 0.0f)));
    }
    if (bounds.bottom > state.ContentSize.y) {
        target.EraseRect(PRect(bounds.left, std::max(bounds.top, state.ContentSize.y), bounds.right, bounds.bottom));
    }
    if (state.ItemHeight > 0.0f)
    {
        const size_t firstVisible = size_t(std::max(0.0f, std::floor(updateRect.top / state.ItemHeight)));
        const size_t endVisible = std::min(size_t(std::max(0.0f, std::ceil(updateRect.bottom / state.ItemHeight))), data.Items.size());
        for (size_t i = firstVisible; i < endVisible; ++i)
        {
            if (i == data.Selection || i == data.PressedItem)
            {
                target.SetFgColor(255, 255, 255);
                target.SetBrush((i == data.Selection) ? PStandardColorID::DropdownMenuSelection : PStandardColorID::DropdownMenuPressed);
            }
            else
            {
                target.SetFgColor(0, 0, 0);
                target.SetBrush(PStandardColorID::DropdownMenuBackground);
            }
            const PRect itemFrame = DropdownMenuPopupGetItemFrame(target, data, i);
            const PPoint baseline(
                DROPDOWN_MENU_POPUP_LABEL_PADDING,
                std::round(itemFrame.top + state.FontHeight.ascender + state.FontHeight.line_gap * 0.5f));
            target.DrawString(target.GetBrush(), data.Items[i], baseline, itemFrame);
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressedItem = data.PressedItem;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuPopupGetPreferredSize(
    const PView& target,
    const PDropdownMenuPopupStyleData& data,
    PPoint* minSize,
    PPoint* maxSize) const
{
    UpdateDropdownMenuPopupMetrics(target, data);
    *minSize = data.GetStyleData<DropdownMenuPopupStyleData>()->ContentSize;
    *maxSize = *minSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DropdownMenuPopupPropertyChanged(
    PView& target,
    PDropdownMenuPopupStyleData& data,
    PDropdownMenuPopupProperties properties) const
{
    DropdownMenuPopupStyleData& state = data.GetOrCreateStyleData<DropdownMenuPopupStyleData>();
    const PDropdownMenuPopupProperties metricProperties(PDropdownMenuPopupProperty::Items, PDropdownMenuPopupProperty::Font);
    if (properties.HasAny(metricProperties))
    {
        state.AreMetricsValid = false;
        target.ContentSizeChanged();
        target.PreferredSizeChanged();
    }
    const PDropdownMenuPopupProperties incrementalProperties(PDropdownMenuPopupProperty::Selection, PDropdownMenuPopupProperty::Pressed);
    const bool propertiesRequireFullUpdate = !(properties & ~incrementalProperties).Empty();
    if (!state.HasVisualState || propertiesRequireFullUpdate)
    {
        target.Invalidate();
    }
    else
    {
        for (size_t index : {state.PreviousSelection, state.PreviousPressedItem, data.Selection, data.PressedItem})
        {
            if (index < data.Items.size()) {
                target.Invalidate(DropdownMenuPopupGetItemFrame(target, data, index));
            }
        }
    }
    state.PreviousSelection = data.Selection;
    state.PreviousPressedItem = data.PressedItem;
    state.HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::DropdownMenuPopupGetItemFrame(const PView& target, const PDropdownMenuPopupStyleData& data, size_t index) const
{
    if (index >= data.Items.size()) {
        return PRect();
    }
    UpdateDropdownMenuPopupMetrics(target, data);
    const float itemHeight = data.GetStyleData<DropdownMenuPopupStyleData>()->ItemHeight;
    const PRect bounds = target.GetBounds();
    return PRect(bounds.left, float(index) * itemHeight, bounds.right, float(index + 1) * itemHeight);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PDefaultGUIStyle::DropdownMenuPopupGetItemFromPosition(
    const PView& target,
    const PDropdownMenuPopupStyleData& data,
    const PPoint& position) const
{
    if (data.Items.empty()) {
        return INVALID_INDEX;
    }
    UpdateDropdownMenuPopupMetrics(target, data);
    const float itemHeight = data.GetStyleData<DropdownMenuPopupStyleData>()->ItemHeight;
    if (itemHeight <= 0.0f) {
        return INVALID_INDEX;
    }
    const float index = std::clamp(std::floor(position.y / itemHeight), 0.0f, float(data.Items.size() - 1));
    return size_t(index);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::GroupViewPaint(PView& target, const PRect& updateRect, const PGroupViewStyleData& data) const
{
    UpdateGroupViewMetrics(target, data);
    const GroupViewStyleData& state = *data.GetStyleData<GroupViewStyleData>();
    if (!data.Label.empty())
    {
        target.SetBrush(nullptr);
        target.SetFgColor(PStandardColorID::Shadow);
        target.DrawString(data.Label, PPoint(GROUP_VIEW_LABEL_OFFSET, state.LabelAscender), target.GetBounds());
    }
    else
    {
        target.EraseRect(updateRect);
    }

    PRect bound = target.GetNormalizedBounds();
    bound.Resize(0.0f, std::floor(state.LabelSize.y * 0.5f), -1.0f, -1.0f);

    const float oldPenWidth = target.GetPenWidth();
    target.SetPenWidth(GROUP_VIEW_PEN_WIDTH);
    bound.Resize(1.0f, 1.0f, -1.0f, -1.0f);

    target.SetFgColor(PStandardColorID::Shadow);
    target.MovePenTo(bound.BottomLeft());
    target.DrawLine(bound.TopLeft());

    if (!data.Label.empty())
    {
        target.DrawLine(GROUP_VIEW_LABEL_GAP_START, bound.top);
        target.MovePenTo(GROUP_VIEW_LABEL_OFFSET + state.LabelSize.x + GROUP_VIEW_LABEL_SPACING, bound.top);
    }
    target.DrawLine(bound.TopRight());

    target.SetFgColor(PStandardColorID::Shine);
    target.MovePenTo(bound.TopRight() + PPoint(0.0f, GROUP_VIEW_PEN_WIDTH));
    target.DrawLine(bound.BottomRight());
    target.DrawLine(bound.BottomLeft() + PPoint(GROUP_VIEW_PEN_WIDTH, 0.0f));

    target.SetPenWidth(oldPenWidth);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::GroupViewGetPreferredSize(const PView& target, const PGroupViewStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    UpdateGroupViewMetrics(target, data);
    const GroupViewStyleData& state = *data.GetStyleData<GroupViewStyleData>();
    *minSize = PPoint(GROUP_VIEW_MIN_SIZE);
    if (!data.Label.empty())
    {
        minSize->x = GROUP_VIEW_LABEL_OFFSET + state.LabelSize.x + GROUP_VIEW_LABEL_SPACING + GROUP_VIEW_PEN_WIDTH;
        minSize->y = std::max(GROUP_VIEW_MIN_SIZE, state.LabelSize.y + GROUP_VIEW_PEN_WIDTH);
    }
    *maxSize = PPoint(LAYOUT_MAX_SIZE);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::GroupViewPropertyChanged(PView& target, PGroupViewStyleData& data, PGroupViewProperties properties) const
{
    if (properties.HasAny(PGroupViewProperties(PGroupViewProperty::Label, PGroupViewProperty::Font)))
    {
        data.GetOrCreateStyleData<GroupViewStyleData>().AreMetricsValid = false;
        target.PreferredSizeChanged();
    }
    target.Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ProgressBarPaint(PView& target, const PRect& frame, const PProgressBarStyleData& data) const
{
    const PRect bounds = frame.GetFloored();
    target.DrawFrame(bounds, FRAME_RECESSED | FRAME_TRANSPARENT);

    const PRect trackFrame = bounds.GetResized(PROGRESS_BAR_BORDER, PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER);
    if (trackFrame.IsValid())
    {
        const float trackLength = (data.Orientation == POrientation::Horizontal) ? trackFrame.Width() : trackFrame.Height();
        const float barLength = std::ceil(trackLength * std::clamp(data.Value, 0.0f, 1.0f));
        PRect barFrame = trackFrame;
        PRect restFrame = trackFrame;
        if (data.Orientation == POrientation::Horizontal)
        {
            barFrame.right = barFrame.left + barLength;
            restFrame.left = barFrame.right;
        }
        else
        {
            barFrame.top = barFrame.bottom - barLength;
            restFrame.bottom = barFrame.top;
        }
        if (barFrame.IsValid()) {
            target.FillRect(barFrame, PColor(0x66, 0x88, 0xbb));
        }
        if (restFrame.IsValid()) {
            target.EraseRect(restFrame);
        }
    }
    data.GetOrCreateStyleData<ProgressBarStyleData>().PreviousFrame = frame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ProgressBarGetPreferredSize(const PView& target, const PProgressBarStyleData& data, PPoint* minSize, PPoint* maxSize) const
{
    if (data.Orientation == POrientation::Horizontal)
    {
        *minSize = PPoint(PROGRESS_BAR_MIN_LENGTH, PROGRESS_BAR_THICKNESS);
        *maxSize = PPoint(LAYOUT_MAX_SIZE, PROGRESS_BAR_THICKNESS);
    }
    else
    {
        *minSize = PPoint(PROGRESS_BAR_THICKNESS, PROGRESS_BAR_MIN_LENGTH);
        *maxSize = PPoint(PROGRESS_BAR_THICKNESS, LAYOUT_MAX_SIZE);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::ProgressBarPropertyChanged(PView& target, PProgressBarStyleData& data, PProgressBarProperties properties) const
{
    ProgressBarStyleData& state = data.GetOrCreateStyleData<ProgressBarStyleData>();
    if (properties.Has(PProgressBarProperty::Font)) {
        target.PreferredSizeChanged();
    }
    if (properties == PProgressBarProperties(PProgressBarProperty::Frame) && state.PreviousFrame.IsValid()) {
        InvalidateProgressBarResize(target, data, state.PreviousFrame);
    } else {
        target.Invalidate();
    }
    state.PreviousFrame = target.GetNormalizedBounds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::TabViewPaint(PView& target, const PView& tabsTarget, const PRect& updateRect, const PTabViewStyleData& data) const
{
    const TabViewGeometry geometry = GetTabViewGeometry(target, data);
    PRect bounds = geometry.Bounds;
    bounds.top += geometry.TabHeight - TAB_VIEW_BORDER;
    DrawTabViewBorder(target, bounds);
    DrawTabViewSelection(target, tabsTarget, data, geometry);
    if (geometry.Overflow) {
        DrawTabViewArrows(target, geometry);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::TabViewTabsPaint(PView& target, const PView& tabView, const PRect& updateRect, const PTabViewStyleData& data) const
{
    const TabViewGeometry geometry = GetTabViewGeometry(tabView, data);
    DrawTabViewTitles(target, data, geometry);
    DrawTabViewTabs(target, data, geometry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::TabViewGetPreferredSize(
    const PView& target,
    const PTabViewStyleData& data,
    const PPoint& contentMinSize,
    const PPoint& contentMaxSize,
    PPoint* minSize,
    PPoint* maxSize) const
{
    UpdateTabViewMetrics(target, data);
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    const float tabHeight = std::max(state.TabHeight, data.TopBarMinSize.y + TAB_VIEW_BORDER);
    minSize->x = std::max(contentMinSize.x, state.TotalTabsWidth + data.TopBarMinSize.x) + 5.0f;
    minSize->y = contentMinSize.y + tabHeight + 3.0f;
    maxSize->x = std::max(contentMaxSize.x, state.TotalTabsWidth + data.TopBarMaxSize.x) + 5.0f;
    maxSize->y = contentMaxSize.y + tabHeight + 3.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::TabViewPropertyChanged(
    PView& target,
    PView& tabsTarget,
    PTabViewStyleData& data,
    PTabViewProperties properties,
    size_t tabIndex) const
{
    UpdateTabViewTitle(target, data, properties, tabIndex);
    const PTabViewProperties sizeProperties(
        PTabViewProperty::TabInserted,
        PTabViewProperty::TabDeleted,
        PTabViewProperty::Title,
        PTabViewProperty::Font,
        PTabViewProperty::TopBar);
    if (properties.HasAny(sizeProperties)) {
        target.PreferredSizeChanged();
    }
    const TabViewGeometry geometry = GetTabViewGeometry(target, data);
    TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    if (!state.HasVisualState || properties.HasAny(sizeProperties))
    {
        target.Invalidate();
        tabsTarget.Invalidate();
    }
    else
    {
        if (properties.Has(PTabViewProperty::Frame)) {
            InvalidateTabViewResize(target, tabsTarget, data, geometry);
        }
        if (properties.Has(PTabViewProperty::Selection))
        {
            target.Invalidate(PRect(0.0f, 0.0f, geometry.Bounds.right, geometry.TabHeight));
            tabsTarget.Invalidate();
        }
        else if (properties.Has(PTabViewProperty::Scroll))
        {
            target.Invalidate(PRect(0.0f, geometry.TabHeight - TAB_VIEW_BORDER, geometry.TabsRight, geometry.TabHeight));
        }
        if (properties.Has(PTabViewProperty::Highlight))
        {
            target.Invalidate(PRect(0.0f, geometry.TabHeight - TAB_VIEW_BORDER, geometry.TabsRight, geometry.TabHeight));
            tabsTarget.Invalidate();
        }
    }
    // Scroll and highlight changes can occur during layout. Preserve the previous geometry until resize damage has been handled.
    if (properties.HasAny(sizeProperties | PTabViewProperty::Selection | PTabViewProperty::Frame))
    {
        state.PreviousFrame = geometry.Bounds;
        state.PreviousTabsFrame = geometry.Frames.TabsFrame;
        state.PreviousTabsRight = geometry.TabsRight;
        state.PreviousOverflow = geometry.Overflow;
        state.HasVisualState = true;
    }
    if (properties.HasAny(sizeProperties)) {
        tabsTarget.ContentSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabViewFrames PDefaultGUIStyle::TabViewGetFrames(const PView& target, const PTabViewStyleData& data) const
{
    return GetTabViewGeometry(target, data).Frames;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PDefaultGUIStyle::TabViewGetTabsContentSize(const PView& target, const PTabViewStyleData& data) const
{
    const TabViewGeometry geometry = GetTabViewGeometry(target, data);
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    // With no overflow, the parent paints the outer borders outside the strip's viewport.
    const float width = geometry.Overflow ? state.TotalTabsWidth : std::max(0.0f, state.TotalTabsWidth - TAB_VIEW_BORDER * 2.0f);
    return PPoint(width, geometry.Frames.TabsFrame.Height());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PDefaultGUIStyle::TabViewGetTabFromPosition(
    const PView& target,
    const PView& tabsTarget,
    const PTabViewStyleData& data,
    const PPoint& position) const
{
    const TabViewGeometry geometry = GetTabViewGeometry(target, data);
    if (!tabsTarget.GetBounds().DoIntersect(position)) {
        return INVALID_INDEX;
    }
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    float tabLeft = geometry.Overflow ? 0.0f : -TAB_VIEW_BORDER;
    for (size_t index = 0; index < state.TabWidths.size(); ++index)
    {
        const float tabRight = tabLeft + state.TabWidths[index];
        if (position.x >= tabLeft && position.x < tabRight) {
            return index;
        }
        tabLeft = tabRight;
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::DrawRadioButtonLabel(PView& target, const PRect& frame, PButtonStyleData& data)
{
    PRect buttonFrame = frame;
    if (!data.Label.empty())
    {
        data.UpdateMetrics(target);
        const PFontHeight fontHeight = target.GetFontHeight();
        const float labelWidth = data.LabelSize.x;
        const PAlignment labelAlignment = data.LabelAlignment;
        if (labelAlignment == PAlignment::Top || labelAlignment == PAlignment::Bottom)
        {
            const float labelLeft = std::floor((frame.Width() - labelWidth) * 0.5f);
            if (labelAlignment == PAlignment::Top)
            {
                target.MovePenTo(labelLeft, fontHeight.ascender);
                buttonFrame.top += fontHeight.ascender + fontHeight.descender + RADIO_BUTTON_LABEL_VERTICAL_SPACING;
            }
            else
            {
                target.MovePenTo(labelLeft, RADIO_BUTTON_SIZE + RADIO_BUTTON_LABEL_VERTICAL_SPACING + fontHeight.ascender);
                buttonFrame.bottom -= fontHeight.ascender + fontHeight.descender + RADIO_BUTTON_LABEL_VERTICAL_SPACING;
            }
        }
        else
        {
            const float baseline = std::floor(frame.Height() * 0.5f - (fontHeight.ascender + fontHeight.descender) * 0.5f + fontHeight.ascender);
            if (labelAlignment == PAlignment::Left)
            {
                target.MovePenTo(0.0f, baseline);
                buttonFrame.left += labelWidth + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING;
            }
            else
            {
                target.MovePenTo(RADIO_BUTTON_SIZE + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING, baseline);
                buttonFrame.right -= labelWidth + RADIO_BUTTON_LABEL_HORIZONTAL_SPACING;
            }
        }
        target.SetFgColor(0, 0, 0);
        target.SetBrush(PStandardColorID::DefaultBackground);
        target.DrawString(target.GetBrush(), data.Label, target.GetPenPosition(), frame);
    }
    else
    {
        target.FillRect(frame, PStandardColorID::DefaultBackground);
    }
    return buttonFrame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PSliderStyleLayout& PDefaultGUIStyle::GetSliderLayout(const PSliderStyleData& data)
{
    PSliderStyleLayout& layout = data.GetOrCreateStyleData<SliderStyleData>().Layout;
    layout.SetKnobBounds(GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame));
    return layout;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::GetSliderKnobBounds(const PSliderStyleData& data, POrientation orientation, SliderKnobFrameMode mode)
{
    PRect solidFrame(-7.0f, -10.0f, 8.0f, 11.0f);

    if (data.KnobPointUp || data.KnobPointDown)
    {
        solidFrame.top      += 4.0f;
        solidFrame.bottom   -= 4.0f;
    }
    if (data.KnobPointUp)
    {
        if (mode == SliderKnobFrameMode::FullFrame) {
            solidFrame.top -= 10;
        } else {
            solidFrame.top -= 2;
        }
    }
    if (data.KnobPointDown)
    {
        if (mode == SliderKnobFrameMode::FullFrame) {
            solidFrame.bottom += 10;
        } else {
            solidFrame.bottom += 2;
        }
    }
    if (orientation == POrientation::Vertical)
    {
        std::swap(solidFrame.left, solidFrame.top);
        std::swap(solidFrame.right, solidFrame.bottom);
    }
    return solidFrame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawSliderTrack(PView& target, const PRect& sliderFrame, const PRect& knobFrame, const PSliderStyleData& data)
{
    if (sliderFrame.IsValid())
    {
        const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
        const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);

        if (data.Orientation == POrientation::Horizontal)
        {
            PRect leftSliderFrame(sliderFrame.left, sliderFrame.top, knobFrame.left, sliderFrame.bottom);
            PRect rightSliderFrame(knobFrame.right - 1.0f, sliderFrame.top, sliderFrame.right, sliderFrame.bottom);
            if (leftSliderFrame.IsValid())
            {
                PRect leftSliderCenter = leftSliderFrame;
                leftSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                leftSliderCenter.Resize(1.0f, 1.0f, 0.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(leftSliderFrame.BottomLeft());
                target.DrawLine(leftSliderFrame.TopLeft());
                target.DrawLine(leftSliderFrame.TopRight());

                target.SetFgColor(shineColor);
                target.MovePenTo(leftSliderFrame.BottomRight());
                target.DrawLine(PPoint(leftSliderFrame.left + 1, leftSliderFrame.bottom));

                target.FillRect(leftSliderCenter, data.SliderColor1);
            }
            if (rightSliderFrame.IsValid())
            {
                PRect rightSliderCenter = rightSliderFrame;
                rightSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                rightSliderCenter.Resize(0.0f, 1.0f, -1.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(rightSliderFrame.TopLeft());
                target.DrawLine(rightSliderFrame.TopRight());

                target.SetFgColor(shineColor);
                target.MovePenTo(PPoint(rightSliderFrame.right, rightSliderFrame.top + 1.0f));
                target.DrawLine(rightSliderFrame.BottomRight());
                target.DrawLine(rightSliderFrame.BottomLeft() + PPoint(1.0f, 0.0f));

                target.FillRect(rightSliderCenter, data.SliderColor2);
            }
        }
        else
        {
            PRect topSliderFrame(sliderFrame.left, sliderFrame.top, sliderFrame.right, knobFrame.top);
            PRect bottomSliderFrame(sliderFrame.left, knobFrame.bottom - 1.0f, sliderFrame.right, sliderFrame.bottom);
            if (topSliderFrame.IsValid())
            {
                PRect topSliderCenter = topSliderFrame;
                topSliderCenter.Resize(1.0f, 1.0f, -1.0f, 0.0f);
                topSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(topSliderFrame.BottomLeft());
                target.DrawLine(topSliderFrame.TopLeft());
                target.DrawLine(topSliderFrame.TopRight());

                target.SetFgColor(shineColor);
                target.MovePenTo(topSliderFrame.BottomRight());
                target.DrawLine(PPoint(topSliderFrame.right, topSliderFrame.top + 1.0f));

                target.FillRect(topSliderCenter, data.SliderColor1);
            }
            if (bottomSliderFrame.IsValid())
            {
                PRect bottomSliderCenter = bottomSliderFrame;
                bottomSliderFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
                bottomSliderCenter.Resize(1.0f, 0.0f, -1.0f, -1.0f);

                target.SetFgColor(shadowColor);
                target.MovePenTo(bottomSliderFrame.TopLeft());
                target.DrawLine(bottomSliderFrame.BottomLeft());

                target.SetFgColor(shineColor);
                target.MovePenTo(PPoint(bottomSliderFrame.left + 1.0f, bottomSliderFrame.bottom));
                target.DrawLine(bottomSliderFrame.BottomRight());
                target.DrawLine(bottomSliderFrame.TopRight() + PPoint(0.0f, 1.0f));

                target.FillRect(bottomSliderCenter, data.SliderColor2);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawSliderKnob(
    PView& target,
    const PRect& trackFrame,
    const PSliderStyleData& data,
    PStandardColorID knobColor,
    float relativeValue)
{
    const PPoint knobPosition = PSliderStyleLayout::GetPosition(trackFrame, data.Orientation, relativeValue);
    PRect knobFrame = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::SquareFrame) + knobPosition;

    if (!data.KnobPointUp && !data.KnobPointDown)
    {
        PRect center(knobFrame);
        center.Resize(1.0f, 1.0f, -1.0f, -1.0f);
        knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

        target.SetFgColor(PStandardColorID::Shine);
        target.MovePenTo(knobFrame.BottomLeft());
        target.DrawLine(knobFrame.TopLeft());
        target.DrawLine(knobFrame.TopRight());

        target.SetFgColor(PStandardColorID::Shadow);
        target.DrawLine(knobFrame.BottomRight());
        target.DrawLine(knobFrame.BottomLeft() + PPoint(1.0f, 0.0f));
        target.FillRect(center, knobColor);
    }
    else
    {
        PRect knobFullFrame = GetSliderKnobBounds(data, data.Orientation, SliderKnobFrameMode::FullFrame) + knobPosition;

        knobFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);
        knobFullFrame.Resize(0.0f, 0.0f, -1.0f, -1.0f);

        if (data.Orientation == POrientation::Horizontal) {
            DrawHorizontalSliderKnob(target, knobFrame, knobFullFrame, data, knobColor);
        } else {
            DrawVerticalSliderKnob(target, knobFrame, knobFullFrame, data, knobColor);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawHorizontalSliderKnob(
    PView& target,
    const PRect& knobFrame,
    const PRect& knobFullFrame,
    const PSliderStyleData& data,
    PStandardColorID knobColor)
{
    const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
    const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);
    const float center = std::floor(knobFrame.left + (knobFrame.right - knobFrame.left) * 0.5f);

    target.SetFgColor(knobColor);
    float offset = 0.0f;
    float direction = 1.0f;
    for (float x = knobFrame.left; x <= knobFrame.right; x += 1.0f)
    {
        float y1 = knobFrame.top;
        float y2 = knobFrame.bottom;

        if (data.KnobPointUp)     y1 -= offset;
        if (data.KnobPointDown)   y2 += offset;

        if (data.KnobPointUp) {
            target.EraseRect(PRect(x, knobFullFrame.top, x + 1.0f, y1));
        }
        if (data.KnobPointDown) {
            target.EraseRect(PRect(x, y2 + 1.0f, x + 1.0f, knobFullFrame.bottom + 1.0f));
        }
        target.DrawLine(x, y1, x, y2);
        if (direction > 0.0f && x >= center) {
            direction = -1.0f;
        }
        offset += direction;
    }

    target.SetFgColor(shadowColor);

    if (data.KnobPointDown) {
        target.MovePenTo(center, knobFullFrame.bottom);
    } else {
        target.MovePenTo(knobFrame.BottomLeft());
    }
    target.DrawLine(knobFrame.BottomRight());
    target.DrawLine(knobFrame.TopRight());
    if (data.KnobPointUp) {
        target.DrawLine(center, knobFullFrame.top);
    }
    target.SetFgColor(shineColor);
    target.DrawLine(knobFrame.TopLeft());
    target.DrawLine(knobFrame.BottomLeft());
    if (data.KnobPointDown) {
        target.DrawLine(center, knobFullFrame.bottom);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawVerticalSliderKnob(
    PView& target,
    const PRect& knobFrame,
    const PRect& knobFullFrame,
    const PSliderStyleData& data,
    PStandardColorID knobColor)
{
    const PColor shineColor = pget_standard_color(PStandardColorID::Shine);
    const PColor shadowColor = pget_standard_color(PStandardColorID::Shadow);
    const float center = std::floor(knobFrame.top + (knobFrame.bottom - knobFrame.top) * 0.5f);

    target.SetFgColor(knobColor);
    float offset = 0.0f;
    float direction = 1.0f;
    for (float y = knobFrame.top; y <= knobFrame.bottom; y += 1.0f)
    {
        float x1 = knobFrame.left;
        float x2 = knobFrame.right;

        if (data.KnobPointUp)  x1 -= offset;
        if (data.KnobPointDown) x2 += offset;

        if (data.KnobPointUp) {
            target.EraseRect(PRect(knobFullFrame.left, y, x1, y + 1.0f));
        }
        if (data.KnobPointDown) {
            target.EraseRect(PRect(x2 + 1.0f, y, knobFullFrame.right + 1.0f, y + 1.0f));
        }
        target.DrawLine(x1, y, x2, y);
        if (direction > 0.0f && y >= center) {
            direction = -1.0f;
        }
        offset += direction;
    }

    target.SetFgColor(shineColor);
    if (data.KnobPointUp) {
        target.MovePenTo(knobFullFrame.left, center);
    } else {
        target.MovePenTo(knobFrame.BottomLeft());
    }
    target.DrawLine(knobFrame.TopLeft());
    target.DrawLine(knobFrame.TopRight());
    if (data.KnobPointDown) {
        target.DrawLine(knobFullFrame.right, center);
    }
    target.SetFgColor(shadowColor);
    target.DrawLine(knobFrame.BottomRight());
    target.DrawLine(knobFrame.BottomLeft());
    if (data.KnobPointUp) {
        target.DrawLine(knobFullFrame.left, center);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDefaultGUIStyle::ScrollBarGeometry PDefaultGUIStyle::GetScrollBarGeometry(const PRect& frame, const PScrollBarStyleData& data, float relativeValue)
{
    ScrollBarGeometry geometry;
    if (!frame.IsValid()) {
        return geometry;
    }
    const bool  isHorizontal    = data.Orientation == POrientation::Horizontal;
    const float start           = isHorizontal ? frame.left : frame.top;
    const float end             = isHorizontal ? frame.right : frame.bottom;
    const float crossStart      = isHorizontal ? frame.top : frame.left;
    const float crossEnd        = isHorizontal ? frame.bottom : frame.right;
    const float arrowLength     = std::ceil((crossEnd - crossStart) * 0.7f);
    float       trackStart      = start + arrowLength;
    float       trackEnd        = end - arrowLength;

    if (trackStart > trackEnd)
    {
        trackStart = start + std::floor((end - start) * 0.5f);
        trackEnd = trackStart;
    }
    geometry.ArrowFrames[0] = GetScrollBarFrame(data.Orientation, start, trackStart, crossStart, crossEnd);
    geometry.ArrowFrames[3] = GetScrollBarFrame(data.Orientation, trackEnd, end, crossStart, crossEnd);
    if (trackEnd - trackStart >= arrowLength * 2.0f + 15.0f)
    {
        geometry.ArrowFrames[1] = GetScrollBarFrame(data.Orientation, trackStart, trackStart + arrowLength, crossStart, crossEnd);
        geometry.ArrowFrames[2] = GetScrollBarFrame(data.Orientation, trackEnd - arrowLength, trackEnd, crossStart, crossEnd);
        trackStart += arrowLength;
        trackEnd -= arrowLength;
    }
    geometry.TrackFrame = GetScrollBarFrame(data.Orientation, trackStart, trackEnd, crossStart + 1.0f, crossEnd - 1.0f);
    if (geometry.TrackFrame.IsValid())
    {
        const float trackLength         = trackEnd - trackStart;
        const float proportionalSize    = (trackLength + 1.0f) * std::clamp(data.Proportion, 0.0f, 1.0f);
        const float knobLength          = data.HasRange ? std::min(trackLength, std::max(SCROLL_BAR_MIN_KNOB_SIZE, proportionalSize) - 1.0f) : trackLength;
        const float knobStart           = trackStart + (trackLength - knobLength) * std::clamp(relativeValue, 0.0f, 1.0f);

        geometry.KnobFrame = GetScrollBarFrame(data.Orientation, knobStart, knobStart + knobLength, crossStart + 1.0f, crossEnd - 1.0f);
    }
    return geometry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::GetScrollBarFrame(POrientation orientation, float start, float end, float crossStart, float crossEnd)
{
    return (orientation == POrientation::Horizontal) ? PRect(start, crossStart, end, crossEnd) : PRect(crossStart, start, crossEnd, end);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollBarPart PDefaultGUIStyle::GetScrollBarArrowPart(size_t index)
{
    switch (index)
    {
        case 0: return PScrollBarPart::DecrementStart;
        case 1: return PScrollBarPart::IncrementStart;
        case 2: return PScrollBarPart::DecrementEnd;
        case 3: return PScrollBarPart::IncrementEnd;
        default: return PScrollBarPart::None;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PDefaultGUIStyle::GetScrollBarTint(PColor color, float tint)
{
    const int red = std::clamp(int(float(color.GetRed()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    const int green = std::clamp(int(float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    const int blue = std::clamp(int(float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)), 0, 255);
    return PColor(uint8_t(red), uint8_t(green), uint8_t(blue), color.GetAlpha());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawScrollBarKnob(PView& target, const PRect& knobFrame, POrientation orientation)
{
    if (!knobFrame.IsValid()) {
        return;
    }
    target.SetBackgroundBrush(PStandardColorID::ScrollBarKnob);
    target.DrawFrame(knobFrame, FRAME_RAISED);
    const bool isHorizontal = orientation == POrientation::Horizontal;
    const float length = isHorizontal ? knobFrame.Width() : knobFrame.Height();
    if (length > 30.0f)
    {
        float center = (isHorizontal ? knobFrame.left : knobFrame.top) + (length + 1.0f) * 0.5f;
        for (PStandardColorID color : {PStandardColorID::Shine, PStandardColorID::Shadow})
        {
            target.SetFgColor(color);
            for (int i = -1; i <= 1; ++i)
            {
                const float position = center + float(i) * 5.0f;
                if (isHorizontal) {
                    target.DrawLine(PPoint(position, knobFrame.top + 4.0f), PPoint(position, knobFrame.bottom - 4.0f));
                } else {
                    target.DrawLine(PPoint(knobFrame.left + 4.0f, position), PPoint(knobFrame.right - 4.0f, position));
                }
            }
            center -= 1.0f;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawScrollBarTrack(PView& target, const ScrollBarGeometry& geometry, POrientation orientation)
{
    const PRect& trackFrame = geometry.TrackFrame;
    if (!trackFrame.IsValid()) {
        return;
    }
    const bool isHorizontal = orientation == POrientation::Horizontal;
    PRect beforeKnob = trackFrame;
    PRect afterKnob = trackFrame;
    if (isHorizontal)
    {
        beforeKnob.right = geometry.KnobFrame.left;
        afterKnob.left = geometry.KnobFrame.right;
    }
    else
    {
        beforeKnob.bottom = geometry.KnobFrame.top;
        afterKnob.top = geometry.KnobFrame.bottom;
    }
    if (beforeKnob.IsValid()) {
        target.FillRect(beforeKnob, PStandardColorID::ScrollBarBackground);
    }
    if (afterKnob.IsValid()) {
        target.FillRect(afterKnob, PStandardColorID::ScrollBarBackground);
    }
    target.SetFgColor(GetScrollBarTint(pget_standard_color(PStandardColorID::Shadow), 0.5f));
    if (isHorizontal) {
        target.DrawLine(PPoint(trackFrame.left, trackFrame.top - 1.0f), PPoint(trackFrame.right, trackFrame.top - 1.0f));
    } else {
        target.DrawLine(PPoint(trackFrame.left - 1.0f, trackFrame.top), PPoint(trackFrame.left - 1.0f, trackFrame.bottom));
    }
    target.SetFgColor(GetScrollBarTint(pget_standard_color(PStandardColorID::Shine), 0.6f));
    if (isHorizontal) {
        target.DrawLine(PPoint(trackFrame.left, trackFrame.bottom), PPoint(trackFrame.right, trackFrame.bottom));
    } else {
        target.DrawLine(PPoint(trackFrame.right, trackFrame.top), PPoint(trackFrame.right, trackFrame.bottom));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawScrollBarArrows(PView& target, const ScrollBarGeometry& geometry, const PScrollBarStyleData& data)
{
    ScrollBarStyleData& state = data.GetOrCreateStyleData<ScrollBarStyleData>();
    if (state.DecrementArrowBitmap == nullptr)
    {
        const bool isHorizontal = data.Orientation == POrientation::Horizontal;
        const int width = isHorizontal ? 4 : 7;
        const int height = isHorizontal ? 7 : 4;
        const uint32_t* decrementRaster = isHorizontal ? SCROLL_BAR_LEFT_ARROW_BITMAP_RASTER : SCROLL_BAR_UP_ARROW_BITMAP_RASTER;
        const uint32_t* incrementRaster = isHorizontal ? SCROLL_BAR_RIGHT_ARROW_BITMAP_RASTER : SCROLL_BAR_DOWN_ARROW_BITMAP_RASTER;
        state.DecrementArrowBitmap = ptr_new<PBitmap>(width, height, PEColorSpace::MONO1, decrementRaster, sizeof(uint32_t));
        state.IncrementArrowBitmap = ptr_new<PBitmap>(width, height, PEColorSpace::MONO1, incrementRaster, sizeof(uint32_t));
    }
    target.SetBackgroundBrush(PStandardColorID::ScrollBarKnob);
    for (size_t i = 0; i < std::size(geometry.ArrowFrames); ++i)
    {
        const PRect& arrowFrame = geometry.ArrowFrames[i];
        if (arrowFrame.IsValid())
        {
            const Ptr<PBitmap>& arrowBitmap = (i & 0x01) ? state.IncrementArrowBitmap : state.DecrementArrowBitmap;
            const PRect bitmapBounds(arrowBitmap->GetBounds());
            const PPoint arrowPosition = arrowFrame.TopLeft() + (arrowFrame.Size() - bitmapBounds.Size()) * 0.5f;
            const bool isPressed = data.PressedPart == GetScrollBarArrowPart(i);
            target.DrawFrame(arrowFrame, (isPressed ? FRAME_RECESSED : FRAME_RAISED) | FRAME_THIN);
            target.SetDrawingMode(PDrawingMode::Overlay);
            target.SetFgColor(0, 0, 0);
            target.DrawBitmap(arrowBitmap, bitmapBounds, arrowPosition);
            target.SetDrawingMode(PDrawingMode::Copy);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateDropdownMenuMetrics(const PView& target, const PDropdownMenuStyleData& data)
{
    DropdownMenuStyleData& state = data.GetOrCreateStyleData<DropdownMenuStyleData>();
    if (!state.AreMetricsValid)
    {
        state.ItemWidth = 0.0f;
        for (const PString& item : data.Items) {
            state.ItemWidth = std::max(state.ItemWidth, target.GetStringWidth(item));
        }
        state.AreMetricsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PDefaultGUIStyle::GetDropdownMenuArrowFrame(const PRect& frame)
{
    const float width = std::round(std::max(0.0f, frame.Height()) * DROPDOWN_MENU_ARROW_ASPECT_RATIO);
    return PRect(std::max(frame.left, frame.right - width), frame.top, frame.right, frame.bottom);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateDropdownMenuPopupMetrics(const PView& target, const PDropdownMenuPopupStyleData& data)
{
    DropdownMenuPopupStyleData& state = data.GetOrCreateStyleData<DropdownMenuPopupStyleData>();
    if (!state.AreMetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        state.ItemHeight = std::round(state.FontHeight.descender + state.FontHeight.ascender + state.FontHeight.line_gap);
        state.ContentSize = PPoint(0.0f, float(data.Items.size()) * state.ItemHeight);
        for (const PString& item : data.Items) {
            state.ContentSize.x = std::max(state.ContentSize.x, target.GetStringWidth(item));
        }
        state.ContentSize.x += DROPDOWN_MENU_POPUP_LABEL_PADDING * 2.0f;
        state.AreMetricsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateGroupViewMetrics(const PView& target, const PGroupViewStyleData& data)
{
    GroupViewStyleData& state = data.GetOrCreateStyleData<GroupViewStyleData>();
    if (!state.AreMetricsValid)
    {
        state.LabelSize = PPoint();
        state.LabelAscender = 0.0f;
        if (!data.Label.empty())
        {
            const PFontHeight fontHeight = target.GetFontHeight();
            state.LabelSize = PPoint(target.GetStringWidth(data.Label), fontHeight.ascender + fontHeight.descender);
            state.LabelAscender = fontHeight.ascender;
        }
        state.AreMetricsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::InvalidateProgressBarResize(PView& target, const PProgressBarStyleData& data, const PRect& previousFrame)
{
    const PRect bounds = target.GetNormalizedBounds();
    const PPoint delta = bounds.Size() - previousFrame.Size();
    if ((data.Orientation == POrientation::Horizontal && delta.x != 0.0f) || (data.Orientation == POrientation::Vertical && delta.y != 0.0f)) {
        target.Invalidate(bounds.GetResized(PROGRESS_BAR_BORDER, PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER, -PROGRESS_BAR_BORDER));
    }
    if (delta.x != 0.0f)
    {
        PRect damage = bounds;
        damage.left = damage.right - std::max(PROGRESS_BAR_BORDER, delta.x + PROGRESS_BAR_BORDER);
        target.Invalidate(damage);
    }
    if (delta.y != 0.0f)
    {
        PRect damage = bounds;
        damage.top = damage.bottom - std::max(PROGRESS_BAR_BORDER, delta.y + PROGRESS_BAR_BORDER);
        target.Invalidate(damage);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateTabViewMetrics(const PView& target, const PTabViewStyleData& data)
{
    TabViewStyleData& state = data.GetOrCreateStyleData<TabViewStyleData>();
    if (!state.AreMetricsValid)
    {
        state.FontHeight = target.GetFontHeight();
        state.GlyphHeight = state.FontHeight.ascender + state.FontHeight.descender + state.FontHeight.line_gap;
        state.TabHeight = std::round(state.GlyphHeight * 1.2f + 6.0f);
        state.TotalTabsWidth = TAB_VIEW_TITLE_PADDING;
        state.TabWidths.resize(data.Titles.size());
        for (size_t index = 0; index < data.Titles.size(); ++index)
        {
            state.TabWidths[index] = std::round(target.GetStringWidth(data.Titles[index]) * TAB_VIEW_TITLE_SCALE) + TAB_VIEW_TITLE_PADDING;
            state.TotalTabsWidth += state.TabWidths[index];
        }
        state.AreMetricsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::UpdateTabViewTitle(const PView& target, const PTabViewStyleData& data, PTabViewProperties properties, size_t tabIndex)
{
    TabViewStyleData& state = data.GetOrCreateStyleData<TabViewStyleData>();
    if (properties.Has(PTabViewProperty::Font)) {
        state.AreMetricsValid = false;
    }
    if (state.AreMetricsValid)
    {
        if (properties.Has(PTabViewProperty::TabInserted))
        {
            const float width = std::round(target.GetStringWidth(data.Titles[tabIndex]) * TAB_VIEW_TITLE_SCALE) + TAB_VIEW_TITLE_PADDING;
            state.TabWidths.insert(state.TabWidths.begin() + tabIndex, width);
            state.TotalTabsWidth += width;
        }
        else if (properties.Has(PTabViewProperty::TabDeleted))
        {
            state.TotalTabsWidth -= state.TabWidths[tabIndex];
            state.TabWidths.erase(state.TabWidths.begin() + tabIndex);
        }
        else if (properties.Has(PTabViewProperty::Title))
        {
            const float width = std::round(target.GetStringWidth(data.Titles[tabIndex]) * TAB_VIEW_TITLE_SCALE) + TAB_VIEW_TITLE_PADDING;
            state.TotalTabsWidth += width - state.TabWidths[tabIndex];
            state.TabWidths[tabIndex] = width;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDefaultGUIStyle::TabViewGeometry PDefaultGUIStyle::GetTabViewGeometry(const PView& target, const PTabViewStyleData& data)
{
    UpdateTabViewMetrics(target, data);
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    TabViewGeometry geometry;
    geometry.Bounds = target.GetNormalizedBounds();
    geometry.TabHeight = std::max(state.TabHeight, data.TopBarMinSize.y + TAB_VIEW_BORDER);
    geometry.TabsRight = geometry.Bounds.right;
    const float headerBottom = std::min(geometry.TabHeight - TAB_VIEW_BORDER, geometry.Bounds.bottom);
    if (data.HasTopBarClient)
    {
        const float availableWidth = std::max(0.0f, geometry.Bounds.Width() - TAB_VIEW_BORDER * 2.0f);
        const float extraWidth = availableWidth - state.TotalTabsWidth;
        const float clientWidth = std::min(availableWidth, std::clamp(extraWidth, data.TopBarMinSize.x, data.TopBarMaxSize.x));
        const float clientRight = std::max(0.0f, geometry.Bounds.right - TAB_VIEW_BORDER);
        const float clientHeight = std::min(headerBottom, data.TopBarMaxSize.y);
        geometry.Frames.TopBarClientFrame = PRect(clientRight - clientWidth, 0.0f, clientRight, clientHeight);
        geometry.TabsRight = geometry.Frames.TopBarClientFrame.left;
    }
    geometry.Overflow = state.TotalTabsWidth > geometry.TabsRight;
    const float inset = geometry.Overflow ? TAB_VIEW_ARROW_SPACE : TAB_VIEW_BORDER;
    const float tabsLeft = std::min(inset, geometry.TabsRight);
    const float tabsRight = std::max(tabsLeft, geometry.TabsRight - inset);
    geometry.Frames.TabsFrame = PRect(tabsLeft, 0.0f, tabsRight, headerBottom);
    geometry.HeaderFrame = PRect(0.0f, 0.0f, geometry.TabsRight, headerBottom);
    const float clientLeft = std::min(TAB_VIEW_BORDER, geometry.Bounds.right);
    const float clientTop = std::min(geometry.TabHeight, geometry.Bounds.bottom);
    const float clientRight = std::max(clientLeft, geometry.Bounds.right - TAB_VIEW_BORDER);
    const float clientBottom = std::max(clientTop, geometry.Bounds.bottom - TAB_VIEW_BORDER);
    geometry.Frames.ClientFrame = PRect(clientLeft, clientTop, clientRight, clientBottom);
    return geometry;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::InvalidateTabViewResize(PView& target, PView& tabsTarget, const PTabViewStyleData& data, const TabViewGeometry& geometry)
{
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    const PPoint delta = geometry.Bounds.Size() - state.PreviousFrame.Size();
    if (delta.x != 0.0f)
    {
        PRect damage = geometry.Bounds;
        damage.left = damage.right - std::max(3.0f, delta.x + 9.0f);
        target.Invalidate(damage);
    }
    if (delta.y != 0.0f)
    {
        PRect damage = geometry.Bounds;
        damage.top = damage.bottom - std::max(3.0f, delta.y + TAB_VIEW_BORDER);
        target.Invalidate(damage);
    }
    if (geometry.Overflow != state.PreviousOverflow || geometry.TabsRight != state.PreviousTabsRight) {
        target.Invalidate(PRect(0.0f, 0.0f, geometry.Bounds.right, geometry.TabHeight));
    }
    if (geometry.Overflow != state.PreviousOverflow || geometry.Frames.TabsFrame.TopLeft() != state.PreviousTabsFrame.TopLeft()) {
        tabsTarget.Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PDefaultGUIStyle::GetTabViewTint(PColor color, float tint)
{
    return PColor(uint8_t((float(color.GetRed()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                  uint8_t((float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                  uint8_t((float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                  color.GetAlpha()
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawTabViewBorder(PView& target, const PRect& bounds)
{
    const PDynamicColor fgColor(PStandardColorID::Shine);
    const PDynamicColor bgColor(PStandardColorID::Shadow);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);
    const PColor bgShadowColor = GetTabViewTint(bgColor, 0.8f);

    target.SetFgColor(bgShadowColor);
    target.DrawLine(PPoint(bounds.right - 1.0f, bounds.top + 2.0f), PPoint(bounds.right - 1.0f, bounds.bottom - 1.0f));  // top/right -> bottom/right
    target.DrawLine(PPoint(bounds.left, bounds.bottom - 1.0f));                                                         // bottom/left

    target.SetFgColor(bgColor);
    target.DrawLine(PPoint(bounds.right - 2.0f, bounds.top + 2.0f), PPoint(bounds.right - 2.0f, bounds.bottom - 2.0f));  // top/right -> bottom/right
    target.DrawLine(PPoint(bounds.left + 1.0f, bounds.bottom - 2.0f));                                                  // bottom/left

    target.SetFgColor(fgShadowColor);
    target.DrawLine(PPoint(bounds.left, bounds.top - 1.0f), PPoint(bounds.left, bounds.bottom - 3.0f));                  // top/left -> bottom/left

    target.SetFgColor(fgColor);
    target.DrawLine(PPoint(bounds.left + 1.0f, bounds.top + 1.0f), PPoint(bounds.left + 1.0f, bounds.bottom - 3.0f));    // top/left -> bottom/left
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawTabViewSelection(PView& target, const PView& tabsTarget, const PTabViewStyleData& data, const TabViewGeometry& geometry)
{
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    const PRect bounds = geometry.Bounds;
    const PRect tabRect = geometry.HeaderFrame;
    const PDynamicColor fgColor(PStandardColorID::Shine);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);

    float x = tabsTarget.GetScrollOffset().x;

    if (geometry.Overflow) {
        x += TAB_VIEW_ARROW_SPACE;
    }
    if (!geometry.Overflow)
    {
        target.FillRect(PRect(0.0f, 0.0f, 2.0f, geometry.TabHeight - 2.0f), PStandardColorID::DefaultBackground);
        target.FillRect(PRect(tabRect.right - 2.0f, 0.0f, tabRect.right, geometry.TabHeight - 2.0f), PStandardColorID::DefaultBackground);
    }

    for (size_t i = 0; i < data.Titles.size(); ++i)
    {
        float width = state.TabWidths[i];
        PRect tabFrame(x + 2.0f, 2.0f, x + width + 2.0f, geometry.TabHeight);

        if (i == data.Selection) {
            tabFrame.Resize(-2.0f, -2.0f, 2.0f, 0.0f);
        }

        if (!geometry.Overflow)
        {
            if (i != data.Selection + 1)
            {
                target.SetFgColor(fgShadowColor);
                target.DrawLine(PPoint(tabFrame.left, tabFrame.bottom - 3.0f), PPoint(tabFrame.left, tabFrame.top + 2.0f + TAB_VIEW_CORNER_SIZE));
                target.DrawLine(PPoint(tabFrame.left + 3.0f, tabFrame.top));
                target.SetFgColor(fgColor);
                target.DrawLine(
                    PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f),
                    PPoint(tabFrame.left + 1.0f, tabFrame.top + 2.0f + TAB_VIEW_CORNER_SIZE));
                target.DrawLine(PPoint(tabFrame.left + 1.0f + TAB_VIEW_CORNER_SIZE, tabFrame.top + 1.0f));
            }
        }
        else
        {
            if (i == data.Selection) {
                target.DrawLine(PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f), PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f));
            }
        }
        if (i == data.Selection)
        {
            target.SetFgColor((i == data.HighlightedTab) ? PStandardColorID::ButtonBackgroundHover : PStandardColorID::DefaultBackground);
            float x1 = x + 2.0f;
            float x2 = x + width + 2.0f;

            if (geometry.Overflow)
            {
                if (x1 < TAB_VIEW_ARROW_SPACE) {
                    x1 = TAB_VIEW_ARROW_SPACE;
                }
                if (x2 > geometry.TabsRight - 2.0f - TAB_VIEW_ARROW_SPACE) {
                    x2 = geometry.TabsRight - 2.0f - TAB_VIEW_ARROW_SPACE;
                }
            }
            target.DrawLine(PPoint(x1, tabFrame.bottom - 2.0f), PPoint(x2, tabFrame.bottom - 2.0f));
            target.DrawLine(PPoint(x1, tabFrame.bottom - 1.0f), PPoint(x2, tabFrame.bottom - 1.0f));

            target.SetFgColor(fgShadowColor);

            if (x >= 0.0f) {
                target.DrawLine(PPoint(0, tabFrame.bottom - 2.0f), PPoint(x, tabFrame.bottom - 2.0f));
            }
            target.DrawLine(PPoint(x + width + 2, tabFrame.bottom - 2.0f), PPoint(bounds.right - 1.0f, tabFrame.bottom - 2.0f));

            target.SetFgColor(fgColor);
            if (x >= 1) {
                target.DrawLine(PPoint(1, tabFrame.bottom - 1.0f), PPoint(x + 1.0f, tabFrame.bottom - 1.0f));
            }
            target.DrawLine(PPoint(x + width + 2.0f, tabFrame.bottom - 1.0f), PPoint(bounds.right - 1.0f, tabFrame.bottom - 1.0f));
        }
        x += width;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawTabViewArrows(PView& target, const TabViewGeometry& geometry)
{
    const PRect tabRect = geometry.HeaderFrame;
    const PDynamicColor fgColor(PStandardColorID::Shine);
    const PDynamicColor bgColor(PStandardColorID::Shadow);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);

    target.FillRect(PRect(0.0f, 0.0f, TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 2.0f), PStandardColorID::DefaultBackground);
    target.FillRect(
        PRect(tabRect.right - TAB_VIEW_ARROW_SPACE, 0.0f, tabRect.right, geometry.TabHeight - 2.0f),
        PStandardColorID::DefaultBackground);
    target.SetFgColor(bgColor);
    for (float x = 2.0f; x < TAB_VIEW_ARROW_SPACE - 2.0f; x += 1.0f)
    {
        const float y1 = geometry.TabHeight * 0.5f - (x - 2.0f);
        const float y2 = geometry.TabHeight * 0.5f + (x - 2.0f);
        target.DrawLine(PPoint(x, y1), PPoint(x, y2));
        target.DrawLine(PPoint(geometry.TabsRight - x - 1.0f, y1), PPoint(geometry.TabsRight - x - 1.0f, y2));
    }

    target.SetFgColor(fgShadowColor);
    target.DrawLine(PPoint(0, geometry.TabHeight - 2.0f), PPoint(TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 2.0f));
    target.DrawLine(
        PPoint(geometry.TabsRight - 1.0f - TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 2.0f),
        PPoint(geometry.TabsRight - 1.0f, geometry.TabHeight - 2.0f));
    target.SetFgColor(fgColor);
    target.DrawLine(PPoint(1.0f, geometry.TabHeight - 1.0f), PPoint(TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 1.0f));
    target.DrawLine(
        PPoint(geometry.TabsRight - 1.0f - TAB_VIEW_ARROW_SPACE, geometry.TabHeight - 1.0f),
        PPoint(geometry.TabsRight - 1.0f, geometry.TabHeight - 1.0f));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawTabViewTitles(PView& target, const PTabViewStyleData& data, const TabViewGeometry& geometry)
{
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    const PRect bounds = target.GetBounds();
    const PFontHeight fontHeight = state.FontHeight;
    const float tabHeight = geometry.TabHeight;
    const float glyphHeight = state.GlyphHeight;
    float x = geometry.Overflow ? 0.0f : -TAB_VIEW_BORDER;
    target.SetBrush(PStandardColorID::DefaultBackground);
    target.SetFgColor(0, 0, 0);
    PRect highlightedFrame;
    PPoint highlightedBaseline;
    float backgroundLeft = bounds.left;
    for (size_t i = 0; i < data.Titles.size(); ++i)
    {
        const float width = state.TabWidths[i];
        const float top = (i == data.Selection) ? 0.0f : 2.0f;
        const PPoint baseline(
            std::round(x + (width - 4.0f) * 0.05f + 4.0f),
            std::round(top + tabHeight * 0.5f + fontHeight.ascender - fontHeight.line_gap * 0.5f - glyphHeight * 0.5f + 2.0f));
        const float right = std::clamp(x + width, bounds.left, bounds.right);
        const PRect background(backgroundLeft, bounds.top, right, bounds.bottom);
        if (i == data.HighlightedTab)
        {
            target.FillRect(background, target.GetBrush());
            highlightedFrame = PRect(x + 2.0f, 2.0f, x + width + 2.0f, tabHeight);
            highlightedBaseline = baseline;
        }
        else
        {
            target.DrawString(target.GetBrush(), data.Titles[i], baseline, background);
        }
        backgroundLeft = right;
        x += width;
    }
    if (backgroundLeft < bounds.right) {
        target.FillRect(PRect(backgroundLeft, bounds.top, bounds.right, bounds.bottom), target.GetBrush());
    }
    if (data.HighlightedTab < data.Titles.size()) {
        DrawTabViewHighlight(target, data, highlightedFrame, highlightedBaseline);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawTabViewHighlight(PView& target, const PTabViewStyleData& data, PRect frame, const PPoint& baseline)
{
    target.SetFgColor(PStandardColorID::ButtonBackgroundHover);
    float cornerSize = TAB_VIEW_CORNER_SIZE;
    if (data.HighlightedTab == data.Selection)
    {
        frame.Resize(-TAB_VIEW_BORDER, -TAB_VIEW_BORDER, TAB_VIEW_BORDER, 0.0f);
    }
    else if (data.HighlightedTab + 1 == data.Selection)
    {
        // The preceding tab remains visible underneath the selected tab's chamfer.
        frame.right -= TAB_VIEW_BORDER;
        const float overlap = TAB_VIEW_CORNER_SIZE - TAB_VIEW_BORDER;
        const float cornerOverlap = std::clamp(cornerSize - frame.Width(), 0.0f, overlap);
        const PPoint topLeft = frame.TopRight() + PPoint(cornerOverlap, 0.0f);
        const PPoint bottomLeft = frame.TopRight() + PPoint(0.0f, overlap);
        target.FillTriangle(topLeft, frame.TopRight() + PPoint(overlap, 0.0f), bottomLeft);
        if (cornerOverlap > 0.0f) {
            target.FillTriangle(topLeft, bottomLeft, frame.TopRight() + PPoint(0.0f, cornerOverlap));
        }
    }
    else if (data.Selection != INVALID_INDEX && data.HighlightedTab == data.Selection + 1)
    {
        // The selected tab covers the following tab's left edge.
        frame.left += TAB_VIEW_BORDER;
        cornerSize -= TAB_VIEW_BORDER;
    }
    const float cornerRight = std::min(frame.left + cornerSize, frame.right);
    const float cornerBottom = frame.top + cornerSize;
    const float cornerTop = cornerBottom - (cornerRight - frame.left);
    target.FillRect(PRect(cornerRight, frame.top, frame.right, frame.bottom), PStandardColorID::ButtonBackgroundHover);
    target.FillRect(PRect(frame.left, cornerBottom, cornerRight, frame.bottom), PStandardColorID::ButtonBackgroundHover);
    target.FillTriangle(PPoint(frame.left, cornerBottom), PPoint(cornerRight, cornerTop), PPoint(cornerRight, cornerBottom));

    const PDrawingMode previousMode = target.GetDrawingMode();
    target.SetDrawingMode(PDrawingMode::Overlay);
    target.SetFgColor(0, 0, 0);
    target.DrawString(target.GetBrush(), data.Titles[data.HighlightedTab], baseline, frame.right - baseline.x);
    target.SetDrawingMode(previousMode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDefaultGUIStyle::DrawTabViewTabs(PView& target, const PTabViewStyleData& data, const TabViewGeometry& geometry)
{
    const TabViewStyleData& state = *data.GetStyleData<TabViewStyleData>();
    float x = geometry.Overflow ? 0.0f : -TAB_VIEW_BORDER;
    const PDynamicColor fgColor(PStandardColorID::Shine);
    const PDynamicColor bgColor(PStandardColorID::Shadow);

    const PColor fgShadowColor = GetTabViewTint(fgColor, 0.9f);
    const PColor bgShadowColor = GetTabViewTint(bgColor, 0.8f);

    for (size_t i = 0; i < data.Titles.size(); ++i)
    {
        float width = state.TabWidths[i];
        PRect tabFrame(x + 2.0f, 2.0f, x + width + 2.0f, geometry.TabHeight);

        if (i == data.Selection) {
            tabFrame.Resize(-2.0f, -2.0f, 2.0f, 0.0f);
        }
        target.SetFgColor(fgShadowColor);
        if (i != data.Selection + 1)
        {
            target.DrawLine(PPoint(tabFrame.left, tabFrame.bottom - 3.0f), PPoint(tabFrame.left, tabFrame.top + TAB_VIEW_CORNER_SIZE));
            target.DrawLine(PPoint(tabFrame.left + TAB_VIEW_CORNER_SIZE, tabFrame.top));
            if (i == data.Selection - 1) {
                target.DrawLine(PPoint(tabFrame.right + TAB_VIEW_CORNER_SIZE - 3.0f, tabFrame.top));
            } else {
                target.DrawLine(PPoint(tabFrame.right - 1.0f, tabFrame.top));
            }

            target.SetFgColor(fgColor);

            target.DrawLine(
                PPoint(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f),
                PPoint(tabFrame.left + 1.0f, tabFrame.top + 1.0f + TAB_VIEW_CORNER_SIZE));
            target.DrawLine(PPoint(tabFrame.left + 1.0f + TAB_VIEW_CORNER_SIZE, tabFrame.top + 1.0f));
            if (i == data.Selection - 1) {
                target.DrawLine(PPoint(tabFrame.right + TAB_VIEW_CORNER_SIZE - 4.0f, tabFrame.top + 1.0f));
            } else {
                target.DrawLine(PPoint(tabFrame.right - 2.0f, tabFrame.top + 1.0f));
            }
        }
        else
        {
            // Draw the chamfer from the edge exposed beside the selected tab.
            const float visibleLeft = tabFrame.left + TAB_VIEW_BORDER;
            target.DrawLine(
                PPoint(visibleLeft, tabFrame.top + TAB_VIEW_CORNER_SIZE - TAB_VIEW_BORDER),
                PPoint(tabFrame.left + TAB_VIEW_CORNER_SIZE, tabFrame.top));
            target.DrawLine(PPoint(tabFrame.right - 1.0f, tabFrame.top));
            target.SetFgColor(fgColor);
            target.DrawLine(
                PPoint(visibleLeft, tabFrame.top + TAB_VIEW_CORNER_SIZE),
                PPoint(tabFrame.left + 1.0f + TAB_VIEW_CORNER_SIZE, tabFrame.top + 1.0f));
            target.DrawLine(PPoint(tabFrame.right - 2.0f, tabFrame.top + 1.0f));
        }

        if (i != data.Selection - 1)
        {
            target.SetFgColor(bgShadowColor);
            target.DrawLine(PPoint(tabFrame.right - 1.0f, tabFrame.top + 2.0f), PPoint(tabFrame.right - 1.0f, tabFrame.bottom - 3.0f));

            target.SetFgColor(bgColor);
            target.DrawLine(PPoint(tabFrame.right - 2.0f, tabFrame.top + 2.0f), PPoint(tabFrame.right - 2.0f, tabFrame.bottom - 2.0f));
        }
        x += width;
    }
}
