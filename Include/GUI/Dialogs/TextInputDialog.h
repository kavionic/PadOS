// This file is part of PadOS.
//
// Copyright (c) 2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
