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

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TextInputDialog::TextInputDialog(const PString& title, const PString& message, const PString& text, DialogButtonSets buttonSet) : DialogBase(title, text, buttonSet)
{
    m_MessageView = ptr_new<TextView>("Message", message, nullptr, TextViewFlags::MultiLine);
    m_TextInput   = ptr_new<TextBox>("Input", text);

    m_MessageView->SetWidthOverride(PrefSizeType::Smallest, SizeOverride::Extend, 400.0f);
    m_TextInput->SetWidthOverride(PrefSizeType::Smallest, SizeOverride::Always, 0.0f);
    m_TextInput->SetWidthOverride(PrefSizeType::Greatest, SizeOverride::Always, COORD_MAX);

    m_TextInput->SetBorders(0.0f, 10.0f, 0.0f, 10.0f);

    m_TextInput->SignalTextChanged.Connect(this, &TextInputDialog::SlotTextChanged);

    Ptr<View> contentView = ptr_new<View>("Content");
    contentView->AddChild(m_MessageView);
    contentView->AddChild(m_TextInput);
    contentView->SetLayoutNode(ptr_new<VLayoutNode>());
    SetContentView(contentView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextInputDialog::OnActivated(DialogButtonID buttonID)
{
    SignalSelected(buttonID, m_TextInput->GetText(), this);
}

const PString& TextInputDialog::GetText() const
{
    return m_TextInput->GetText();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TextInputDialog::SlotTextChanged(const PString& newText, bool finalUpdate, TextBox* source)
{
    SignalTextChanged(newText, finalUpdate, this);
}

} //namespace os
