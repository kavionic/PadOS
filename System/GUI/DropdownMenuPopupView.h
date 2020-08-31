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

#include <GUI/ScrollView.h>

namespace os
{
class DropdownMenu;
}

namespace osi
{
using namespace os;

class DropdownMenuPopupView;

class DropdownMenuPopupWindow : public ScrollView
{
public:
    DropdownMenuPopupWindow(const std::vector<String>& itemList, size_t selection);

    // From View:
    virtual void Paint(const Rect& updateRect) override;

    void MakeSelectionVisible();

    Signal<void, size_t, bool>  SignalSelectionChanged;
private:
    Ptr<DropdownMenuPopupView> m_ContentView;
};


class DropdownMenuPopupView : public View
{
public:
    DropdownMenuPopupView(const std::vector<String>& itemList, size_t selection, Signal<void, size_t, bool>& signalSelectionChanged);

    // From View:
    virtual void    Paint(const Rect& updateRect) override;

    virtual bool    OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;

    virtual bool    OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual void    Activated(bool isActive);

    virtual void    CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;
    virtual Point   GetContentSize() const override { return m_ContentSize; }

    void MakeSelectionVisible();

    Signal<void, size_t, bool>&  SignalSelectionChanged;
private:
    size_t PositionToIndex(const Point& position);

    Point                   m_ContentSize;
    FontHeight              m_FontHeight;
    float                   m_GlyphHeight;
    size_t                  m_OldSelection;
    size_t                  m_CurSelection;
    size_t                  m_HitItem = INVALID_INDEX;
    MouseButton_e           m_HitButton = MouseButton_e::None;
    Point                   m_HitPos;
    bool                    m_MouseMoved = false;
    const std::vector<String>& m_ItemList;
};

} // namespace osi
