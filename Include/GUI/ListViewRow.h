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

#include "Ptr/PtrTarget.h"

namespace os
{

class View;
class Rect;
class Point;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class ListViewRow : public PtrTarget
{
public:
    ListViewRow();
    virtual ~ListViewRow();

    virtual void AttachToView(Ptr<View> view, int column) = 0;
    virtual void SetRect(const Rect& rect, size_t column) = 0;

    virtual float GetWidth(Ptr<View> view, size_t column) = 0;
    virtual float GetHeight(Ptr<View> view) = 0;
    virtual void  Paint(const Rect& frame, Ptr<View> view, size_t column, bool selected, bool highlighted, bool hasFocus) = 0;
    virtual bool  HitTest(Ptr<View> view, const Rect& frame, int column, const Point& pos);
    virtual bool  IsLessThan(Ptr<const ListViewRow> other, size_t column) const = 0;

    void      SetIsSelectable(bool selectable);
    bool      IsSelectable() const;
    bool      IsSelected() const;
    bool      IsHighlighted() const;
private:
    friend class ListView;
    friend class ListViewScrolledView;
    friend class ListViewColumnView;

    float   m_YPos = 0.0f;
    float   m_Height = 0.0f;
    bool    m_IsSelectable = true;
    bool    m_Selected = false;
    bool    m_Highlighted = false;
};

} // namespace os
