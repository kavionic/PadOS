// This file is part of PadOS.
//
// Copyright (c) 2021 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 04.04.2021 15:40

#include <GUI/Dialogs/MessageBox.h>
#include <GUI/Widgets/TextView.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMessageBox::PMessageBox(const PString& title, const PString& text, PDialogButtonSets buttonSet) : PDialogBase(title, text, buttonSet)
{
    m_MessageView = ptr_new<PTextView>("TextView", text, nullptr, PTextViewFlags::MultiLine);
    m_MessageView->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Extend, 400.0f);
    SetContentView(m_MessageView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMessageBox::~PMessageBox()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PMessageBox> PMessageBox::ShowMessage(const PString& title, const PString& text, PDialogButtonSets buttonSet)
{
    try
    {
        Ptr<PMessageBox> dialog = ptr_new<PMessageBox>(title, text, buttonSet);
        dialog->Open();
        return dialog;
    }
    catch (const std::bad_alloc& error)
    {
        set_last_error(ENOMEM);
        return nullptr;
    }
}

PDialogButtonID PMessageBox::ShowMessageSync(Ptr<PView> owner, const PString& title, const PString& text, PDialogButtonSets buttonSet)
{
    try
    {
        Ptr<PMessageBox> dialog = ptr_new<PMessageBox>(title, text, buttonSet);
        return dialog->Go(owner);
    }
    catch (const std::bad_alloc& error)
    {
        set_last_error(ENOMEM);
        return PDialogButtonID::None;
    }
}
