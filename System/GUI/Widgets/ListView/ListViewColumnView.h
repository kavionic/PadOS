// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
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

#include "GUI/View.h"

namespace os
{

class ListViewScrolledView;

class ListViewColumnView : public View
{
public:
    ListViewColumnView(Ptr<ListViewScrolledView> parent, const PString& title);
    ~ListViewColumnView();

    virtual void OnPaint(const Rect& updateRect) override;
    void         Refresh(const Rect& updateRect);
private:
    friend class ListViewHeaderView;
    friend class ListViewScrolledView;

    PString m_Title;
    float   m_ContentWidth;
};


} // namespace os

