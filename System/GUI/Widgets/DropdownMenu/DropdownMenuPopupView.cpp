// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/DropdownMenu.h>
#include "DropdownMenuPopupView.h"

namespace osi
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupWindow::DropdownMenuPopupWindow(const std::vector<PString>& itemList, size_t selection)
    : PStyledView(PString::zero, PViewFlag::WillDraw)
{
    m_ContentView = ptr_new<DropdownMenuPopupView>(itemList, selection, SignalSelectionChanged);
    m_ContentView->SignalPreferredSizeChanged.Connect(this, &PView::PreferredSizeChanged);
    SetScrolledView(m_ContentView);

    AddChild(m_ContentView);
    OnFrameSized(PPoint());
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnFontChanged(Ptr<const PFont> newFont)
{
    m_ContentView->SetFont(newFont);
    OnFrameSized(PPoint());
    PreferredSizeChanged();
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnStyleChanged()
{
    m_ContentView->OnStyleChanged();
    OnFrameSized(PPoint());
    PStyledView::OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->DropdownMenuPopupFramePaint(*this, GetBounds());
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnFrameSized(const PPoint& delta)
{
    const PGUIStyle* style = GetStyle();
    m_ContentView->SetFrame((style != nullptr) ? style->DropdownMenuPopupFrameGetContentFrame(*this) : GetBounds());
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        const PPoint contentSize = m_ContentView->GetPreferredSize(PPrefSizeType::Smallest);
        style->DropdownMenuPopupFrameGetPreferredSize(*this, contentSize, minSize, maxSize);
    }
    else
    {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
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

void DropdownMenuPopupWindow::ItemsChanged()
{
    m_ContentView->ItemsChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupView::DropdownMenuPopupView(
    const std::vector<PString>& itemList,
    size_t selection,
    Signal<void, size_t, bool>& signalSelectionChanged)
    : PStyledView("drop_down_view", PViewFlag::WillDraw)
    , SignalSelectionChanged(signalSelectionChanged)
    , m_PopupStyleData(itemList)
{
    m_PopupStyleData.Selection = selection;
    m_OldSelection = m_PopupStyleData.Selection;
    PreferredSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnFontChanged(Ptr<const PFont> newFont)
{
    NotifyStylePropertyChanged(PDropdownMenuPopupProperty::Font);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnStyleChanged()
{
    m_PopupStyleData.StyleData.Reset();
    ContentSizeChanged();
    PStyledView::OnStyleChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnFrameSized(const PPoint& delta)
{
    PStyledView::OnFrameSized(delta);
    NotifyStylePropertyChanged(PDropdownMenuPopupProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->DropdownMenuPopupPaint(*this, updateRect, m_PopupStyleData);
    } else {
        PView::OnPaint(updateRect);
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
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->DropdownMenuPopupGetPreferredSize(*this, m_PopupStyleData, minSize, maxSize);
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint DropdownMenuPopupView::CalculateContentSize() const
{
    const PGUIStyle* style = GetStyle();
    PPoint size;
    if (style != nullptr)
    {
        PPoint maximumSize;
        style->DropdownMenuPopupGetPreferredSize(*this, m_PopupStyleData, &size, &maximumSize);
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (GetStyle() == nullptr)
    {
        PView::OnPointerDown(pointerID, position, event, phase);
        return;
    }
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (m_HitPointerID != PInvalidPointerID)
        {
            PView::OnPointerDown(pointerID, position, event, phase);
            return;
        }
        m_HitPointerID = pointerID;

        if (GetBounds().DoIntersect(position)) {
            SignalSelectionChanged(m_PopupStyleData.Selection, true);
        } else {
            SignalSelectionChanged(m_OldSelection, true);
        }
        return;
    }

    if (m_HitPointerID != PInvalidPointerID)
    {
        PView::OnPointerDown(pointerID, position, event, phase);
        return;
    }
    m_HitPos = position;
    m_HitPointerID = pointerID;

    m_PopupStyleData.PressedItem = PositionToIndex(position);

    NotifyStylePropertyChanged(PDropdownMenuPopupProperty::Pressed);

    SetPointerCapture(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (GetStyle() == nullptr)
    {
        PView::OnPointerUp(pointerID, position, event, phase);
        return;
    }
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (GetBounds().DoIntersect(position)) {
            SignalSelectionChanged(m_PopupStyleData.Selection, true);
        }
        return;
    }

    if (pointerID != m_HitPointerID)
    {
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
    if (GetStyle() == nullptr)
    {
        PView::OnPointerMove(pointerID, position, event, phase);
        return;
    }
    if (event.ToolType == PMotionToolType::Mouse)
    {
        if (!GetBounds().DoIntersect(position)) {
            return;
        }
        size_t newSelection = PositionToIndex(position);

        if (newSelection != m_PopupStyleData.Selection)
        {
            m_PopupStyleData.Selection = newSelection;
            NotifyStylePropertyChanged(PDropdownMenuPopupProperty::Selection);
            SignalSelectionChanged(newSelection, false);
        }
        return;
    }

    if (pointerID != m_HitPointerID)
    {
        PView::OnPointerMove(pointerID, position, event, phase);
        return;
    }
    if (m_MouseMoved)
    {
        PViewScroller* viewScroller = PViewScroller::GetViewScroller(this);
        if (viewScroller != nullptr) {
            viewScroller->SwipeMove(position);
        }
        if (m_PopupStyleData.PressedItem != INVALID_INDEX)
        {
            m_PopupStyleData.PressedItem = INVALID_INDEX;
            NotifyStylePropertyChanged(PDropdownMenuPopupProperty::Pressed);
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
    const PGUIStyle* style = GetStyle();
    if (style == nullptr || m_PopupStyleData.Selection >= m_PopupStyleData.Items.size()) {
        return;
    }
    const PRect bounds = GetBounds();
    const PRect itemFrame = style->DropdownMenuPopupGetItemFrame(*this, m_PopupStyleData, m_PopupStyleData.Selection);

    // NOTE: All scroll offsets are negative.
    const float maxScroll = bounds.Height() - GetContentSize().y;
    if (maxScroll < 0.0f)
    {
        const float offset = std::round((bounds.Height() - itemFrame.Height()) * 0.5f - itemFrame.top);
        ScrollTo(0.0f, std::clamp(offset, maxScroll, 0.0f));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::ItemsChanged()
{
    NotifyStylePropertyChanged(PDropdownMenuPopupProperty::Items);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::NotifyStylePropertyChanged(PDropdownMenuPopupProperties properties)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr)
    {
        style->DropdownMenuPopupPropertyChanged(*this, m_PopupStyleData, properties);
    }
    else
    {
        ContentSizeChanged();
        PreferredSizeChanged();
        Invalidate();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t DropdownMenuPopupView::PositionToIndex(const PPoint& position)
{
    const PGUIStyle* style = GetStyle();
    return (style != nullptr) ? style->DropdownMenuPopupGetItemFromPosition(*this, m_PopupStyleData, position) : INVALID_INDEX;
}

} // namespace osi
