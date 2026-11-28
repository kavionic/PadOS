// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>
#include <utility>

#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleScrollBar.h>
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererScrollBarSkinned.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

struct PViewRendererScrollBarSkinned::SkinnedScrollBarPart
{
    std::array<PImage, std::to_underlying(PButtonSkinState::Count)> Images;
    std::array<PBrush, std::to_underlying(PButtonSkinState::Count)> Brushes;
};

struct PViewRendererScrollBarSkinned::Context : PViewRendererContext<PViewStyleScrollBar>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}

    SkinnedScrollBarPart Track;
    SkinnedScrollBarPart Knob;
    std::array<SkinnedScrollBarPart, 4> Arrows;
    PScrollBarPart PressedPart = PScrollBarPart::None;
    PScrollBarPart HoveredPart = PScrollBarPart::None;
    bool HasVisualState = false;
};

struct PViewRendererScrollBarSkinned::SkinnedScrollBarGeometry
{
    PRect Frame;
    PRect TrackFrame;
    PRect KnobArea;
    PRect KnobFrame;
    std::array<PRect, 4> ArrowFrames;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererScrollBarSkinned::PViewRendererScrollBarSkinned(const PScrollBarSkin& skin) : m_ScrollBarSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererScrollBarSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PScrollBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetScrollBarStyleData();
    if (!frame.IsValid()) {
        return;
    }
    const PScrollBarOrientationSkin& skin = GetScrollBarOrientation(data.Orientation);
    const SkinnedScrollBarGeometry geometry = GetScrollBarGeometry(frame, data, data.Value);
    Context& state = *static_cast<Context*>(context);
    const PDrawingMode previousMode = target.GetDrawingMode();
    PScopeExit restoreDrawing([&target, previousMode] { target.SetBrush(PBrush::zero); target.SetDrawingMode(previousMode); });
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

void PViewRendererScrollBarSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PScrollBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetScrollBarStyleData();
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

void PViewRendererScrollBarSkinned::PropertyChanged(PView& target, PScrollBarProperties properties, PtrTarget* context) const
{
    const PScrollBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetScrollBarStyleData();
    Context& state = *static_cast<Context*>(context);
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

PScrollBarPart PViewRendererScrollBarSkinned::HitTest(const PView& target, const PPoint& position, PtrTarget* context) const
{
    const PScrollBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetScrollBarStyleData();
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

PPoint PViewRendererScrollBarSkinned::GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const
{
    const PScrollBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetScrollBarStyleData();
    const SkinnedScrollBarGeometry geometry = GetScrollBarGeometry(target.GetNormalizedBounds(), data, relativeValue);
    return geometry.KnobFrame.IsValid() ? geometry.KnobFrame.Center() : target.GetNormalizedBounds().Center();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PViewRendererScrollBarSkinned::GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    const PScrollBarStyleData& data = static_cast<Context*>(context)->GetTarget().GetScrollBarStyleData();
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

const PScrollBarOrientationSkin& PViewRendererScrollBarSkinned::GetScrollBarOrientation(POrientation orientation) const
{
    return (orientation == POrientation::Horizontal) ? m_ScrollBarSkin.Horizontal : m_ScrollBarSkin.Vertical;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererScrollBarSkinned::SkinnedScrollBarGeometry PViewRendererScrollBarSkinned::GetScrollBarGeometry(
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

PRect PViewRendererScrollBarSkinned::GetScrollBarPartFrame(const SkinnedScrollBarGeometry& geometry, PScrollBarPart part)
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

PButtonSkinState PViewRendererScrollBarSkinned::GetScrollBarState(const PScrollBarStyleData& data, PScrollBarPart part)
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

PBrush PViewRendererScrollBarSkinned::GetScrollBarBrush(SkinnedScrollBarPart& cache, const PScrollBarSkinPart& part, PButtonSkinState state, const PPoint& size)
{
    const size_t index = std::to_underlying(state);
    if (!cache.Images[index].IsValid() || cache.Images[index].GetLayoutSize() != size)
    {
        const PImage& source = part.States[index].Image.IsValid() ? part.States[index].Image : part.States[0].Image;
        PImage image = cache.Images[index].IsValid() ? cache.Images[index] : source;
        image.SetLayoutSize(size, PImageLayoutMode::Absolute);
        PBrush brush(image);
        cache.Images[index] = std::move(image);
        cache.Brushes[index] = std::move(brush);
    }
    return cache.Brushes[index];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererScrollBarSkinned::PaintScrollBarPart(
    PView& target,
    const PRect& frame,
    const PScrollBarSkinPart& part,
    SkinnedScrollBarPart& cache,
    PButtonSkinState state)
{
    if (!frame.IsValid()) {
        return;
    }
    const PBrush brush = GetScrollBarBrush(cache, part, state, frame.Size());
    const size_t index = std::to_underlying(state);
    const PScrollBarSkinAppearance& appearance = (part.States[index].Image.IsValid()) ? part.States[index] : part.States[0];
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

void PViewRendererScrollBarSkinned::ClearScrollBarTrack(PView& target, const SkinnedScrollBarGeometry& geometry)
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
