// This file is part of PadOS.
//
// Copyright (c) 2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 15.04.2021 23:40

#include <GUI/Dialogs/ErrorMessageBox.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorMessageBox::PErrorMessageBox(const PString& title, const PString& text, PDialogButtonSets buttonSet) : PMessageBox(title, PString::vformat_string(text.c_str(), strerror(errno)), buttonSet)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PErrorMessageBox> PErrorMessageBox::ShowMessage(const PString& title, const PString& text, PDialogButtonSets buttonSet)
{
    try
    {
        Ptr<PErrorMessageBox> dlg = ptr_new<PErrorMessageBox>(title, text, buttonSet);
        dlg->Open();
        return dlg;
    }
    catch (const std::bad_alloc& error)
    {
        set_last_error(ENOMEM);
        return nullptr;
    }
}
