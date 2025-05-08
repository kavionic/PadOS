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

class ListView;
class ListViewScrolledView;

class ListViewHeaderView : public View
{
public:
    friend class ListView;
    friend class ListViewRow;
    friend class ListViewColumnView;

    ListViewHeaderView(Ptr<ListView> parent);

private:
    void  DrawButton(const char* title, const Rect& frame, Ptr<Font> font, FontHeight* fontHeight);
    virtual void    OnPaint(const Rect& updateRect) override;

    virtual bool    OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual void    OnFrameSized(const Point& deltaSize) override;
    virtual void    OnViewScrolled(const Point& delta) override;

    virtual bool    HasFocus(MouseButton_e button) const override;

    void    Layout();

    Ptr<ListViewScrolledView>  m_ScrolledContainerView;

    size_t                  m_SizeColumn = INVALID_INDEX;
    size_t                  m_DragColumn = INVALID_INDEX;
    Point                   m_HitPos;
    float                   m_HeaderHeight;
};


} // namespace os

