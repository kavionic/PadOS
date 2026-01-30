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

#include <GUI/Widgets/ListViewRow.h>
#include <Utils/String.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PListViewStringRow : public PListViewRow
{
public:
    PListViewStringRow() {}
    virtual ~PListViewStringRow() {}

    virtual void    AttachToView(Ptr<PView> view, int column) override;
    virtual void    SetRect(const PRect& rect, size_t column) override;
    void            AppendString(const PString& string);
    void            SetString(size_t index, const PString& string);
    const PString&  GetString(size_t index) const;
    virtual float   GetWidth(Ptr<PView> view, size_t column) override;
    virtual float   GetHeight(Ptr<PView> view) override;
    virtual void    Paint(const PRect& frame, Ptr<PView> view, size_t column, bool selected, bool highlighted, bool hasFocus) override;
    virtual bool    IsLessThan(Ptr<const PListViewRow> other, size_t column) const override;

private:
    std::vector<std::pair<PString, float>> m_Strings;
};
