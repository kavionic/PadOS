// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/SliderStyleData.h>
#include <GUI/Widgets/Control.h>


namespace PSliderFlags
{
static constexpr uint32_t TicksAbove        = 0x0001 << PViewFlags::FirstUserBit;
static constexpr uint32_t TicksBelow        = 0x0002 << PViewFlags::FirstUserBit;
static constexpr uint32_t TicksLeft         = TicksAbove;
static constexpr uint32_t TicksRight        = TicksBelow;
static constexpr uint32_t KnobPointUp       = 0x0004 << PViewFlags::FirstUserBit;
static constexpr uint32_t KnobPointDown     = 0x0008 << PViewFlags::FirstUserBit;
static constexpr uint32_t KnobPointLeft     = KnobPointUp;
static constexpr uint32_t KnobPointRight    = KnobPointDown;

extern const std::map<PString, uint32_t> FlagMap;

}

///////////////////////////////////////////////////////////////////////////////
/// \ingroup gui
/// \par Description:
///
/// \sa os::Control
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PSlider : public PControl
{
public:
    PSlider(
        const PString& name = PString::zero,
        uint32_t flags = PSliderFlags::TicksBelow,
        int tickCount = 10,
        POrientation orientation = POrientation::Horizontal);
    PSlider(PViewFactoryContext& context, const pugi::xml_node& xmlData);

    ~PSlider() override = default;

    // From View:
    virtual void OnFlagsChanged(uint32_t changedFlags) override;

    void SetLabel(const PString& label);
    const PString& GetLabel() const { return m_SliderStyleData.Label; }
    virtual void OnLabelChanged(const PString& label);

    // From Slider:
    virtual float PosToVal(const PPoint& position) const;
    virtual PPoint ValToPos(float value) const;

    virtual void    SetSliderColors(const PColor& color1, const PColor& color2);
    virtual void    GetSliderColors(PColor* color1, PColor* color2) const;
    virtual void    SetSliderSize(float size);
    virtual float   GetSliderSize() const;

    
    virtual void    SetValueStringFormat(const PString& format, float scale = 1.0f);
    virtual PString GetValueStringFormat() const;
    virtual PString GetValueString() const;

    void        SetValue(float value, bool sendEvent = true);
    float       GetValue() const;

    void        SetStepCount(int count);
    int         GetStepCount() const;
    
    void        SetTickCount( int count);
    int         GetTickCount() const;

    void        SetLimitLabels(const PString& minLabel, const PString& maxLabel);
    void        GetLimitLabels(PString* minLabel, PString* maxLabel);
    
    virtual void    SetSteps(float small, float big)         { m_SmallStep = small; m_BigStep = big; }
    virtual void    GetSteps(float* small, float* big) const { *small = m_SmallStep; *big = m_BigStep; }
    void            SetDragScale(float scale) { m_DragScale = scale; }
    void            SetDragScaleRange(float range) { m_DragScaleRange = range; }
    virtual void    SetMinMax(float min, float max);
    void            SetResolution(float resolution);

    void            SetShadowKnobsCount(size_t count);
    size_t          GetShadowKnobsCount() const;
    void            SetShadowKnobValue(size_t index, float value);

    bool            IsBeingDragged() const { return m_HitPointerID != PInvalidPointerID; }
    // From Control:
    virtual void    OnEnableStatusChanged(bool isEnabled) override;
    
    // From View
    virtual void AttachedToScreen() override;
    virtual void OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason) override;
    virtual void OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void OnStyleChanged() override;
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;


    Signal<void, float, PSlider*, PPointerID> SignalBeginDrag;
    Signal<void, float, PSlider*, PPointerID> SignalEndDrag;
    Signal<void, float, bool, PSlider*>          SignalValueChanged;//(float value, bool finalUpdate, os::Slider* slider)
private:
    float GetRelativeValue(float value) const;
    void UpdateStyleFlags();
    bool UpdateValueLabel();
    void NotifyStylePropertyChanged(PSliderProperties properties);

    PSliderStyleData m_SliderStyleData;
    std::vector<float> m_ShadowValues;
    PString         m_ValueFormat;
    float           m_Min = 0.0f;
    float           m_Max = 1.0f;
    float           m_Value = 0.0f;
    float           m_ValueScale = 1.0f;
    int             m_NumSteps = 0;
    float           m_Resolution = 0.0f;
    float           m_SmallStep = 0.05f;
    float           m_BigStep = 0.1f;
    float           m_DragScale = 1.0f;
    float           m_DragScaleRange = 100.0f;

    bool            m_Changed = false;
    bool            m_PointerCaptureLocked = false;
    PPointerID   m_HitPointerID = PInvalidPointerID;
    PPoint           m_HitPos;
    float           m_HitValue = 0.0f;
    PPoint           m_SmoothedPos;
};
