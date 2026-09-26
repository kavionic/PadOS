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


class PMessageBox : public PDialogBase
{
public:
    PMessageBox(const PString& title, const PString& text, PDialogButtonSets buttonSet = PDialogButtonSets::Ok);
    virtual ~PMessageBox();
    static Ptr<PMessageBox> ShowMessage(const PString& title, const PString& text, PDialogButtonSets buttonSet = PDialogButtonSets::Ok);
    static PDialogButtonID  ShowMessageSync(Ptr<PView> owner, const PString& title, const PString& text, PDialogButtonSets buttonSet = PDialogButtonSets::Ok);

    virtual void OnActivated(PDialogButtonID buttonID) override { SignalSelected(buttonID, this); }

    Signal<void, PDialogButtonID, PMessageBox*> SignalSelected;//(DialogButtonID buttonID, MessageBox* dialog)

private:
    Ptr<PTextView> m_MessageView;
};
