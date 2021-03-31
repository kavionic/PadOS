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
// Created: 29.06.2020 19:45

#include <GUI/Widgets/ButtonGroup.h>
#include <GUI/Widgets/ButtonBase.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ButtonGroup::ButtonGroup(const String& name, size_t reserveCount) : m_Name(name)
{
	if (reserveCount != 0) {
		m_ButtonList.reserve(reserveCount);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ButtonGroup::~ButtonGroup()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t	ButtonGroup::AddButton(Ptr<ButtonBase> button)
{
	size_t index = m_ButtonList.size();
	InsertButton(index, button);
	return index;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ButtonGroup::InsertButton(size_t index, Ptr<ButtonBase> button)
{
	Ptr<ButtonGroup> prevGroup = button->GetButtonGroup();
	if (prevGroup == this) {
		return;
	} else if (prevGroup != nullptr) {
		prevGroup->RemoveButton(button);
	}

	m_ButtonList.insert(m_ButtonList.begin() + index, button);
	button->SetButtonGroup(ptr_tmp_cast(this));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ButtonBase> ButtonGroup::RemoveButton(Ptr<ButtonBase> button)
{
	size_t index = GetButtonIndex(button);
	if (index != INVALID_INDEX)
	{
		return RemoveButtonAt(index);
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ButtonBase> ButtonGroup::RemoveButtonAt(size_t index)
{
	if (index < m_ButtonList.size())
	{
		Ptr<ButtonBase> button = m_ButtonList[index].Lock();
		m_ButtonList.erase(m_ButtonList.begin() + index);
		button->SetButtonGroup(nullptr);
		return button;
	}
	return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ButtonBase>	ButtonGroup::SetSelectedIndex(size_t index, bool sendEvent)
{
	Ptr<ButtonBase> button = GetButton(index);
	SelectButton(button);

	return button;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ButtonGroup::SelectButton(Ptr<ButtonBase> button, bool sendEvent)
{
	size_t	prevSelection = GetSelectedIndex();
	size_t  selectedIndex = INVALID_INDEX;

	for (size_t i = 0; i < m_ButtonList.size(); ++i)
	{
		Ptr<ButtonBase> curButton = m_ButtonList[i].Lock();
		if (curButton != nullptr)
		{
			if (curButton == button) {
				selectedIndex = i;
				curButton->SetChecked(true);
			} else {
				curButton->SetChecked(false);
			}
		}
	}
	if (sendEvent && selectedIndex != prevSelection) {
		SignalSelectionChanged(selectedIndex, (button != nullptr) ? button->GetID() : Control::INVALID_ID, button, ptr_tmp_cast(this));
	}
	return selectedIndex;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ButtonBase> ButtonGroup::GetSelectedButton() const
{
	size_t index = GetSelectedIndex();

	return (index != INVALID_INDEX) ? GetButton(index) : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t ButtonGroup::GetSelectedID() const
{
	Ptr<ButtonBase> button = GetSelectedButton();
	return (button != nullptr) ? button->GetID() : Control::INVALID_ID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ButtonGroup::GetSelectedIndex() const
{
	for (size_t i = 0; i < m_ButtonList.size(); ++i)
	{
		Ptr<ButtonBase> button = m_ButtonList[i].Lock();
		if (button != nullptr && button->IsChecked()) {
			return i;
		}
	}
	return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ButtonGroup::GetButtonIndex(Ptr<ButtonBase> button) const
{
	for (size_t i = 0; i < m_ButtonList.size(); ++i)
	{
		if (button == m_ButtonList[i]) {
			return i;
		}
	}
	return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ButtonGroup::GetButtonCount() const
{
	return m_ButtonList.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ButtonBase> ButtonGroup::GetButton(size_t index) const
{
	if (index < m_ButtonList.size()) {
		return m_ButtonList[index].Lock();
	}
	return nullptr;
}

