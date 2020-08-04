// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#pragma once

#include "ListViewRow.h"
#include "Utils/String.h"

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class ListViewStringRow : public ListViewRow
{
public:
    ListViewStringRow() {}
    virtual ~ListViewStringRow() {}

    virtual void    AttachToView(Ptr<View> view, int column) override;
    virtual void    SetRect(const Rect& rect, size_t column) override;
    void            AppendString(const String& string);
    void            SetString(size_t index, const String& string);
    const String& GetString(size_t index) const;
    virtual float   GetWidth(Ptr<View> view, size_t column) override;
    virtual float   GetHeight(Ptr<View> view) override;
    virtual void    Paint(const Rect& frame, Ptr<View> view, size_t column, bool selected, bool highlighted, bool hasFocus) override;
    virtual bool    IsLessThan(Ptr<const ListViewRow> other, size_t column) const override;

private:
    std::vector<std::pair<String, float>> m_Strings;
};


} // namespace os

