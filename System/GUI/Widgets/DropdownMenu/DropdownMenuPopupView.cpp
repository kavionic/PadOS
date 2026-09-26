// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Widgets/DropdownMenu.h>
#include "DropdownMenuPopupView.h"

namespace osi
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupWindow::DropdownMenuPopupWindow(const std::vector<PString>& itemList, size_t selection) : PView(PString::zero, nullptr, PViewFlags::WillDraw)
{
    m_ContentView = ptr_new<DropdownMenuPopupView>(itemList, selection, SignalSelectionChanged);
    m_ContentView->SetBorders(2.0f, 4.0f, 2.0f, 4.0f);
    m_ContentView->SignalPreferredSizeChanged.Connect(this, &PView::PreferredSizeChanged);
    SetScrolledView(m_ContentView);

    AddChild(m_ContentView);
    OnFrameSized(PPoint());
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnPaint(const PRect& updateRect)
{
    SetEraseColor(255, 255, 255);
    DrawFrame(GetBounds(), FRAME_RECESSED);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnFrameSized(const PPoint& delta)
{
    PRect contentFrame = GetBounds();
    PRect contentBorders = m_ContentView->GetBorders();
    contentFrame.Resize(contentBorders.left, contentBorders.top, contentBorders.right, contentBorders.bottom);
    m_ContentView->SetFrame(contentFrame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    *minSize = m_ContentView->GetPreferredSize(PPrefSizeType::Smallest);
    *maxSize = m_ContentView->GetPreferredSize(PPrefSizeType::Greatest);
    PRect  clientBorders = m_ContentView->GetBorders();
    PPoint borderSize(clientBorders.left + clientBorders.right, clientBorders.top + clientBorders.bottom);

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

DropdownMenuPopupView::DropdownMenuPopupView(const std::vector<PString>& itemList, size_t selection, Signal<void, size_t, bool>& signalSelectionChanged)
    : PView("drop_down_view", nullptr, PViewFlags::WillDraw)
    , SignalSelectionChanged(signalSelectionChanged)
    , m_ItemList(itemList)
{
    m_CurSelection = selection;
    m_OldSelection = m_CurSelection;

    m_FontHeight = GetFontHeight();
    m_GlyphHeight = std::round(m_FontHeight.descender + m_FontHeight.ascender + m_FontHeight.line_gap);

    m_ContentSize.y = float(m_ItemList.size()) * m_GlyphHeight;

    for (size_t i = 0; i < m_ItemList.size(); ++i)
    {
        const float width = GetStringWidth(m_ItemList[i]);
        if (width > m_ContentSize.x) {
            m_ContentSize.x = width;
        }
    }
    m_ContentSize.x += 4.0f;

    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPaint(const PRect& updateRect)
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
            PRect itemFrame = GetBounds();

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
        MovePenTo(2.0f, std::round(y + m_FontHeight.ascender + m_FontHeight.line_gap * 0.5f));
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

void DropdownMenuPopupView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    *minSize = m_ContentSize;
    *maxSize = m_ContentSize;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (m_HitPointerID != PInvalidPointerID) {
            PView::OnPointerDown(pointerID, position, event, phase);
            return;
        }
        m_HitPointerID = pointerID;

        if (GetBounds().DoIntersect(position))
        {
            SignalSelectionChanged(m_CurSelection, true);
        }
        else
        {
            SignalSelectionChanged(m_OldSelection, true);
        }
        return;
    }

    if (m_HitPointerID != PInvalidPointerID) {
        PView::OnPointerDown(pointerID, position, event, phase);
        return;
    }
    m_HitPos = position;
    m_HitPointerID = pointerID;

    m_HitItem = PositionToIndex(position);

    PRect itemFrame = GetBounds();

    itemFrame.top = float(m_HitItem) * m_GlyphHeight;
    itemFrame.bottom = itemFrame.top + m_GlyphHeight;
    Invalidate(itemFrame);

    SetPointerCapture(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (GetBounds().DoIntersect(position))
        {
            SignalSelectionChanged(m_CurSelection, true);
        }
        return;
    }

    if (pointerID != m_HitPointerID) {
        PView::OnPointerUp(pointerID, position, event, phase);
        return;
    }

    m_HitPointerID = PInvalidPointerID;

    if (m_MouseMoved)
    {
        PViewScroller* viewScroller = PViewScroller::GetViewScroller(this);
        if (viewScroller != nullptr) {
            viewScroller->EndSwipe();
        }
        m_MouseMoved = false;
    }
    else
    {
        SignalSelectionChanged(PositionToIndex(position), true);
    }
    ReleasePointerCapture(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (!GetBounds().DoIntersect(position)) {
            return;
        }
        size_t newSelection = PositionToIndex(position);

        if (newSelection != m_CurSelection)
        {
            int prevSel = m_CurSelection;
            m_CurSelection = newSelection;
            PRect itemFrame = GetBounds();

            itemFrame.top = float(prevSel) * m_GlyphHeight;
            itemFrame.bottom = itemFrame.top + m_GlyphHeight;
            Invalidate(itemFrame);

            itemFrame.top = float(m_CurSelection) * m_GlyphHeight;
            itemFrame.bottom = itemFrame.top + m_GlyphHeight;

            Invalidate(itemFrame);
            SignalSelectionChanged(newSelection, false);
        }
        return;
    }

    if (pointerID != m_HitPointerID) {
        PView::OnPointerMove(pointerID, position, event, phase);
        return;
    }
    if (m_MouseMoved)
    {
        PViewScroller* viewScroller = PViewScroller::GetViewScroller(this);
        if (viewScroller != nullptr) {
            viewScroller->SwipeMove(position);
        }
        if (m_HitItem != INVALID_INDEX)
        {
            PRect itemFrame = GetBounds();

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
            PViewScroller* viewScroller = PViewScroller::GetViewScroller(this);
            if (viewScroller != nullptr) {
                viewScroller->BeginSwipe(position);
            }
            m_MouseMoved = true;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::MakeSelectionVisible()
{
    PRect bounds = GetBounds();

    const float itemTop = float(m_CurSelection) * m_GlyphHeight;
    const float itemBottom = itemTop + m_GlyphHeight;

    // NOTE: All scroll offsets are negative.
    const float maxScroll = bounds.Height() - GetContentSize().y;
    if (maxScroll < 0.0f)
    {
        const float offset = std::round((bounds.Height() + itemTop - itemBottom) * 0.5f - itemTop);
        ScrollTo(0.0f, std::clamp(offset, maxScroll, 0.0f));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t DropdownMenuPopupView::PositionToIndex(const PPoint& position)
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
