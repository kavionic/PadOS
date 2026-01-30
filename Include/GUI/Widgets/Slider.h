// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <limits.h>
#include <map>
#include <GUI/Widgets/Control.h>


class PTextView;


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
    PSlider(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = PSliderFlags::TicksBelow,
           int tickCount = 10, POrientation orientation = POrientation::Horizontal);
    PSlider(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    ~PSlider();

    // From View:
    virtual void OnFlagsChanged(uint32_t changedFlags) override { PControl::OnFlagsChanged(changedFlags); PreferredSizeChanged(); Invalidate(); }

    // From Control:
    virtual void OnLabelChanged(const PString& label) override;

    // From Slider:
    virtual void RenderLabels();
    virtual void RenderSlider();
    virtual void RenderKnob(PStandardColorID knobColor, float value);
    virtual void RenderTicks();
    
    virtual float PosToVal(const PPoint& position) const;
    virtual PPoint ValToPos(float value) const;

    enum class GetKnobFrameMode {FullFrame, SquareFrame};
    virtual PRect    GetKnobFrame(POrientation orientation, GetKnobFrameMode mode) const;
    virtual PRect    GetSliderFrame(PRect* outTotalFrame = nullptr, float* minimumLength = nullptr) const;

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
    virtual void    SetMinMax(float min, float max ) { m_Min = min; m_Max = max; }
    void            SetResolution(float resolution);

    void            SetShadowKnobsCount(size_t count);
    size_t          GetShadowKnobsCount() const;
    void            SetShadowKnobValue(size_t index, float value);

    bool            IsBeingDragged() const { return m_HitButton != PMouseButton::None; }
    // From Control:
    virtual void    OnEnableStatusChanged(bool isEnabled) override;
    
    // From View
    virtual void AttachedToScreen() override;
    virtual bool OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void OnPaint(const PRect& updateRect) override;
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;


    Signal<void, float, PSlider*, PMouseButton> SignalBeginDrag;
    Signal<void, float, PSlider*, PMouseButton> SignalEndDrag;
    Signal<void, float, bool, PSlider*>          SignalValueChanged;//(float value, bool finalUpdate, os::Slider* slider)
private:
    void UpdateValueView();
    void LayoutValueView();
    void RefreshDisplay();

    Ptr<PTextView>   m_ValueView;

    PString         m_MinLabel;
    PString         m_MaxLabel;
    PString         m_ValueFormat;
    float           m_ValueScale = 1.0f;

    PColor           m_SliderColor1;
    PColor           m_SliderColor2;
    float           m_SliderSize = 5.0f;
    int             m_NumSteps = 0;
    int             m_NumTicks;
    float           m_Min = 0.0f;
    float           m_Max = 1.0f;
    float           m_Resolution = 0.0f;
    float           m_SmallStep = 0.05f;
    float           m_BigStep = 0.1f;
    float           m_DragScale = 1.0f;
    float           m_DragScaleRange = 100.0f;
    POrientation     m_Orientation;
    float           m_Value = 0.0f;
    
    std::vector<float>  m_ShadowArrows;

    bool            m_Changed = false;
    PMouseButton   m_HitButton = PMouseButton::None;
    PPoint           m_HitPos;
    float           m_HitValue = 0.0f;
    PPoint           m_SmoothedPos;
};
