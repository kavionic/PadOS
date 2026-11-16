// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <vector>
#include <GUI/Style/SliderStyleData.h>
#include <Math/Rect.h>

class PView;

// Optional shared layout and redraw bookkeeping, owned by each style's payload.
// Measuring geometry does not advance the appearance snapshot used for damage.
class PSliderStyleLayout
{
public:
    // Bounds relative to the track anchor, covering every knob state and shadow.
    void SetKnobBounds(const PRect& bounds) noexcept { m_LayoutKnobBounds = bounds; }

    void GetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize);
    void PropertyChanged(PView& target, const PSliderStyleData& data, PSliderProperties properties);
    PPoint GetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue);
    float GetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position);

    bool HasVisualState() const noexcept { return m_HasVisualState; }

    void ClearBackground(PView& target, const PRect& bounds, const PRect& sliderFrame, const PRect& knobFrame, const PSliderStyleData& data);
    void UpdateDamage(PView& target, const PSliderStyleData& data, PSliderProperties properties, bool invalidate);
    PRect GetValueLabelFrame(const PView& target, const PSliderStyleData& data);
    PRect GetTrackFrame(const PView& target, const PRect& bounds, const PSliderStyleData& data, PRect* outTotalFrame = nullptr, float* minimumLength = nullptr);
    static PPoint GetPosition(const PRect& trackFrame, POrientation orientation, float relativeValue);
    void DrawLabels(PView& target, const PRect& bounds, const PRect& sliderFrame, const PSliderStyleData& data, PColor color) const;
    static void DrawTicks(PView& target, const PRect& sliderFrame, const PSliderStyleData& data, PColor shadow, PColor shine);

private:
    static constexpr float SLIDER_TICK_LENGTH = 6.0f;
    static constexpr float SLIDER_TICK_SPACING = 3.0f;
    static constexpr float SLIDER_VERTICAL_LABEL_SPACING = 3.0f;
    static constexpr float SLIDER_HORIZONTAL_LABEL_SPACING = 2.0f;

    float GetHorizontalLimitLabelTop(const PRect& sliderFrame, const PSliderStyleData& data) const;
    void UpdateMetrics(const PView& target, const PSliderStyleData& data);
    void UpdateShadowDamage(PView& target, const PSliderStyleData& data, const PRect& trackFrame, const PRect& knobBounds, bool invalidate);
    static void InvalidateFrames(PView& target, const PRect& previousFrame, const PRect& currentFrame);

    PRect m_LayoutKnobBounds;
    PPoint m_ValueLabelSize;
    float m_MinLabelWidth = 0.0f;
    float m_MaxLabelWidth = 0.0f;
    PRect m_KnobFrame;
    PRect m_ValueLabelFrame;
    std::vector<PRect> m_ShadowKnobFrames;
    bool m_IsValueLabelSizeValid = false;
    bool m_AreLimitLabelWidthsValid = false;
    bool m_HasVisualState = false;
};
