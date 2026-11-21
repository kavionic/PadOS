// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/ViewRendererSlider.h>
#include <GUI/Styles/SkinnableGUIStyle.h>

class PViewRendererSliderSkinned : public PViewRendererSlider
{
public:
    explicit PViewRendererSliderSkinned(const PSliderSkin& skin);
    virtual Ptr<PtrTarget> Setup(PView& target) const override;
    virtual void Paint(PView& target, const PRect& frame, const PRect& updateRect, PtrTarget* context) const override;
    virtual void GetPreferredSize(const PView& target, PPoint* minSize, PPoint* maxSize, PtrTarget* context) const override;
    virtual void PropertyChanged(PView& target, PSliderProperties properties, PtrTarget* context) const override;
    virtual PPoint GetValuePosition(const PView& target, float relativeValue, PtrTarget* context) const override;
    virtual float GetValueFromPosition(const PView& target, const PPoint& position, PtrTarget* context) const override;

private:
    struct Context;

    Context& GetSliderStyleData(const PSliderStyleData& data, PtrTarget* context) const;
    static PButtonSkinState GetSliderState(const PSliderStyleData& data);
    static size_t GetSliderKnobIndex(const PSliderStyleData& data);
    static uint8_t ShadeSliderComponent(uint8_t component, uint8_t shade);
    static Ptr<PImage> CreateSliderTrackImage(const PImage& source, PColor color);
    static void UpdateSliderTrackImages(
        Context& state,
        const PSliderOrientationSkin& skin,
        const PSliderSkinColors& colors,
        const PSliderStyleData& data,
        const PPoint& size);
    static void DrawSkinSliderKnob(
        PView& target,
        const PRect& knobFrame,
        const PRect& beforeFrame,
        const PRect& afterFrame,
        const Context& state,
        const Ptr<const PBitmap>& bitmap);

    const PSliderSkin m_SliderSkin;
};
