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

#pragma once

#include <GUI/Dialogs/MessageBox.h>

namespace os
{

class ErrorMessageBox : public MessageBox
{
public:
    ErrorMessageBox(const String& title, const String& text, DialogButtonSets buttonSet = DialogButtonSets::Ok);

    static Ptr<ErrorMessageBox> ShowMessage(const String& title, const String& text, DialogButtonSets buttonSet = DialogButtonSets::Ok);
private:
};

} // namespace os
