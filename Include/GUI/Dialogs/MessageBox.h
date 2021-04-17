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

#include <GUI/Dialogs/DialogBase.h>

namespace os
{

class TextView;

class MessageBox : public DialogBase
{
public:
    MessageBox(const String& title, const String& text, DialogButtonSets buttonSet = DialogButtonSets::Ok);
    virtual ~MessageBox();
    static Ptr<MessageBox> ShowMessage(const String& title, const String& text, DialogButtonSets buttonSet = DialogButtonSets::Ok);
    static DialogButtonID  ShowMessageSync(Ptr<View> owner, const String& title, const String& text, DialogButtonSets buttonSet = DialogButtonSets::Ok);

    virtual void OnActivated(DialogButtonID buttonID) override { SignalSelected(buttonID, this); }

    Signal<void, DialogButtonID, MessageBox*> SignalSelected;//(DialogButtonID buttonID, MessageBox* dialog)

private:
    Ptr<TextView> m_MessageView;
};

} // namespace os
