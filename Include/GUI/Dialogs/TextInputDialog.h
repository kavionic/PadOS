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

class PTextView;


class PTextBox;

class PTextInputDialog : public PDialogBase
{
public:
    PTextInputDialog(const PString& title, const PString& message, const PString& text, PDialogButtonSets buttonSet = PDialogButtonSets::Ok);
    virtual void OnActivated(PDialogButtonID buttonID) override;

    const PString& GetText() const;

    Signal<void, const PString&, bool, PTextInputDialog*> SignalTextChanged;//(const PString& newText, bool finalUpdate, TextInputDialog* dialog)
    Signal<void, PDialogButtonID, const PString&, PTextInputDialog*> SignalSelected;//(DialogButtonID buttonID, const PString& text, TextInputDialog* dialog)

private:
    void SlotTextChanged(const PString& newText, bool finalUpdate, PTextBox* source);

    Ptr<PTextView>   m_MessageView;
    Ptr<PTextBox>    m_TextInput;
};
