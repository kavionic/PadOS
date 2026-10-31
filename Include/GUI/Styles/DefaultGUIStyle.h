// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Color.h>
#include <GUI/GUIDefines.h>
#include <GUI/Style/GUIStyle.h>

class PDefaultGUIStyle : public PGUIStyle
{
public:
    void ButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const override;
    void ButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    void CheckBoxPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const override;
    void CheckBoxGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    void RadioButtonPaint(PView& target, const PRect& frame, bool isEnabled, PButtonStyleData& data) const override;
    void RadioButtonGetPreferredSize(const PView& target, PButtonStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    void SliderPaint(PView& target, const PRect& frame, const PSliderStyleData& data) const override;
    void SliderGetPreferredSize(const PView& target, const PSliderStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    // Called after all listed properties have been updated. The style owns
    // invalidation and preferred-size notifications for these changes.
    void SliderPropertyChanged(PView& target, PSliderStyleData& data, PSliderProperties properties) const override;

    PPoint SliderGetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue) const override;
    float SliderGetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position) const override;

    void ScrollBarPaint(PView& target, const PRect& frame, const PScrollBarStyleData& data) const override;
    void ScrollBarGetPreferredSize(const PView& target, const PScrollBarStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    // Called after all listed properties have been updated. The style owns
    // invalidation and preferred-size notifications for these changes.
    void ScrollBarPropertyChanged(PView& target, PScrollBarStyleData& data, PScrollBarProperties properties) const override;
    PScrollBarPart ScrollBarHitTest(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const override;

    // Positions refer to the knob center. The widget preserves the pointer's
    // offset from this center while dragging.
    PPoint ScrollBarGetValuePosition(const PView& target, const PScrollBarStyleData& data, float relativeValue) const override;
    float ScrollBarGetValueFromPosition(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const override;

private:
    struct SliderStyleData;
    struct ScrollBarStyleData;
    struct ScrollBarGeometry;

    static constexpr float SCROLL_BAR_THICKNESS = 16.0f;
    static constexpr float SCROLL_BAR_MIN_KNOB_SIZE = 12.0f;
    static constexpr uint32_t SCROLL_BAR_LEFT_ARROW_BITMAP_RASTER[] =
    {
        0b00010000000000000000000000000000,
        0b00110000000000000000000000000000,
        0b01110000000000000000000000000000,
        0b11110000000000000000000000000000,
        0b01110000000000000000000000000000,
        0b00110000000000000000000000000000,
        0b00010000000000000000000000000000
    };
    static constexpr uint32_t SCROLL_BAR_RIGHT_ARROW_BITMAP_RASTER[] =
    {
        0b10000000000000000000000000000000,
        0b11000000000000000000000000000000,
        0b11100000000000000000000000000000,
        0b11110000000000000000000000000000,
        0b11100000000000000000000000000000,
        0b11000000000000000000000000000000,
        0b10000000000000000000000000000000
    };
    static constexpr uint32_t SCROLL_BAR_UP_ARROW_BITMAP_RASTER[] =
    {
        0b00010000000000000000000000000000,
        0b00111000000000000000000000000000,
        0b01111100000000000000000000000000,
        0b11111110000000000000000000000000
    };
    static constexpr uint32_t SCROLL_BAR_DOWN_ARROW_BITMAP_RASTER[] =
    {
        0b11111110000000000000000000000000,
        0b01111100000000000000000000000000,
        0b00111000000000000000000000000000,
        0b00010000000000000000000000000000
    };

    static constexpr float CHECK_BOX_SIZE = 32.0f;
    static constexpr float CHECK_BOX_LABEL_SPACING = 8.0f;
    static constexpr float CHECK_BOX_CHECK_BORDER = 7.0f;
    static constexpr float RADIO_BUTTON_SIZE = 32.0f;
    static constexpr float RADIO_BUTTON_LABEL_HORIZONTAL_SPACING = 8.0f;
    static constexpr float RADIO_BUTTON_LABEL_VERTICAL_SPACING = 4.0f;

    static constexpr float SLIDER_TICK_LENGTH = 6.0f;
    static constexpr float SLIDER_TICK_SPACING = 3.0f;
    static constexpr float SLIDER_VERTICAL_LABEL_SPACING = 3.0f;
    static constexpr float SLIDER_HORIZONTAL_LABEL_SPACING = 2.0f;

    enum class SliderKnobFrameMode
    {
        FullFrame,
        SquareFrame
    };

    static void UpdateButtonMetrics(const PView& target, PButtonStyleData& data);
    static PRect DrawRadioButtonLabel(PView& target, const PRect& frame, PButtonStyleData& data);
    static void UpdateSliderMetrics(const PView& target, const PSliderStyleData& data);
    static void UpdateSliderDamage(PView& target, const PSliderStyleData& data, PSliderProperties properties, bool invalidate);

    static void UpdateSliderShadowDamage(
        PView& target,
        const PSliderStyleData& data,
        const PRect& trackFrame,
        const PRect& knobBounds,
        bool invalidate);

    static void InvalidateSliderFrames(PView& target, const PRect& previousFrame, const PRect& currentFrame);
    static PRect GetSliderValueLabelFrame(const PView& target, const PSliderStyleData& data);

    static PRect GetSliderTrackFrame(
        const PView& target,
        const PRect& bounds,
        const PSliderStyleData& data,
        PRect* outTotalFrame = nullptr,
        float* minimumLength = nullptr);

    static PRect GetSliderKnobBounds(const PSliderStyleData& data, POrientation orientation, SliderKnobFrameMode mode);
    static PPoint GetSliderPosition(const PRect& trackFrame, POrientation orientation, float relativeValue);
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

    static void DrawSliderLabels(PView& target, const PRect& bounds, const PRect& sliderFrame, const PSliderStyleData& data);
    static void DrawSliderTicks(PView& target, const PRect& sliderFrame, const PSliderStyleData& data);

    static ScrollBarGeometry GetScrollBarGeometry(const PRect& frame, const PScrollBarStyleData& data, float relativeValue);
    static PRect GetScrollBarFrame(POrientation orientation, float start, float end, float crossStart, float crossEnd);
    static PScrollBarPart GetScrollBarArrowPart(size_t index);
    static PColor GetScrollBarTint(PColor color, float tint);
    static void DrawScrollBarKnob(PView& target, const PRect& knobFrame, POrientation orientation);
    static void DrawScrollBarTrack(PView& target, const ScrollBarGeometry& geometry, POrientation orientation);
    static void DrawScrollBarArrows(PView& target, const ScrollBarGeometry& geometry, const PScrollBarStyleData& data);
};
