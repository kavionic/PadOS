// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <limits>

#include <GUI/Style/ScrollBarStyleData.h>
#include <GUI/Widgets/Control.h>
#include <Threads/EventTimer.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PScrollBar : public PControl
{
public:
    PScrollBar(
        const PString& name = PString::zero,
        float min = 0.0f,
        float max = std::numeric_limits<float>::max(),
        POrientation orientation = POrientation::Vertical,
        uint32_t flags = 0);
    ~PScrollBar();

    void  SetScrollTarget(Ptr<PView> target);
    Ptr<PView> GetScrollTarget();


    // From View
    virtual void    OnFrameSized(const PPoint& delta) override;
    virtual void    OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void    OnStyleChanged() override;
    virtual void    OnEnableStatusChanged(bool isEnabled) override;

    virtual void    OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerCancel(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
    virtual void    OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason) override;

//    virtual void	WheelMoved(const Point& cDelta);

    virtual void	OnPaint(const PRect& updateRect) override;

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    void SlotTimerTick();

    // From ScrollBar:
    void    SetValue(float value);
    float   GetValue() const { return m_Value; }

    void	SetSteps(float small, float big);
    void	GetSteps(float* small, float* big) const;

    void	SetMinMax(float min, float max);

    void	SetProportion(float proportion);
    float	GetProportion() const;

    Signal<void, float, bool, Ptr<PScrollBar>>	SignalValueChanged;

private:
    static bool IsArrowPart(PScrollBarPart part);
    void        StepValue(PScrollBarPart part);
    void        EndInteraction();
    float       GetRelativeValue() const;
    void        NotifyStylePropertyChanged(PScrollBarProperties properties);

    PEventTimer m_RepeatTimer;

    PScrollBarStyleData m_ScrollBarStyleData;

    PView*          m_Target = nullptr;
    PPoint          m_HitPos;
    float           m_Min = 0.0f;
    float           m_Max = 1.0f;
    float           m_Value = 0.0f;
    float           m_SmallStep = 1.0f;
    float           m_BigStep = 10.0f;
    PPointerID      m_HitPointerID = PInvalidPointerID;
    PScrollBarPart  m_HitPart = PScrollBarPart::None;
    bool            m_Changed = false;
    bool            m_FirstTick = true;
};
