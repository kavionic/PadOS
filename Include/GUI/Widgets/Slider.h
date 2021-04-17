// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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


namespace os
{

class TextView;


namespace SliderFlags
{
static constexpr uint32_t TicksAbove        = 0x0001 << ViewFlags::FirstUserBit;
static constexpr uint32_t TicksBelow        = 0x0002 << ViewFlags::FirstUserBit;
static constexpr uint32_t TicksLeft         = TicksAbove;
static constexpr uint32_t TicksRight        = TicksBelow;
static constexpr uint32_t KnobPointUp       = 0x0004 << ViewFlags::FirstUserBit;
static constexpr uint32_t KnobPointDown     = 0x0008 << ViewFlags::FirstUserBit;
static constexpr uint32_t KnobPointLeft     = KnobPointUp;
static constexpr uint32_t KnobPointRight    = KnobPointDown;

extern const std::map<String, uint32_t> FlagMap;

}

///////////////////////////////////////////////////////////////////////////////
/// \ingroup gui
/// \par Description:
///
/// \sa os::Control
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class Slider : public Control
{
public:
    Slider(const String& name, Ptr<View> parent = nullptr, uint32_t flags = SliderFlags::TicksBelow,
           int tickCount = 10, Orientation orientation = Orientation::Horizontal);
    Slider(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);

    ~Slider();

    // From View:
    virtual void OnFlagsChanged(uint32_t changedFlags) override { Control::OnFlagsChanged(changedFlags); PreferredSizeChanged(); Invalidate(); }

    // From Control:
    virtual void OnLabelChanged(const String& label) override;

    // From Slider:
    virtual void RenderLabels();
    virtual void RenderSlider();
    virtual void RenderKnob(StandardColorID knobColor, float value);
    virtual void RenderTicks();
    
    virtual float PosToVal(const Point& position) const;
    virtual Point ValToPos(float value) const;

    enum class GetKnobFrameMode {FullFrame, SquareFrame};
    virtual Rect    GetKnobFrame(Orientation orientation, GetKnobFrameMode mode) const;
    virtual Rect    GetSliderFrame(Rect* outTotalFrame = nullptr, float* minimumLength = nullptr) const;

    virtual void    SetSliderColors(const Color& color1, const Color& color2);
    virtual void    GetSliderColors(Color* color1, Color* color2) const;
    virtual void    SetSliderSize(float size);
    virtual float   GetSliderSize() const;

    
    virtual void    SetValueStringFormat(const String& format, float scale = 1.0f);
    virtual String  GetValueStringFormat() const;
    virtual String  GetValueString() const;

    void        SetValue(float value, bool sendEvent = true);
    float       GetValue() const;

    void        SetStepCount(int count);
    int         GetStepCount() const;
    
    void        SetTickCount( int count);
    int         GetTickCount() const;

    void        SetLimitLabels(const String& minLabel, const String& maxLabel);
    void        GetLimitLabels(String* minLabel, String* maxLabel);
    
    virtual void    SetSteps(float small, float big)         { m_SmallStep = small; m_BigStep = big; }
    virtual void    GetSteps(float* small, float* big) const { *small = m_SmallStep; *big = m_BigStep; }
    void            SetDragScale(float scale) { m_DragScale = scale; }
    virtual void    SetMinMax(float min, float max ) { m_Min = min; m_Max = max; }
    
    void            SetShadowKnobsCount(size_t count);
    size_t          GetShadowKnobsCount() const;
    void            SetShadowKnobValue(size_t index, float value);

    bool            IsBeingDragged() const { return m_HitButton != MouseButton_e::None; }
    // From Control:
    virtual void    OnEnableStatusChanged(bool isEnabled) override;
    
    // From View
    virtual void AttachedToScreen() override;
    virtual bool OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual void FrameSized(const Point& delta) override;
    virtual void Paint(const Rect& updateRect) override;
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;


    Signal<void, float, Slider*, MouseButton_e> SignalBeginDrag;
    Signal<void, float, Slider*, MouseButton_e> SignalEndDrag;
    Signal<void, float, bool, Slider*>          SignalValueChanged;//(float value, bool finalUpdate, os::Slider* slider)
private:
    void UpdateValueView();
    void LayoutValueView();
    void RefreshDisplay();

    Ptr<TextView>   m_ValueView;

    String          m_MinLabel;
    String          m_MaxLabel;
    String          m_ValueFormat;
    float           m_ValueScale = 1.0f;

    Color           m_SliderColor1;
    Color           m_SliderColor2;
    float           m_SliderSize = 5.0f;
    int             m_NumSteps = 0;
    int             m_NumTicks;
    float           m_Min = 0.0f;
    float           m_Max = 1.0f;
    float           m_SmallStep = 0.05f;
    float           m_BigStep = 0.1f;
    float           m_DragScale = 1.0f;
    float           m_DragScaleRange = 100.0f;
    Orientation     m_Orientation;
    float           m_Value = 0.0f;
    
    std::vector<float>  m_ShadowArrows;

    bool            m_Changed = false;
    MouseButton_e   m_HitButton = MouseButton_e::None;
    Point           m_HitPos;
    float           m_HitValue = 0.0f;
    Point           m_SmoothedPos;
};

}
