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

class PListView;
class PListViewScrolledView;

class PListViewHeaderView : public PView
{
public:
    friend class PListView;
    friend class PListViewRow;
    friend class PListViewColumnView;

    PListViewHeaderView(Ptr<PListView> parent);

private:
    void  DrawButton(const char* title, const PRect& frame, Ptr<PFont> font, PFontHeight* fontHeight);
    virtual void    OnPaint(const PRect& updateRect) override;

    virtual bool    OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual void    OnFrameSized(const PPoint& deltaSize) override;
    virtual void    OnViewScrolled(const PPoint& delta) override;

    virtual bool    HasFocus(PMouseButton button) const override;

    void    Layout();

    Ptr<PListViewScrolledView>  m_ScrolledContainerView;

    size_t                  m_SizeColumn = INVALID_INDEX;
    size_t                  m_DragColumn = INVALID_INDEX;
    PPoint                   m_HitPos;
    float                   m_HeaderHeight;
};
