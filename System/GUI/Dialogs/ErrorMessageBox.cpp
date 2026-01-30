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
