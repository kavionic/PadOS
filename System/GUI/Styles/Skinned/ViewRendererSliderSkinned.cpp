// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <cmath>

#include <GUI/Style/SliderStyleLayout.h>
#include <GUI/Style/ViewRendererContext.h>
#include <GUI/Style/ViewStyleSlider.h>
#include <GUI/Styles/Skinned/SkinnedImage.h>
#include <GUI/Styles/Skinned/ViewRendererSliderSkinned.h>
#include <GUI/View.h>
#include <Math/Misc.h>
#include <System/ExceptionHandling.h>

struct PViewRendererSliderSkinned::Context : PViewRendererContext<PViewStyleSlider>
{
    explicit Context(PView& target) : PViewRendererContext(target) {}

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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererSliderSkinned::PViewRendererSliderSkinned(const PSliderSkin& skin) : m_SliderSkin(skin)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PViewRendererSliderSkinned::Setup(PView& target) const
{
    return ptr_new<Context>(target);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderSkinned::Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const
{
    const PSliderStyleData& data = static_cast<Context*>(context)->GetTarget().GetSliderStyleData();
    Context& state = GetSliderStyleData(data, context);
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

void PViewRendererSliderSkinned::GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const
{
    const PSliderStyleData& data = static_cast<Context*>(context)->GetTarget().GetSliderStyleData();
    GetSliderStyleData(data, context).Layout.GetPreferredSize(target, data, minSize, maxSize);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PViewRendererSliderSkinned::PropertyChanged(PView& target, PSliderProperties properties, PtrTarget* context) const
{
    const PSliderStyleData& data = static_cast<Context*>(context)->GetTarget().GetSliderStyleData();
    Context& state = GetSliderStyleData(data, context);
    const PSliderSkinColors& colors = m_SliderSkin.Colors[std::to_underlying(GetSliderState(data))];
    if (state.Appearance != nullptr && *state.Appearance != colors) {
        target.Invalidate();
    }
    state.Appearance = &colors;
    state.Layout.PropertyChanged(target, data, properties);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PViewRendererSliderSkinned::GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const
{
    const PSliderStyleData& data = static_cast<Context*>(context)->GetTarget().GetSliderStyleData();
    return GetSliderStyleData(data, context).Layout.GetValuePosition(target, data, relativeValue);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PViewRendererSliderSkinned::GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const
{
    const PSliderStyleData& data = static_cast<Context*>(context)->GetTarget().GetSliderStyleData();
    return GetSliderStyleData(data, context).Layout.GetValueFromPosition(target, data, position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewRendererSliderSkinned::Context& PViewRendererSliderSkinned::GetSliderStyleData(const PSliderStyleData& data, PtrTarget* context) const
{
    Context& state = *static_cast<Context*>(context);
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

PButtonSkinState PViewRendererSliderSkinned::GetSliderState(const PSliderStyleData& data)
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

size_t PViewRendererSliderSkinned::GetSliderKnobIndex(const PSliderStyleData& data)
{
    return (data.KnobPointUp ? size_t(1) : size_t(0)) + (data.KnobPointDown ? size_t(2) : size_t(0));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t PViewRendererSliderSkinned::ShadeSliderComponent(uint8_t component, uint8_t shade)
{
    return (shade < 128) ? uint8_t((uint32_t(component) * shade + 64) / 128) : uint8_t(component + (uint32_t(255 - component) * (shade - 128) + 63) / 127);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PImage> PViewRendererSliderSkinned::CreateSliderTrackImage(const PImage& source, PColor color)
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

void PViewRendererSliderSkinned::UpdateSliderTrackImages(
    Context& state,
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

void PViewRendererSliderSkinned::DrawSkinSliderKnob(
    PView& target,
    const PRect& knobFrame,
    const PRect& beforeFrame,
    const PRect& afterFrame,
    const Context& state,
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
