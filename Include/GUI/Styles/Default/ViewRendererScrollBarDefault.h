// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererScrollBar.h>

class PViewRendererScrollBarDefault : public PViewRendererScrollBar
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PScrollBarProperties properties, PtrTarget* context) const override;
    virtual PScrollBarPart HitTest(const PView& target, const PPoint& position, PtrTarget* context) const override;
    virtual PPoint GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const override;
    virtual float GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;
    struct ScrollBarGeometry;
    static constexpr float SCROLL_BAR_THICKNESS = 16.0f;
    static constexpr float SCROLL_BAR_MIN_KNOB_SIZE = 12.0f;
    static constexpr uint32_t SCROLL_BAR_LEFT_ARROW_BITMAP_RASTER[] = {
        0b00010000000000000000000000000000,
        0b00110000000000000000000000000000,
        0b01110000000000000000000000000000,
        0b11110000000000000000000000000000,
        0b01110000000000000000000000000000,
        0b00110000000000000000000000000000,
        0b00010000000000000000000000000000
    };
    static constexpr uint32_t SCROLL_BAR_RIGHT_ARROW_BITMAP_RASTER[] = {
        0b10000000000000000000000000000000,
        0b11000000000000000000000000000000,
        0b11100000000000000000000000000000,
        0b11110000000000000000000000000000,
        0b11100000000000000000000000000000,
        0b11000000000000000000000000000000,
        0b10000000000000000000000000000000
    };
    static constexpr uint32_t SCROLL_BAR_UP_ARROW_BITMAP_RASTER[] = {
        0b00010000000000000000000000000000,
        0b00111000000000000000000000000000,
        0b01111100000000000000000000000000,
        0b11111110000000000000000000000000
    };
    static constexpr uint32_t SCROLL_BAR_DOWN_ARROW_BITMAP_RASTER[] = {
        0b11111110000000000000000000000000,
        0b01111100000000000000000000000000,
        0b00111000000000000000000000000000,
        0b00010000000000000000000000000000
    };
    static ScrollBarGeometry GetScrollBarGeometry(const PRect& frame, const PScrollBarStyleData& data, float relativeValue);
    static PRect GetScrollBarFrame(POrientation orientation, float start, float end, float crossStart, float crossEnd);
    static PScrollBarPart GetScrollBarArrowPart(size_t index);
    static PColor GetScrollBarTint(PColor color, float tint);
    static void DrawScrollBarKnob(PView& target, const PRect& knobFrame, POrientation orientation);
    static void DrawScrollBarTrack(PView& target, const ScrollBarGeometry& geometry, POrientation orientation);
    static void DrawScrollBarArrows(PView& target, const ScrollBarGeometry& geometry, const PScrollBarStyleData& data, Context& state);
};
