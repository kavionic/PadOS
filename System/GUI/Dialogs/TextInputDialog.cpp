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

#include <GUI/Dialogs/TextInputDialog.h>
#include <GUI/Widgets/TextView.h>
#include <GUI/Widgets/TextBox.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTextInputDialog::PTextInputDialog(const PString& title, const PString& message, const PString& text, PDialogButtonSets buttonSet) : PDialogBase(title, text, buttonSet)
{
    m_MessageView = ptr_new<PTextView>("Message", message, nullptr, PTextViewFlags::MultiLine);
    m_TextInput   = ptr_new<PTextBox>("Input", text);

    m_MessageView->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Extend, 400.0f);
    m_TextInput->SetWidthOverride(PPrefSizeType::Smallest, PSizeOverride::Always, 0.0f);
    m_TextInput->SetWidthOverride(PPrefSizeType::Greatest, PSizeOverride::Always, COORD_MAX);

    m_TextInput->SetBorders(0.0f, 10.0f, 0.0f, 10.0f);

    m_TextInput->SignalTextChanged.Connect(this, &PTextInputDialog::SlotTextChanged);

    Ptr<PView> contentView = ptr_new<PView>("Content");
    contentView->AddChild(m_MessageView);
    contentView->AddChild(m_TextInput);
    contentView->SetLayoutNode(ptr_new<PVLayoutNode>());
    SetContentView(contentView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextInputDialog::OnActivated(PDialogButtonID buttonID)
{
    SignalSelected(buttonID, m_TextInput->GetText(), this);
}

const PString& PTextInputDialog::GetText() const
{
    return m_TextInput->GetText();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTextInputDialog::SlotTextChanged(const PString& newText, bool finalUpdate, PTextBox* source)
{
    SignalTextChanged(newText, finalUpdate, this);
}
