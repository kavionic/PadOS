// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererSlider.h>

class PSliderStyleLayout;

class PViewRendererSliderDefault : public PViewRendererSlider
{
public:
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PSliderProperties properties, PtrTarget* context) const override;
    virtual PPoint GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const override;
    virtual float GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;
    enum class SliderKnobFrameMode
    {
        FullFrame,
        SquareFrame
    };
    static PSliderStyleLayout& GetSliderLayout(const PSliderStyleData& data, Context& state);
    static PRect GetSliderKnobBounds(const PSliderStyleData& data, POrientation orientation, SliderKnobFrameMode mode);
    static void DrawSliderTrack(PView& target, const PRect& sliderFrame, const PRect& knobFrame, const PSliderStyleData& data);
    static void DrawSliderKnob(PView& target, const PRect& trackFrame, const PSliderStyleData& data, PStandardColorID knobColor, float relativeValue);
    static void DrawHorizontalSliderKnob(
        PView& target,
        const PRect& knobFrame,
        const PRect& knobFullFrame,
        const PSliderStyleData& data,
        PStandardColorID knobColor);
    static void DrawVerticalSliderKnob(
        PView& target,
        const PRect& knobFrame,
        const PRect& knobFullFrame,
        const PSliderStyleData& data,
        PStandardColorID knobColor);
};
