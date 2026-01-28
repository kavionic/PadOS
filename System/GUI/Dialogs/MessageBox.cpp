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

#include <GUI/Dialogs/MessageBox.h>
#include <GUI/Widgets/TextView.h>

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MessageBox::MessageBox(const PString& title, const PString& text, DialogButtonSets buttonSet) : DialogBase(title, text, buttonSet)
{
    m_MessageView = ptr_new<TextView>("TextView", text, nullptr, TextViewFlags::MultiLine);
    m_MessageView->SetWidthOverride(PrefSizeType::Smallest, SizeOverride::Extend, 400.0f);
    SetContentView(m_MessageView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MessageBox::~MessageBox()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<MessageBox> MessageBox::ShowMessage(const PString& title, const PString& text, DialogButtonSets buttonSet)
{
    try
    {
        Ptr<MessageBox> dialog = ptr_new<MessageBox>(title, text, buttonSet);
        dialog->Open();
        return dialog;
    }
    catch (const std::bad_alloc& error)
    {
        set_last_error(ENOMEM);
        return nullptr;
    }
}

DialogButtonID MessageBox::ShowMessageSync(Ptr<View> owner, const PString& title, const PString& text, DialogButtonSets buttonSet)
{
    try
    {
        Ptr<MessageBox> dialog = ptr_new<MessageBox>(title, text, buttonSet);
        return dialog->Go(owner);
    }
    catch (const std::bad_alloc& error)
    {
        set_last_error(ENOMEM);
        return DialogButtonID::None;
    }
}

} //namespace os
