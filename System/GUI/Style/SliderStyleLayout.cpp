// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <GUI/Region.h>
#include <GUI/Style/SliderStyleLayout.h>
#include <GUI/View.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::GetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize)
{
    PRect totalFrame;
    float minimumLength;
    GetTrackFrame(target, target.GetNormalizedBounds(), data, &totalFrame, &minimumLength);
    if (data.Orientation == POrientation::Horizontal)
    {
        *minSize = PPoint(minimumLength, totalFrame.Height());
        *maxSize = PPoint(2000.0f, minSize->y);
    }
    else
    {
        *minSize = PPoint(totalFrame.Width(), minimumLength);
        *maxSize = PPoint(minSize->x, 2000.0f);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::PropertyChanged(PView& target, const PSliderStyleData& data, PSliderProperties properties)
{
    if (properties.HasAny(PSliderProperties(PSliderProperty::ValueLabel, PSliderProperty::Font))) {
        m_IsValueLabelSizeValid = false;
    }
    if (properties.HasAny(PSliderProperties(PSliderProperty::MinLabel, PSliderProperty::MaxLabel, PSliderProperty::Font))) {
        m_AreLimitLabelWidthsValid = false;
    }
    const bool labelHeightChanged = m_ValueLabelFrame.Height() != GetValueLabelFrame(target, data).Height();
    UpdateDamage(target, data, properties, true);

    const PSliderProperties sizeProperties(
        PSliderProperty::Label,
        PSliderProperty::MinLabel,
        PSliderProperty::MaxLabel,
        PSliderProperty::Font,
        PSliderProperty::SliderSize,
        PSliderProperty::TickCount,
        PSliderProperty::Appearance);
    if (labelHeightChanged || properties.HasAny(sizeProperties)) {
        target.PreferredSizeChanged();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSliderStyleLayout::GetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue)
{
    const PRect trackFrame = GetTrackFrame(target, target.GetNormalizedBounds(), data);
    return GetPosition(trackFrame, data.Orientation, relativeValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSliderStyleLayout::GetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position)
{
    const PRect trackFrame = GetTrackFrame(target, target.GetNormalizedBounds(), data);
    const bool isHorizontal = data.Orientation == POrientation::Horizontal;
    const float length = isHorizontal ? trackFrame.Width() : trackFrame.Height();
    if (length <= 0.0f) {
        return data.Value;
    }
    const float relativeValue = isHorizontal ? (position.x - trackFrame.left) / length : (trackFrame.bottom - position.y) / length;
    return std::clamp(relativeValue, 0.0f, 1.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::ClearBackground(PView& target, const PRect& bounds, const PRect& sliderFrame, const PRect& knobFrame, const PSliderStyleData& data)
{
    // Clear areas not fully overwritten by the slider track and the knob.
    PRegion clearRegion(bounds);
    clearRegion.Exclude(sliderFrame);
    clearRegion.Exclude(knobFrame);
    const PRect labelFrame = GetValueLabelFrame(target, data) + bounds.TopLeft();
    if (labelFrame.IsValid()) {
        clearRegion.Exclude(labelFrame);
    }
    if (data.Orientation == POrientation::Horizontal)
    {
        if (!data.MinLabel.empty() && !data.MaxLabel.empty())
        {
            const float top = GetHorizontalLimitLabelTop(sliderFrame, data);
            clearRegion.Exclude(PRect(bounds.left, top, bounds.right, bounds.bottom));
        }
    }
    else
    {
        const float top = sliderFrame.top + std::min(0.0f, m_LayoutKnobBounds.top) - SLIDER_VERTICAL_LABEL_SPACING;
        const float bottom = sliderFrame.bottom + std::max(0.0f, m_LayoutKnobBounds.bottom) + SLIDER_VERTICAL_LABEL_SPACING;
        if (!data.MaxLabel.empty()) {
            clearRegion.Exclude(PRect(bounds.left, bounds.top, bounds.right, top));
        }
        if (!data.MinLabel.empty()) {
            clearRegion.Exclude(PRect(bounds.left, bottom, bounds.right, bounds.bottom));
        }
    }
    clearRegion.Optimize();
    for (const PIRect& rect : clearRegion.m_Rects) {
        target.EraseRect(rect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::UpdateDamage(PView& target, const PSliderStyleData& data, PSliderProperties properties, bool invalidate)
{
    const PRect trackFrame = GetTrackFrame(target, target.GetNormalizedBounds(), data);
    const PRect& knobBounds = m_LayoutKnobBounds;
    const PRect knobFrame = knobBounds + GetPosition(trackFrame, data.Orientation, data.Value);
    const PRect labelFrame = GetValueLabelFrame(target, data);
    const PSliderProperties incrementalProperties(
        PSliderProperty::Value,
        PSliderProperty::ValueLabel,
        PSliderProperty::ShadowValues,
        PSliderProperty::Pressed,
        PSliderProperty::Hovered);
    const bool propertiesRequireFullUpdate = !(properties & ~incrementalProperties).Empty();
    const bool fullUpdate = !m_HasVisualState || propertiesRequireFullUpdate || labelFrame.Height() != m_ValueLabelFrame.Height();
    if (invalidate)
    {
        if (fullUpdate)
        {
            target.Invalidate();
        }
        else
        {
            if (properties.HasAny(PSliderProperties(PSliderProperty::Value, PSliderProperty::Pressed, PSliderProperty::Hovered))) {
                InvalidateFrames(target, m_KnobFrame, knobFrame);
            }
            if (properties.Has(PSliderProperty::ValueLabel)) {
                InvalidateFrames(target, m_ValueLabelFrame, labelFrame);
            }
        }
    }
    if (fullUpdate || properties.Has(PSliderProperty::ShadowValues)) {
        UpdateShadowDamage(target, data, trackFrame, knobBounds, invalidate && !fullUpdate);
    }

    // Advance only after queuing damage. Repeated changes before a paint then
    // accumulate all affected regions, including any intermediate positions.
    m_KnobFrame = knobFrame;
    m_ValueLabelFrame = labelFrame;
    m_HasVisualState = true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSliderStyleLayout::GetValueLabelFrame(const PView& target, const PSliderStyleData& data)
{
    if (data.Orientation == POrientation::Horizontal && data.HasValueLabel)
    {
        UpdateMetrics(target, data);
        return PRect::FromSize(m_ValueLabelSize);
    }
    else
    {
        return PRect();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PSliderStyleLayout::GetTrackFrame(const PView& target, const PRect& bounds, const PSliderStyleData& data, PRect* outTotalFrame, float* minimumLength)
{
    UpdateMetrics(target, data);
    PRect knobFrame = m_LayoutKnobBounds;
    if (data.Orientation == POrientation::Vertical)
    {
        std::swap(knobFrame.left, knobFrame.top);
        std::swap(knobFrame.right, knobFrame.bottom);
    }

    const float center = std::floor(data.SliderSize * 0.5f);
    PRect sliderFrame(0.0f, -center, 0.0f, data.SliderSize - center);
    PRect totalFrame = sliderFrame;

    if (data.TicksAbove) {
        totalFrame.top -= SLIDER_TICK_SPACING + SLIDER_TICK_LENGTH;
    }
    if (data.TicksBelow) {
        totalFrame.bottom += SLIDER_TICK_SPACING + SLIDER_TICK_LENGTH;
    }
    if (knobFrame.top < totalFrame.top)         totalFrame.top = knobFrame.top;
    if (knobFrame.bottom > totalFrame.bottom)   totalFrame.bottom = knobFrame.bottom;

    if (data.Orientation == POrientation::Vertical)
    {
        std::swap(knobFrame.left,   knobFrame.top);
        std::swap(knobFrame.right,  knobFrame.bottom);
        std::swap(sliderFrame.left, sliderFrame.top);
        std::swap(sliderFrame.right, sliderFrame.bottom);
        std::swap(totalFrame.left, totalFrame.top);
        std::swap(totalFrame.right, totalFrame.bottom);
    }

    if (data.Orientation == POrientation::Horizontal)
    {
        if (data.HasValueLabel) {
            totalFrame.top -= m_ValueLabelSize.y;
        }
        if (!data.MinLabel.empty() || !data.MaxLabel.empty())
        {
            const PFontHeight    fontHeight = target.GetFontHeight();
            totalFrame.bottom += fontHeight.ascender + fontHeight.descender + SLIDER_HORIZONTAL_LABEL_SPACING;
        }
        const float leadingExtent = std::max(0.0f, -knobFrame.left);
        const float trailingExtent = std::max(0.0f, knobFrame.right);
        sliderFrame.left = leadingExtent;
        sliderFrame.right = std::max(sliderFrame.left, bounds.right - trailingExtent);
        totalFrame.left = bounds.left;
        totalFrame.right = bounds.right;
        if (minimumLength != nullptr) {
            *minimumLength = leadingExtent + trailingExtent;
        }
    }
    else
    {
        const PFontHeight    fontHeight = target.GetFontHeight();

        const float minLabelWidth = m_MinLabelWidth;
        const float minLabelFontHeight = data.MinLabel.empty() ? 0.0f : fontHeight.ascender + fontHeight.descender + SLIDER_VERTICAL_LABEL_SPACING;

        const float maxLabelWidth = m_MaxLabelWidth;
        const float maxLabelFontHeight = data.MaxLabel.empty() ? 0.0f : fontHeight.ascender + fontHeight.descender + SLIDER_VERTICAL_LABEL_SPACING;

        const float labelCenter = std::ceil(std::max(minLabelWidth, maxLabelWidth) * 0.5f);

        if (-labelCenter < totalFrame.left) totalFrame.left = -labelCenter;
        if (labelCenter > totalFrame.right) totalFrame.right = labelCenter;

        const float leadingExtent = std::max(0.0f, -knobFrame.top);
        const float trailingExtent = std::max(0.0f, knobFrame.bottom);
        sliderFrame.top     = leadingExtent;
        sliderFrame.bottom  = bounds.bottom - trailingExtent;

        sliderFrame.top     += maxLabelFontHeight;
        sliderFrame.bottom  -= minLabelFontHeight;
        sliderFrame.bottom = std::max(sliderFrame.top, sliderFrame.bottom);

        totalFrame.top    = bounds.top;
        totalFrame.bottom = bounds.bottom;
        if (minimumLength != nullptr) {
            *minimumLength = leadingExtent + trailingExtent + minLabelFontHeight + maxLabelFontHeight;
        }
    }
    sliderFrame.left -= totalFrame.left;
    sliderFrame.right -= totalFrame.left;
    totalFrame.right -= totalFrame.left;
    totalFrame.left = 0.0f;

    sliderFrame.top -= totalFrame.top;
    sliderFrame.bottom -= totalFrame.top;
    totalFrame.bottom -= totalFrame.top;
    totalFrame.top = 0.0f;

    PPoint centerOffset;
    if (data.Orientation == POrientation::Horizontal) {
        centerOffset.y = std::floor((bounds.Height() - totalFrame.Height()) * 0.5f);
    } else {
        centerOffset.x = std::floor((bounds.Width() - totalFrame.Width()) * 0.5f);
    }
    sliderFrame += centerOffset;
    totalFrame += centerOffset;
    if (outTotalFrame != nullptr) {
        *outTotalFrame = totalFrame;
    }
    return sliderFrame;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PSliderStyleLayout::GetPosition(const PRect& trackFrame, POrientation orientation, float relativeValue)
{
    if (orientation == POrientation::Horizontal)
    {
        return PPoint(std::round(trackFrame.left + trackFrame.Width() * relativeValue), std::floor(trackFrame.top + trackFrame.Height() * 0.5f));
    }
    else
    {
        return PPoint(
            std::floor(trackFrame.left + trackFrame.Width() * 0.5f),
            std::round(trackFrame.top + trackFrame.Height() * (1.0f - relativeValue)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::DrawLabels(PView& target, const PRect& bounds, const PRect& sliderFrame, const PSliderStyleData& data, PColor color) const
{
    const PPoint         center(bounds.left + bounds.Width() * 0.5f, bounds.top + bounds.Height() * 0.5f);
    const PFontHeight    fontHeight = target.GetFontHeight();

    target.SetBrush(nullptr);
    target.SetFgColor(color);
    if (data.Orientation == POrientation::Horizontal)
    {
        if (!data.MinLabel.empty() && !data.MaxLabel.empty())
        {
            const float top = GetHorizontalLimitLabelTop(sliderFrame, data);
            const float rightLabelX = bounds.right - m_MaxLabelWidth;
            const float split = std::clamp(rightLabelX, bounds.left, bounds.right);
            target.DrawString(data.MinLabel, PPoint(bounds.left, top + fontHeight.ascender), PRect(bounds.left, top, split, bounds.bottom));
            target.DrawString(data.MaxLabel, PPoint(rightLabelX, top + fontHeight.ascender), PRect(split, top, bounds.right, bounds.bottom));
        }
    }
    else
    {
        const float top = sliderFrame.top + std::min(0.0f, m_LayoutKnobBounds.top) - SLIDER_VERTICAL_LABEL_SPACING;
        const float bottom = sliderFrame.bottom + std::max(0.0f, m_LayoutKnobBounds.bottom) + SLIDER_VERTICAL_LABEL_SPACING;
        if (!data.MaxLabel.empty())
        {
            target.DrawString(
                data.MaxLabel,
                PPoint(center.x - m_MaxLabelWidth * 0.5f, top - fontHeight.descender),
                PRect(bounds.left, bounds.top, bounds.right, top));
        }
        if (!data.MinLabel.empty())
        {
            target.DrawString(
                data.MinLabel,
                PPoint(center.x - m_MinLabelWidth * 0.5f, bottom + fontHeight.ascender),
                PRect(bounds.left, bottom, bounds.right, bounds.bottom));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::DrawTicks(PView& target, const PRect& sliderFrame, const PSliderStyleData& data, PColor shadow, PColor shine)
{
    if (data.TickCount > 0 && sliderFrame.IsValid())
    {
        const float scale = (data.TickCount > 1) ? 1.0f / float(data.TickCount - 1) : 0.0f;

        if (data.Orientation == POrientation::Horizontal)
        {
            const float width = std::floor(sliderFrame.Width() - 2.0f);

            const float y1 = std::floor(sliderFrame.top - SLIDER_TICK_SPACING);
            const float y2 = y1 - SLIDER_TICK_LENGTH;
            const float y4 = std::floor(sliderFrame.bottom + SLIDER_TICK_SPACING);
            const float y3 = y4 + SLIDER_TICK_LENGTH;

            target.SetFgColor(shadow);
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float x = std::floor(sliderFrame.left + width * float(i) * scale);
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x, y1), PPoint(x, y2));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x, y3), PPoint(x, y4));
                }
            }
            target.SetFgColor(shine);
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float x = std::floor(sliderFrame.left + width * float(i) * scale) + 1.0f;
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x, y1), PPoint(x, y2));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x, y3), PPoint(x, y4));
                }
            }
        }
        else
        {
            const float height = sliderFrame.Height() - 2.0f;

            const float x1 = std::floor(sliderFrame.left - SLIDER_TICK_SPACING);
            const float x2 = x1 - SLIDER_TICK_LENGTH;
            const float x4 = std::floor(sliderFrame.right + SLIDER_TICK_SPACING - 1.0f);
            const float x3 = x4 + SLIDER_TICK_LENGTH;

            target.SetFgColor(shadow);
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float y = std::floor(sliderFrame.top + height * float(i) * scale);
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x1, y), PPoint(x2, y));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x3, y), PPoint(x4, y));
                }
            }
            target.SetFgColor(shine);
            for (int i = 0; i < data.TickCount; ++i)
            {
                const float y = std::floor(sliderFrame.top + height * float(i) * scale) + 1.0f;
                if (data.TicksAbove) {
                    target.DrawLine(PPoint(x1, y), PPoint(x2, y));
                }
                if (data.TicksBelow) {
                    target.DrawLine(PPoint(x3, y), PPoint(x4, y));
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PSliderStyleLayout::GetHorizontalLimitLabelTop(const PRect& sliderFrame, const PSliderStyleData& data) const
{
    const float knobBottom = std::floor(sliderFrame.top + sliderFrame.Height() * 0.5f) + m_LayoutKnobBounds.bottom;
    const float tickBottom = sliderFrame.bottom + (data.TicksBelow ? SLIDER_TICK_SPACING + SLIDER_TICK_LENGTH : 0.0f);
    return std::max(knobBottom, tickBottom) + SLIDER_HORIZONTAL_LABEL_SPACING;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::UpdateMetrics(const PView& target, const PSliderStyleData& data)
{
    if (!m_IsValueLabelSizeValid)
    {
        const PFontHeight fontHeight = target.GetFontHeight();
        m_ValueLabelSize = PPoint(target.GetStringWidth(data.ValueLabel), fontHeight.ascender + fontHeight.descender);
        m_IsValueLabelSizeValid = true;
    }
    if (!m_AreLimitLabelWidthsValid)
    {
        m_MinLabelWidth = target.GetStringWidth(data.MinLabel);
        m_MaxLabelWidth = target.GetStringWidth(data.MaxLabel);
        m_AreLimitLabelWidthsValid = true;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::UpdateShadowDamage(PView& target, const PSliderStyleData& data, const PRect& trackFrame, const PRect& knobBounds, bool invalidate)
{
    const size_t previousCount = m_ShadowKnobFrames.size();
    if (invalidate)
    {
        for (size_t i = data.ShadowValues.size(); i < previousCount; ++i) {
            InvalidateFrames(target, m_ShadowKnobFrames[i], PRect());
        }
    }
    m_ShadowKnobFrames.resize(data.ShadowValues.size());
    for (size_t i = 0; i < data.ShadowValues.size(); ++i)
    {
        const PRect frame = knobBounds + GetPosition(trackFrame, data.Orientation, data.ShadowValues[i]);
        if (invalidate && (i >= previousCount || frame != m_ShadowKnobFrames[i])) {
            InvalidateFrames(target, m_ShadowKnobFrames[i], frame);
        }
        m_ShadowKnobFrames[i] = frame;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PSliderStyleLayout::InvalidateFrames(PView& target, const PRect& previousFrame, const PRect& currentFrame)
{
    if (previousFrame.IsValid()) {
        target.Invalidate(currentFrame.IsValid() ? previousFrame | currentFrame : previousFrame);
    } else if (currentFrame.IsValid()) {
        target.Invalidate(currentFrame);
    }
}
