// This file is part of PadOS.
//
// Copyright (C) 2020-2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 28.06.2020 12:56

#include <GUI/Widgets/ButtonBase.h>
#include <GUI/Widgets/ButtonGroup.h>
#include <Utils/XMLObjectParser.h>
#include <Utils/Beep.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonBase::PButtonBase(const PString& name, Ptr<PView> parent, uint32_t flags) : PControl(name, parent, flags)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonBase::PButtonBase(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData, PAlignment defaultLabelAlignment) : PControl(context, parent, xmlData, defaultLabelAlignment)
{
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

bool PButtonBase::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    //    printf("Button: Mouse down %d, %.1f/%.1f\n", int(button), position.x, position.y);

    if (!IsEnabled()) {
        return false;
    }

    if (m_HitButton == PMouseButton::None)
    {
        m_HitButton = button;
        SetPressedState(true);
        p_beep(PBeepLength::Short);
        if (m_CanBeCheked)
        {
            if (m_ButtonGroup != nullptr) {
                m_ButtonGroup->SelectButton(ptr_tmp_cast(this));
            } else {
                SetChecked(!m_IsChecked);
            }
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PButtonBase::OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (!IsEnabled()) {
        return false;
    }
    //    printf("Button: Mouse up %d, %.1f/%.1f\n", int(button), position.x, position.y);
    if (button == m_HitButton)
    {
        m_HitButton = PMouseButton::None;
        if (m_IsPressed)
        {
            SetPressedState(false);
            if (!m_CanBeCheked) {
                SignalActivated(button, this);
            }
        }
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PButtonBase::OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (button == m_HitButton)
    {
        //        printf("Button: Mouse move %d, %.1f/%.1f\n", int(button), position.x, position.y);
        SetPressedState(GetBounds().DoIntersect(position));
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetChecked(bool isChecked)
{
    if (isChecked != m_IsChecked)
    {
        m_IsChecked = isChecked;
        OnCheckedStateChanged(m_IsChecked);
        SignalToggled(isChecked, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::SetPressedState(bool isPressed)
{
    if (isPressed != m_IsPressed)
    {
        m_IsPressed = isPressed;
        OnPressedStateChanged(m_IsPressed);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonBase::OnEnableStatusChanged(bool isEnabled)
{
    if (!isEnabled)
    {
        m_HitButton = PMouseButton::None;
        if (m_IsPressed)
        {
            SetPressedState(false);
        }
    }
    PControl::OnEnableStatusChanged(isEnabled);
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

void PButtonBase::SetButtonGroup(Ptr<PButtonGroup> group)
{
    m_ButtonGroup = group;
}
