// This file is part of PadOS.
//
// Copyright (C) 2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.04.2021 15:40

#pragma once

#include <GUI/Window.h>

namespace os
{

class Button;
class ButtonBase;
class DialogBoxView;


enum class DialogButtonID : int32_t
{
    None,
    Cancel,
    Ok,
    No = Cancel,
    Yes = Ok,
    FirstCustom = 100
};
enum class DialogButtonSets : int
{
    None,
    Ok,
    Cancel,
    OkCancel,
    YesNo
};

class DialogBase : public Window
{
public:
    DialogBase(const String& title, const String& text, DialogButtonSets buttonSet);
    virtual ~DialogBase();

    Ptr<View>   SetContentView(Ptr<View> contentView);
    Ptr<Button> AddButton(const String& label, DialogButtonID buttonID);
    Ptr<Button> AddButton(const String& label, int32_t buttonID);

    Ptr<Button> FindButton(DialogButtonID buttonID) const;
    Ptr<Button> FindButton(int32_t buttonID) const;
    Ptr<Button> FindButton(const String& buttonName) const;

    DialogButtonID Go(Ptr<View> owner);
protected:
    virtual void OnActivated(DialogButtonID buttonID) = 0;

private:
    void SlotSelected(DialogButtonID buttonID);

    Ptr<DialogBoxView> m_DialogView;
//    WeakPtr<View>      m_OwnerView;
    DialogButtonID     m_ButtonClicked = DialogButtonID::None;
};

} // namespace os
