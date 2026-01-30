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
