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

#include <Ptr/PtrTarget.h>

class PRect;
class PPoint;
class PView;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PListViewRow : public PtrTarget
{
public:
    PListViewRow();
    virtual ~PListViewRow();

    virtual void AttachToView(Ptr<PView> view, int column) = 0;
    virtual void SetRect(const PRect& rect, size_t column) = 0;

    virtual float GetWidth(Ptr<PView> view, size_t column) = 0;
    virtual float GetHeight(Ptr<PView> view) = 0;
    virtual void  Paint(const PRect& frame, Ptr<PView> view, size_t column, bool selected, bool highlighted, bool hasFocus) = 0;
    virtual bool  HitTest(Ptr<PView> view, const PRect& frame, int column, const PPoint& pos);
    virtual bool  IsLessThan(Ptr<const PListViewRow> other, size_t column) const = 0;

    void      SetIsSelectable(bool selectable);
    bool      IsSelectable() const;
    bool      IsSelected() const;
    bool      IsHighlighted() const;
private:
    friend class PListView;
    friend class PListViewScrolledView;
    friend class PListViewColumnView;

    float   m_YPos = 0.0f;
    float   m_Height = 0.0f;
    bool    m_IsSelectable = true;
    bool    m_Selected = false;
    bool    m_Highlighted = false;
};
