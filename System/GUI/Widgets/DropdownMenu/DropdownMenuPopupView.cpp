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

#include <GUI/Widgets/DropdownMenu.h>
#include "DropdownMenuPopupView.h"

namespace os
{
namespace osi
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupWindow::DropdownMenuPopupWindow(const std::vector<String>& itemList, size_t selection) : View(String::zero, nullptr, ViewFlags::WillDraw)
{
    m_ContentView = ptr_new<DropdownMenuPopupView>(itemList, selection, SignalSelectionChanged);
    m_ContentView->SetBorders(2.0f, 4.0f, 2.0f, 4.0f);
    SetScrolledView(m_ContentView);

    AddChild(m_ContentView);
    FrameSized(Point());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::Paint(const Rect& updateRect)
{
    SetEraseColor(255, 255, 255);
    DrawFrame(GetBounds(), FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::FrameSized(const Point& delta)
{
    Rect contentFrame = GetBounds();
    Rect contentBorders = m_ContentView->GetBorders();
    contentFrame.Resize(contentBorders.left, contentBorders.top, contentBorders.right, contentBorders.bottom);
    m_ContentView->SetFrame(contentFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    *minSize = m_ContentView->GetPreferredSize(PrefSizeType::Smallest);
    *maxSize = m_ContentView->GetPreferredSize(PrefSizeType::Greatest);
    Rect  clientBorders = m_ContentView->GetBorders();
    Point borderSize(clientBorders.left + clientBorders.right, clientBorders.top + clientBorders.bottom);

    *minSize += borderSize;
    *maxSize += borderSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::MakeSelectionVisible()
{
    m_ContentView->MakeSelectionVisible();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupView::DropdownMenuPopupView(const std::vector<String>& itemList, size_t selection, Signal<void, size_t, bool>& signalSelectionChanged)
    : View("drop_down_view", nullptr, ViewFlags::WillDraw)
    , SignalSelectionChanged(signalSelectionChanged)
    , m_ItemList(itemList)
{
    m_CurSelection = selection;
    m_OldSelection = m_CurSelection;

    m_FontHeight = GetFontHeight();
    m_GlyphHeight = round(m_FontHeight.descender + m_FontHeight.ascender + m_FontHeight.line_gap);

    m_ContentSize.y = float(m_ItemList.size()) * m_GlyphHeight;

    for (size_t i = 0; i < m_ItemList.size(); ++i)
    {
        const float width = GetStringWidth(m_ItemList[i]);
        if (width > m_ContentSize.x) {
            m_ContentSize.x = width;
        }
    }
    m_ContentSize.x += 4.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::Paint(const Rect& updateRect)
{
    SetEraseColor(255, 255, 255);
    EraseRect(GetBounds());

    SetFgColor(0, 0, 0);
    SetBgColor(255, 255, 255);

    const size_t firstVisible = size_t(updateRect.top / m_GlyphHeight);
    const size_t lastVisible = std::min(size_t((updateRect.bottom + m_GlyphHeight - 1.0f) / m_GlyphHeight), m_ItemList.size() - 1);

    float y = float(firstVisible) * m_GlyphHeight;
    for (size_t i = firstVisible; i <= lastVisible; ++i)
    {
        if (i == m_CurSelection || i == m_HitItem)
        {
            Rect itemFrame = GetBounds();

            if (i == m_CurSelection) {
                SetFgColor(0, 0, 0);
            } else {
                SetFgColor(0, 0, 200);
            }

            itemFrame.top = y;
            itemFrame.bottom = itemFrame.top + m_GlyphHeight;
            FillRect(itemFrame);

            SetFgColor(255, 255, 255);
            if (i == m_CurSelection) {
                SetBgColor(0, 0, 0);
            } else {
                SetBgColor(0, 0, 200);
            }
        }
        MovePenTo(2.0f, round(y + m_FontHeight.ascender + m_FontHeight.line_gap * 0.5f));
        DrawString(m_ItemList[i]);
        y += m_GlyphHeight;

        if (i == m_CurSelection)
        {
            SetFgColor(0, 0, 0);
            SetBgColor(255, 255, 255);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::Activated(bool isActive)
{
    if (!isActive)
    {
        SignalSelectionChanged(m_OldSelection, true);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    *minSize = m_ContentSize;
    *maxSize = m_ContentSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DropdownMenuPopupView::OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (m_HitButton != MouseButton_e::None) {
        return View::OnTouchDown(pointID, position, event);
    }
    m_HitPos = position;
    m_HitButton = pointID;

    m_HitItem = PositionToIndex(position);

    Rect itemFrame = GetBounds();

    itemFrame.top = float(m_HitItem) * m_GlyphHeight;
    itemFrame.bottom = itemFrame.top + m_GlyphHeight;
    Invalidate(itemFrame);

    MakeFocus(pointID, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DropdownMenuPopupView::OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchUp(pointID, position, event);
    }

    m_HitButton = MouseButton_e::None;

    if (m_MouseMoved)
    {
        ViewScroller* viewScroller = ViewScroller::GetViewScroller(this);
        if (viewScroller != nullptr) {
            viewScroller->EndSwipe();
        }
        m_MouseMoved = false;
    }
    else
    {
        SignalSelectionChanged(PositionToIndex(position), true);
    }
    MakeFocus(pointID, false);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DropdownMenuPopupView::OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return View::OnTouchMove(pointID, position, event);
    }
    if (m_MouseMoved)
    {
        ViewScroller* viewScroller = ViewScroller::GetViewScroller(this);
        if (viewScroller != nullptr) {
            viewScroller->SwipeMove(position);
        }
        if (m_HitItem != INVALID_INDEX)
        {
            Rect itemFrame = GetBounds();

            itemFrame.top = float(m_HitItem) * m_GlyphHeight;
            itemFrame.bottom = itemFrame.top + m_GlyphHeight;
            m_HitItem = INVALID_INDEX;
            Invalidate(itemFrame);
        }
    }
    else
    {
        if ((position - m_HitPos).LengthSqr() > 20.0f * 20.0f)
        {
            ViewScroller* viewScroller = ViewScroller::GetViewScroller(this);
            if (viewScroller != nullptr) {
                viewScroller->BeginSwipe(position);
            }
            m_MouseMoved = true;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DropdownMenuPopupView::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (m_HitButton != MouseButton_e::None) {
        return View::OnMouseDown(button, position, event);
    }
    m_HitButton = button;

    if (GetBounds().DoIntersect(position))
    {
        SignalSelectionChanged(m_CurSelection, true);
    }
    else
    {
        SignalSelectionChanged(m_OldSelection, true);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DropdownMenuPopupView::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (GetBounds().DoIntersect(position))
    {
        SignalSelectionChanged(m_CurSelection, true);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool DropdownMenuPopupView::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (!GetBounds().DoIntersect(position)) {
        return false;
    }
    size_t newSelection = PositionToIndex(position);

    if (newSelection != m_CurSelection)
    {
        int prevSel = m_CurSelection;
        m_CurSelection = newSelection;
        Rect itemFrame = GetBounds();

        itemFrame.top = float(prevSel) * m_GlyphHeight;
        itemFrame.bottom = itemFrame.top + m_GlyphHeight;
        Invalidate(itemFrame);

        itemFrame.top = float(m_CurSelection) * m_GlyphHeight;
        itemFrame.bottom = itemFrame.top + m_GlyphHeight;

        Invalidate(itemFrame);
        SignalSelectionChanged(newSelection, false);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::MakeSelectionVisible()
{
    Rect bounds = GetBounds();

    const float itemTop = float(m_CurSelection) * m_GlyphHeight;
    const float itemBottom = itemTop + m_GlyphHeight;

    // NOTE: All scroll offsets are negative.
    const float maxScroll = bounds.Height() - GetContentSize().y;
    if (maxScroll < 0.0f)
    {
        const float offset = round((bounds.Height() + itemTop - itemBottom) * 0.5f - itemTop);
        ScrollTo(0.0f, std::clamp(offset, maxScroll, 0.0f));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t DropdownMenuPopupView::PositionToIndex(const Point& position)
{
    size_t newSelection = 0;
    if (position.y < 0.0f) {
        return 0;
    }
    newSelection = size_t(position.y / m_GlyphHeight);
    if (newSelection >= m_ItemList.size()) {
        newSelection = m_ItemList.size() - 1;
    }
    return newSelection;
}

} // namespace osi
} // namespace os

