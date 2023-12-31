// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.06.2020 16:30:00

#pragma once

#include <GUI/View.h>

namespace os
{

class GroupView : public View
{
public:
    GroupView(const String& name, Ptr<View> parent = nullptr, uint32_t flags = 0);
    GroupView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    // From View:
    virtual void Paint(const Rect& updateRect) override;

private:
    String  m_Label;
};

}
