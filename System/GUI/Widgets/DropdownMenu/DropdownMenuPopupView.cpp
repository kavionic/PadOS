// This file is part of PadOS.
//
// Copyright (c) 1999-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/DropdownMenu.h>
#include "DropdownMenuPopupView.h"
#include <GUI/Style/ViewRendererDropdownMenuPopup.h>
#include <GUI/Style/ViewRendererDropdownMenuPopupFrame.h>

namespace osi
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupWindow::DropdownMenuPopupWindow(const std::vector<PString>& itemList, size_t selection)
    : PView(PString::zero, PViewFlag::WillDraw), PViewStyleDropdownMenuPopupFrame(this)
{
    m_ContentView = ptr_new<DropdownMenuPopupView>(itemList, selection, SignalSelectionChanged);
    m_ContentView->SignalPreferredSizeChanged.Connect(this, &PView::PreferredSizeChanged);
    SetScrolledView(m_ContentView);

    AddChild(m_ContentView);
    OnFrameSized(PPoint());
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupWindow::~DropdownMenuPopupWindow()
{
    ReleaseRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint DropdownMenuPopupWindow::GetPopupContentSize() const
{
    return m_ContentView->GetPreferredSize(PPrefSizeType::Smallest);
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
    PView::OnStyleChanged();
    OnFrameSized(PPoint());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnPaint(const PRect& updateRect)
{
    const PViewRendererDropdownMenuPopupFrame* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::OnFrameSized(const PPoint& delta)
{
    const PViewRendererDropdownMenuPopupFrame* renderer = GetRenderer();
    m_ContentView->SetFrame((renderer != nullptr) ? renderer->GetContentFrame(*this, GetRendererContext()) : GetBounds());
    Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupWindow::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PViewRendererDropdownMenuPopupFrame* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
    } else {
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
    : PView("drop_down_view", PViewFlag::WillDraw), PViewStyleDropdownMenuPopup(this)
    , SignalSelectionChanged(signalSelectionChanged)
    , m_PopupStyleData(itemList)
{
    m_PopupStyleData.Selection = selection;
    m_OldSelection = m_PopupStyleData.Selection;
    InitializeRenderer();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

DropdownMenuPopupView::~DropdownMenuPopupView()
{
    ReleaseRenderer();
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
    PView::OnStyleChanged();
    ContentSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnFrameSized(const PPoint& delta)
{
    PView::OnFrameSized(delta);
    NotifyStylePropertyChanged(PDropdownMenuPopupProperty::Frame);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPaint(const PRect& updateRect)
{
    const PViewRendererDropdownMenuPopup* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->Paint(*this, GetBounds(), updateRect, GetRendererContext());
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
    const PViewRendererDropdownMenuPopup* renderer = GetRenderer();
    if (renderer != nullptr) {
        renderer->GetPreferredSize(*this, minSize, maxSize, GetRendererContext());
    } else {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint DropdownMenuPopupView::CalculateContentSize() const
{
    const PViewRendererDropdownMenuPopup* renderer = GetRenderer();
    PPoint size;
    if (renderer != nullptr)
    {
        PPoint maximumSize;
        renderer->GetPreferredSize(*this, &size, &maximumSize, GetRendererContext());
    }
    return size;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void DropdownMenuPopupView::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (GetRenderer() == nullptr)
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
    if (GetRenderer() == nullptr)
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
    if (GetRenderer() == nullptr)
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
    const PViewRendererDropdownMenuPopup* renderer = GetRenderer();
    if (renderer == nullptr || m_PopupStyleData.Selection >= m_PopupStyleData.Items.size()) {
        return;
    }
    const PRect bounds = GetBounds();
    const PRect itemFrame = renderer->GetItemFrame(*this, m_PopupStyleData.Selection, GetRendererContext());

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
    const PViewRendererDropdownMenuPopup* renderer = GetRenderer();
    if (renderer != nullptr)
    {
        renderer->PropertyChanged(*this, properties, GetRendererContext());
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
    const PViewRendererDropdownMenuPopup* renderer = GetRenderer();
    return (renderer != nullptr) ? renderer->GetItemFromPosition(*this, position, GetRendererContext()) : INVALID_INDEX;
}

} // namespace osi
