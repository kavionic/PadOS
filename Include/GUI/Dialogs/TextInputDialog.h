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
class TextBox;

class TextInputDialog : public DialogBase
{
public:
    TextInputDialog(const String& title, const String& message, const String& text, DialogButtonSets buttonSet = DialogButtonSets::Ok);
    virtual void OnActivated(DialogButtonID buttonID) override;

    const String& GetText() const;

    Signal<void, const String&, bool, TextInputDialog*> SignalTextChanged;//(const String& newText, bool finalUpdate, TextInputDialog* dialog)
    Signal<void, DialogButtonID, const String&, TextInputDialog*> SignalSelected;//(DialogButtonID buttonID, const String& text, TextInputDialog* dialog)

private:
    void SlotTextChanged(const String& newText, bool finalUpdate, TextBox* source);

    Ptr<TextView>   m_MessageView;
    Ptr<TextBox>    m_TextInput;
};

} // namespace os
