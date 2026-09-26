// This file is part of PadOS.
//
// Copyright (c) 2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 15.04.2021 23:40

#pragma once

#include <GUI/Dialogs/MessageBox.h>


class PErrorMessageBox : public PMessageBox
{
public:
    PErrorMessageBox(const PString& title, const PString& text, PDialogButtonSets buttonSet = PDialogButtonSets::Ok);

    static Ptr<PErrorMessageBox> ShowMessage(const PString& title, const PString& text, PDialogButtonSets buttonSet = PDialogButtonSets::Ok);
private:
};
