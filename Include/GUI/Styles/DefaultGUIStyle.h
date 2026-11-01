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
    void    SliderPropertyChanged(PView& target, PSliderStyleData& data, PSliderProperties properties) const override;

    PPoint  SliderGetValuePosition(const PView& target, const PSliderStyleData& data, float relativeValue) const override;
    float   SliderGetValueFromPosition(const PView& target, const PSliderStyleData& data, const PPoint& position) const override;

    void    ScrollBarPaint(PView& target, const PRect& frame, const PScrollBarStyleData& data) const override;
    void    ScrollBarGetPreferredSize(const PView& target, const PScrollBarStyleData& data, PPoint* minSize, PPoint* maxSize) const override;

    // Called after all listed properties have been updated. The style owns
    // invalidation and preferred-size notifications for these changes.
    void            ScrollBarPropertyChanged(PView& target, PScrollBarStyleData& data, PScrollBarProperties properties) const override;
    PScrollBarPart  ScrollBarHitTest(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const override;

    // Positions refer to the knob center. The widget preserves the pointer's
    // offset from this center while dragging.
    PPoint  ScrollBarGetValuePosition(const PView& target, const PScrollBarStyleData& data, float relativeValue) const override;
    float   ScrollBarGetValueFromPosition(const PView& target, const PScrollBarStyleData& data, const PPoint& position) const override;

    virtual void DropdownMenuPaint(PView& target, const PRect& frame, const PDropdownMenuStyleData& data) const override;
    virtual void DropdownMenuGetPreferredSize(
        const PView& target,
        const PDropdownMenuStyleData& data,
        PPoint* minSize,
        PPoint* maxSize) const override;
    virtual void    DropdownMenuPropertyChanged(PView& target, PDropdownMenuStyleData& data, PDropdownMenuProperties properties) const override;
    virtual PRect   DropdownMenuGetEditorFrame(const PView& target, const PDropdownMenuStyleData& data) const override;
    virtual bool    DropdownMenuHitTest(const PView& target, const PDropdownMenuStyleData& data, const PPoint& position) const override;

    virtual void    DropdownMenuPopupFramePaint(PView& target, const PRect& frame) const override;

    virtual void    DropdownMenuPopupFrameGetPreferredSize(
        const PView& target,
        const PPoint& contentSize,
        PPoint* minSize,
        PPoint* maxSize) const override;

    virtual PRect   DropdownMenuPopupFrameGetContentFrame(const PView& target) const override;

    virtual void    DropdownMenuPopupPaint(PView& target, const PRect& updateRect, const PDropdownMenuPopupStyleData& data) const override;

    virtual void    DropdownMenuPopupGetPreferredSize(
        const PView& target,
        const PDropdownMenuPopupStyleData& data,
        PPoint* minSize,
        PPoint* maxSize) const override;

    virtual void DropdownMenuPopupPropertyChanged(
        PView& target,
        PDropdownMenuPopupStyleData& data,
        PDropdownMenuPopupProperties properties) const override;

    virtual PRect   DropdownMenuPopupGetItemFrame(const PView& target, const PDropdownMenuPopupStyleData& data, size_t index) const override;

    virtual size_t  DropdownMenuPopupGetItemFromPosition(
        const PView& target,
        const PDropdownMenuPopupStyleData& data,
        const PPoint& position) const override;

    virtual void GroupViewPaint(PView& target, const PRect& updateRect, const PGroupViewStyleData& data) const override;
    virtual void GroupViewGetPreferredSize(const PView& target, const PGroupViewStyleData& data, PPoint* minSize, PPoint* maxSize) const override;
    virtual void GroupViewPropertyChanged(PView& target, PGroupViewStyleData& data, PGroupViewProperties properties) const override;

private:
    struct SliderStyleData;
    struct ScrollBarStyleData;
    struct ScrollBarGeometry;
    struct DropdownMenuStyleData;
    struct DropdownMenuPopupStyleData;
    struct GroupViewStyleData;

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

    static constexpr int DROPDOWN_MENU_ARROW_WIDTH = 19;
    static constexpr int DROPDOWN_MENU_ARROW_HEIGHT = 10;
    static constexpr float DROPDOWN_MENU_ARROW_ASPECT_RATIO = 0.9f;
    static constexpr float DROPDOWN_MENU_LABEL_PADDING = 10.0f;
    static constexpr float DROPDOWN_MENU_POPUP_BORDER_X = 2.0f;
    static constexpr float DROPDOWN_MENU_POPUP_BORDER_Y = 4.0f;
    static constexpr float DROPDOWN_MENU_POPUP_LABEL_PADDING = 2.0f;
    static constexpr uint32_t DROPDOWN_MENU_ARROW_BITMAP_RASTER[] =
    {
        0b11111111111111111110000000000000,
        0b01111111111111111100000000000000,
        0b00111111111111111000000000000000,
        0b00011111111111110000000000000000,
        0b00001111111111100000000000000000,
        0b00000111111111000000000000000000,
        0b00000011111110000000000000000000,
        0b00000001111100000000000000000000,
        0b00000000111000000000000000000000,
        0b00000000010000000000000000000000
    };

    static constexpr float GROUP_VIEW_PEN_WIDTH = 2.0f;
    static constexpr float GROUP_VIEW_LABEL_OFFSET = 30.0f;
    static constexpr float GROUP_VIEW_LABEL_GAP_START = 15.0f;
    static constexpr float GROUP_VIEW_LABEL_SPACING = 10.0f;
    static constexpr float GROUP_VIEW_MIN_SIZE = 5.0f;

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

    static void     UpdateButtonMetrics(const PView& target, PButtonStyleData& data);
    static PRect    DrawRadioButtonLabel(PView& target, const PRect& frame, PButtonStyleData& data);
    static void     UpdateSliderMetrics(const PView& target, const PSliderStyleData& data);
    static void     UpdateSliderDamage(PView& target, const PSliderStyleData& data, PSliderProperties properties, bool invalidate);

    static void     UpdateSliderShadowDamage(
        PView& target,
        const PSliderStyleData& data,
        const PRect& trackFrame,
        const PRect& knobBounds,
        bool invalidate);

    static void     InvalidateSliderFrames(PView& target, const PRect& previousFrame, const PRect& currentFrame);
    static PRect    GetSliderValueLabelFrame(const PView& target, const PSliderStyleData& data);

    static PRect    GetSliderTrackFrame(
        const PView& target,
        const PRect& bounds,
        const PSliderStyleData& data,
        PRect* outTotalFrame = nullptr,
        float* minimumLength = nullptr);

    static PRect    GetSliderKnobBounds(const PSliderStyleData& data, POrientation orientation, SliderKnobFrameMode mode);
    static PPoint   GetSliderPosition(const PRect& trackFrame, POrientation orientation, float relativeValue);
    static void     DrawSliderTrack(PView& target, const PRect& sliderFrame, const PRect& knobFrame, const PSliderStyleData& data);
    static void     DrawSliderKnob(PView& target, const PRect& trackFrame, const PSliderStyleData& data, PStandardColorID knobColor, float relativeValue);

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

    static ScrollBarGeometry    GetScrollBarGeometry(const PRect& frame, const PScrollBarStyleData& data, float relativeValue);
    static PRect                GetScrollBarFrame(POrientation orientation, float start, float end, float crossStart, float crossEnd);
    static PScrollBarPart       GetScrollBarArrowPart(size_t index);
    static PColor               GetScrollBarTint(PColor color, float tint);
    static void                 DrawScrollBarKnob(PView& target, const PRect& knobFrame, POrientation orientation);
    static void                 DrawScrollBarTrack(PView& target, const ScrollBarGeometry& geometry, POrientation orientation);
    static void                 DrawScrollBarArrows(PView& target, const ScrollBarGeometry& geometry, const PScrollBarStyleData& data);
    static void                 UpdateDropdownMenuMetrics(const PView& target, const PDropdownMenuStyleData& data);
    static PRect                GetDropdownMenuArrowFrame(const PRect& frame);
    static void                 UpdateDropdownMenuPopupMetrics(const PView& target, const PDropdownMenuPopupStyleData& data);
    static void                 UpdateGroupViewMetrics(const PView& target, const PGroupViewStyleData& data);
};
