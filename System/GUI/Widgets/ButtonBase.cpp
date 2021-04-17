// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ButtonBase::ButtonBase(const String& name, Ptr<View> parent, uint32_t flags) : Control(name, parent, flags)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ButtonBase::ButtonBase(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData, Alignment defaultLabelAlignment) : Control(context, parent, xmlData, defaultLabelAlignment)
{
	String groupName = context->GetAttribute(xmlData, "group", String::zero);

	if (!groupName.empty())
	{
		Ptr<ButtonGroup> group = context->GetButtonGroup(groupName);
		if (group != nullptr) {
			group->AddButton(ptr_tmp_cast(this));
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ButtonBase::~ButtonBase()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ButtonGroup> ButtonBase::FindButtonGroup(Ptr<View> root, const String& name)
{
	for (const Ptr<View>& child : root->GetChildList())
	{
		Ptr<ButtonBase> button = ptr_dynamic_cast<ButtonBase>(child);
		if (button != nullptr)
		{
			Ptr<ButtonGroup> group = button->GetButtonGroup();
			if (group != nullptr && group->GetName() == name) {
				return group;
			}
		}
	}
	for (const Ptr<View>& child : root->GetChildList())
	{
		Ptr<ButtonGroup> group = FindButtonGroup(child, name);
		if (group != nullptr) {
			return group;
		}
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ButtonBase::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
	//    printf("Button: Mouse down %d, %.1f/%.1f\n", int(button), position.x, position.y);

    if (!IsEnabled()) {
        return false;
    }

	if (m_HitButton == MouseButton_e::None)
	{
		m_HitButton = button;
		SetPressedState(true);
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

bool ButtonBase::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (!IsEnabled()) {
        return false;
    }
    //    printf("Button: Mouse up %d, %.1f/%.1f\n", int(button), position.x, position.y);
	if (button == m_HitButton)
	{
		m_HitButton = MouseButton_e::None;
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

bool ButtonBase::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
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

void ButtonBase::SetChecked(bool isChecked)
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

void ButtonBase::SetPressedState(bool isPressed)
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

void ButtonBase::OnEnableStatusChanged(bool isEnabled)
{
    if (!isEnabled)
    {
        m_HitButton = MouseButton_e::None;
        if (m_IsPressed)
        {
            SetPressedState(false);
        }
    }
    Control::OnEnableStatusChanged(isEnabled);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ButtonGroup> ButtonBase::GetButtonGroup() const
{
	return m_ButtonGroup;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ButtonBase::SetButtonGroup(Ptr<ButtonGroup> group)
{
	m_ButtonGroup = group;
}
