// This file is part of PadOS.
//
// Copyright (c) 2020-2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 28.06.2020 12:56

#include <GUI/Widgets/ButtonBase.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <Utils/XMLObjectParser.h>
#include <Utils/Beep.h>
#include <GUI/Style/ViewRendererButtonBase.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonBase::PButtonBase(const PString& name, PViewFlags flags) : PControl(name, flags), PViewStyleButtonBase(this)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonBase::PButtonBase(PViewFactoryContext& context, const pugi::xml_node& xmlData, PAlignment defaultLabelAlignment)
    : PControl(context, xmlData), PViewStyleButtonBase(this)
{
    m_ButtonStyleData.Label = context.GetAttribute(xmlData, "label", PString::zero);
    m_ButtonStyleData.LabelAlignment = context.GetAttribute(xmlData, "label_alignment", defaultLabelAlignment);

    PString groupName = context.GetAttribute(xmlData, "group", PString::zero);

    if (!groupName.empty())
    {
        Ptr<PButtonGroup> group = context.GetButtonGroup(groupName);
        if (group != nullptr) {
            group->AddButton(ptr_tmp_cast(this));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonBase::~PButtonBase()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButtonGroup> PButtonBase::FindButtonGroup(Ptr<PView> root, const PString& name)
{
    for (const Ptr<PView>& child : root->GetChildList())
    {
        Ptr<PButtonBase> button = ptr_dynamic_cast<PButtonBase>(child);
        if (button != nullptr)
        {
            Ptr<PButtonGroup> group = button->GetButtonGroup();
            if (group != nullptr && group->GetName() == name) {
                return group;
            }
        }
    }
    for (const Ptr<PView>& child : root->GetChildList())
    {
        Ptr<PButtonGroup> group = FindButtonGroup(child, name);
        if (group != nullptr) {
            return group;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnFontChanged(Ptr<const PFont> newFont)
{
    NotifyRendererPropertyChanged(PButtonProperty::Font);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnFrameSized(const PPoint& delta)
{
    PControl::OnFrameSized(delta);
    NotifyRendererPropertyChanged(PButtonProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    //    printf("Button: Mouse down %d, %.1f/%.1f\n", int(pointerID), position.x, position.y);

    if (phase == PEventPhase::Target
        && IsEnabled()
        && m_HitPointerID == PInvalidPointerID
        && SetPointerCapture(pointerID, PPointerCaptureMode::Preemptible))
    {
        m_HitPointerID = pointerID;
        SetPressedState(true);
        if (event.ToolType != PMotionToolType::Mouse) {
            p_beep(PBeepLength::Short);
        }
        if (m_ButtonStyleData.IsCheckable && event.ToolType == PMotionToolType::Mouse) {
            ActivateCheckableButton();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    //    printf("Button: Mouse up %d, %.1f/%.1f\n", int(pointerID), position.x, position.y);
    if (phase == PEventPhase::Target
        && IsEnabled()
        && pointerID == m_HitPointerID)
    {
        m_HitPointerID = PInvalidPointerID;
        if (m_ButtonStyleData.IsPressed)
        {
            SetPressedState(false);
            if (!m_ButtonStyleData.IsCheckable) {
                SignalActivated(pointerID, this);
            } else if (event.ToolType != PMotionToolType::Mouse) {
                ActivateCheckableButton();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Target && pointerID == m_HitPointerID)
    {
        //        printf("Button: Mouse move %d, %.1f/%.1f\n", int(pointerID), position.x, position.y);
        SetPressedState(GetBounds().DoIntersect(position));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnPointerOver(PPointerID pointerID, const PPoint& position, const PPointerEvent& pointerEvent, PEventPhase phase)
{
    if (phase == PEventPhase::Target && !m_ButtonStyleData.IsPointerOver)
    {
        m_ButtonStyleData.IsPointerOver = true;
        NotifyRendererPropertyChanged(PButtonProperty::Hovered);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnPointerOut(PPointerID pointerID, const PPoint& position, const PPointerEvent& pointerEvent, PEventPhase phase)
{
    if (phase == PEventPhase::Target && m_ButtonStyleData.IsPointerOver)
    {
        m_ButtonStyleData.IsPointerOver = false;
        NotifyRendererPropertyChanged(PButtonProperty::Hovered);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnPointerCaptureLost(
    PPointerID pointerID, PPointerCaptureLostReason reason)
{
    if (pointerID == m_HitPointerID)
    {
        m_HitPointerID = PInvalidPointerID;
        SetPressedState(false);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetLabel(const PString& label)
{
    if (label != m_ButtonStyleData.Label)
    {
        m_ButtonStyleData.Label = label;
        NotifyRendererPropertyChanged(PButtonProperty::Label);
        Flush();
        OnLabelChanged(m_ButtonStyleData.Label);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetLabelAlignment(PAlignment alignment)
{
    m_ButtonStyleData.LabelAlignment = alignment;
    NotifyRendererPropertyChanged(PButtonProperty::LabelAlignment);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetCheckable(bool value)
{
    if (m_ButtonStyleData.IsCheckable != value)
    {
        m_ButtonStyleData.IsCheckable = value;
        NotifyRendererPropertyChanged(PButtonProperty::Checkable);
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetChecked(bool isChecked)
{
    if (isChecked != m_ButtonStyleData.IsChecked)
    {
        m_ButtonStyleData.IsChecked = isChecked;
        NotifyRendererPropertyChanged(PButtonProperty::Checked);
        Flush();
        OnCheckedStateChanged(m_ButtonStyleData.IsChecked);
        SignalToggled(isChecked, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnEnableStatusChanged(bool isEnabled)
{
    if (!isEnabled)
    {
        m_HitPointerID = PInvalidPointerID;
        if (m_ButtonStyleData.IsPressed) {
            SetPressedState(false);
        }
    }
    NotifyRendererPropertyChanged(PButtonProperty::Enabled);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButtonGroup> PButtonBase::GetButtonGroup() const
{
    return m_ButtonGroup;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetPressedState(bool isPressed)
{
    if (isPressed != m_ButtonStyleData.IsPressed)
    {
        m_ButtonStyleData.IsPressed = isPressed;
        NotifyRendererPropertyChanged(PButtonProperty::Pressed);
        Flush();
        OnPressedStateChanged(m_ButtonStyleData.IsPressed);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::ActivateCheckableButton()
{
    if (m_ButtonGroup != nullptr) {
        m_ButtonGroup->SelectButton(ptr_tmp_cast(this));
    } else {
        SetChecked(!m_ButtonStyleData.IsChecked);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetButtonGroup(Ptr<PButtonGroup> group)
{
    m_ButtonGroup = group;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::NotifyRendererPropertyChanged(PButtonProperties properties)
{
    const PViewRendererButtonBase* renderer = GetRenderer();
    if (renderer != nullptr)
    {
        renderer->PropertyChanged(*this, properties, GetRendererContext());
    }
    else
    {
        if (properties.HasAny(PButtonProperties(PButtonProperty::Label, PButtonProperty::Font, PButtonProperty::LabelAlignment, PButtonProperty::Checkable))) {
            PreferredSizeChanged();
        }
        Invalidate();
    }
}
