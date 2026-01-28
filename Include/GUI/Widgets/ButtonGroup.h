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

#pragma once

#include <vector>

#include <Ptr/PtrTarget.h>
#include <Signals/SignalTarget.h>
#include <Signals/Signal.h>
#include <Utils/String.h>

namespace os
{

class ButtonBase;

class ButtonGroup : public PtrTarget, public SignalTarget
{
public:
    ButtonGroup(const PString& name = PString::zero, size_t reserveCount = 0);
    virtual ~ButtonGroup() override;

    const PString&  GetName() const { return m_Name; }

    size_t          AddButton(Ptr<ButtonBase> button);
    void            InsertButton(size_t index, Ptr<ButtonBase> button);
    Ptr<ButtonBase> RemoveButton(Ptr<ButtonBase> button);
    Ptr<ButtonBase> RemoveButtonAt(size_t index);

    Ptr<ButtonBase> SetSelectedIndex(size_t index, bool sendEvent = true);
    size_t          SelectButton(Ptr<ButtonBase> button, bool sendEvent = true);
    Ptr<ButtonBase> GetSelectedButton() const;
    int32_t         GetSelectedID() const;
    size_t          GetSelectedIndex() const;

    size_t          GetButtonIndex(Ptr<ButtonBase> button) const;

    size_t          GetButtonCount() const;
    Ptr<ButtonBase> GetButton(size_t index) const;

    Signal<void, size_t, int32_t, Ptr<ButtonBase>, Ptr<ButtonGroup>> SignalSelectionChanged;//(size_t index, int32_t ID, Ptr<ButtonBase> button, Ptr<ButtonGroup> group)
private:
    PString                             m_Name;
    std::vector<WeakPtr<ButtonBase>>    m_ButtonList;
};

} // namespace
