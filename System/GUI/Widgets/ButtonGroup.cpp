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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonGroup::PButtonGroup(const PString& name, size_t reserveCount) : m_Name(name)
{
    if (reserveCount != 0) {
        m_ButtonList.reserve(reserveCount);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PButtonGroup::~PButtonGroup()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PButtonGroup::AddButton(Ptr<PButtonBase> button)
{
    size_t index = m_ButtonList.size();
    InsertButton(index, button);
    return index;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PButtonGroup::InsertButton(size_t index, Ptr<PButtonBase> button)
{
    Ptr<PButtonGroup> prevGroup = button->GetButtonGroup();
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

Ptr<PButtonBase> PButtonGroup::RemoveButton(Ptr<PButtonBase> button)
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

Ptr<PButtonBase> PButtonGroup::RemoveButtonAt(size_t index)
{
    if (index < m_ButtonList.size())
    {
        Ptr<PButtonBase> button = m_ButtonList[index].Lock();
        m_ButtonList.erase(m_ButtonList.begin() + index);
        button->SetButtonGroup(nullptr);
        return button;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButtonBase> PButtonGroup::SetSelectedIndex(size_t index, bool sendEvent)
{
    Ptr<PButtonBase> button = GetButton(index);
    SelectButton(button, sendEvent);

    return button;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PButtonGroup::SelectButton(Ptr<PButtonBase> button, bool sendEvent)
{
    size_t  prevSelection = GetSelectedIndex();
    size_t  selectedIndex = INVALID_INDEX;

    for (size_t i = 0; i < m_ButtonList.size(); ++i)
    {
        Ptr<PButtonBase> curButton = m_ButtonList[i].Lock();
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
        SignalSelectionChanged(selectedIndex, (button != nullptr) ? button->GetID() : PControl::INVALID_ID, button, ptr_tmp_cast(this));
    }
    return selectedIndex;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButtonBase> PButtonGroup::GetSelectedButton() const
{
    size_t index = GetSelectedIndex();

    return (index != INVALID_INDEX) ? GetButton(index) : nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int32_t PButtonGroup::GetSelectedID() const
{
    Ptr<PButtonBase> button = GetSelectedButton();
    return (button != nullptr) ? button->GetID() : PControl::INVALID_ID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PButtonGroup::GetSelectedIndex() const
{
    for (size_t i = 0; i < m_ButtonList.size(); ++i)
    {
        Ptr<PButtonBase> button = m_ButtonList[i].Lock();
        if (button != nullptr && button->IsChecked()) {
            return i;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PButtonGroup::GetButtonIndex(Ptr<PButtonBase> button) const
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

size_t PButtonGroup::GetButtonCount() const
{
    return m_ButtonList.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PButtonBase> PButtonGroup::GetButton(size_t index) const
{
    if (index < m_ButtonList.size()) {
        return m_ButtonList[index].Lock();
    }
    return nullptr;
}

