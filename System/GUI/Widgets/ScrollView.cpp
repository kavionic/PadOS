// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 29.08.2020 00:30

#include <GUI/Widgets/ScrollView.h>
#include <GUI/Widgets/ScrollableView.h>
#include <GUI/ViewFactory.h>

#include <cmath>

static constexpr float SCROLL_VIEW_WHEEL_SCROLL_STEP = 32.0f;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollView::PScrollView(const PString& name, Ptr<PView> parent, uint32_t flags) : PView(name, parent, flags | PViewFlags::WillDraw)
{
    EnableEventPhase(PEventPhase::Capture, true);
    EnableEventPhase(PEventPhase::Bubble, true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollView::PScrollView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PView(context, parent, xmlData)
{
    MergeFlags(PViewFlags::WillDraw);
    EnableEventPhase(PEventPhase::Capture, true);
    EnableEventPhase(PEventPhase::Bubble, true);

    for (pugi::xml_node childNode = xmlData.first_child(); childNode; childNode = childNode.next_sibling())
    {
        if (strcmp(childNode.name(), "_ScrollContent") == 0)
        {
            Ptr<PView> contentView;
            if (childNode.first_child())
            {
                contentView = PViewFactory::Get().CreateView(context, nullptr, childNode);
                if (contentView != nullptr)
                {
                    Ptr<PScrollableView> scrollableView = ptr_new<PScrollableView>();

                    scrollableView->SetHAlignment(PAlignment::Stretch);
                    scrollableView->SetVAlignment(PAlignment::Stretch);

                    if (contentView->GetLayoutNode() == nullptr) {
                        contentView->SetLayoutNode(ptr_new<PLayoutNode>());
                    }
                    scrollableView->MergeFlags(PViewFlags::WillDraw);
                    contentView->MergeFlags(PViewFlags::WillDraw);

                    scrollableView->SetContentView(contentView);
                    SetScrolledView(scrollableView);
                    break;
                }
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollView::OnLayoutChanged()
{
    Ptr<PView> clientView = GetScrolledView();
    if (clientView != nullptr)
    {
        PRect       clientFrame = GetBounds();
        const PRect clientBorders = clientView->GetBorders();
        clientFrame.Resize(clientBorders.left, clientBorders.top, clientBorders.right, clientBorders.bottom);
        clientView->SetFrame(clientFrame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollView::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Capture)
    {
        BeginPointerScrollTracking(pointerID, position);
        return;
    }
    if (phase != PEventPhase::Target) {
        return;
    }
    if (!BeginPointerScrollTracking(pointerID, position)) {
        return;
    }
    if (!SetPointerCapture(pointerID))
    {
        EndPointerScrollTracking(pointerID);
        return;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollView::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase != PEventPhase::Target && phase != PEventPhase::Capture) {
        return;
    }
    EndPointerScrollTracking(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollView::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Capture)
    {
        if (pointerID == m_HitPointerID && !HasPointerCapture(pointerID) && ShouldCaptureScroll(position)) {
            SetPointerCapture(pointerID);
        }
        return;
    }
    if (phase != PEventPhase::Target) {
        return;
    }
    if (pointerID != m_HitPointerID) {
        return;
    }
    SwipeMove(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PScrollView::OnPointerWheel(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Capture) {
        return event.WheelDelta;
    }

    PPoint scrollDelta;
    if (CanScrollHorizontally()) {
        scrollDelta.x = event.WheelDelta.x * SCROLL_VIEW_WHEEL_SCROLL_STEP;
    }
    if (CanScrollVertically()) {
        scrollDelta.y = event.WheelDelta.y * SCROLL_VIEW_WHEEL_SCROLL_STEP;
    }
    if (scrollDelta.x == 0.0f && scrollDelta.y == 0.0f) {
        return event.WheelDelta;
    }

    const PPoint consumedScrollDelta = ScrollScrolledViewBy(scrollDelta);
    PPoint remainingWheelDelta = event.WheelDelta;
    if (scrollDelta.x != 0.0f) {
        remainingWheelDelta.x -= consumedScrollDelta.x / SCROLL_VIEW_WHEEL_SCROLL_STEP;
    }
    if (scrollDelta.y != 0.0f) {
        remainingWheelDelta.y -= consumedScrollDelta.y / SCROLL_VIEW_WHEEL_SCROLL_STEP;
    }
    return remainingWheelDelta;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollView::OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason)
{
    // PointerUp and PointerCancel stop tracking before capture is released.
    // A remaining match is an interrupted gesture that cannot receive more input.
    EndPointerScrollTracking(pointerID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    Ptr<PView> clientView = GetScrolledView();
    if (clientView != nullptr)
    {
        *minSize = clientView->GetPreferredSize(PPrefSizeType::Smallest);
        *maxSize = clientView->GetPreferredSize(PPrefSizeType::Greatest);
        const PRect  clientBorders = clientView->GetBorders();
        const PPoint borderSize(clientBorders.left + clientBorders.right, clientBorders.top + clientBorders.bottom);

        *minSize += borderSize;
        *maxSize += borderSize;
    }
    else
    {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PScrollView::SetScrolledView(Ptr<PView> view)
{
    Ptr<PView> prevClient = PViewScroller::SetScrolledView(view);
    if (prevClient == view) {
        return prevClient;
    }
    if (prevClient != nullptr) {
        RemoveChild(prevClient);
    }
    if (view != nullptr) {
        AddChild(view);
    }
    InvalidateLayout();
    return prevClient;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollView::CanScrollHorizontally() const
{
    Ptr<PView> clientView = GetScrolledView();
    if (clientView == nullptr) {
        return false;
    }
    return clientView->GetContentSize().x > clientView->GetBounds().Width();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollView::CanScrollVertically() const
{
    Ptr<PView> clientView = GetScrolledView();
    if (clientView == nullptr) {
        return false;
    }
    return clientView->GetContentSize().y > clientView->GetBounds().Height();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollView::ShouldCaptureScroll(const PPoint& position) const
{
    const bool canScrollHorizontally = CanScrollHorizontally();
    const bool canScrollVertically = CanScrollVertically();
    if (!canScrollHorizontally && !canScrollVertically) {
        return false;
    }

    const PPoint delta = position - m_HitPos;
    const float absoluteX = std::abs(delta.x);
    const float absoluteY = std::abs(delta.y);
    const float threshold = GetStartScrollThreshold();

    const bool horizontalIntent = canScrollHorizontally && absoluteX >= threshold && (!canScrollVertically || absoluteX >= absoluteY);
    const bool verticalIntent = canScrollVertically && absoluteY >= threshold && (!canScrollHorizontally || absoluteY > absoluteX);

    return horizontalIntent || verticalIntent;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollView::BeginPointerScrollTracking(PPointerID pointerID, const PPoint& position)
{
    if (m_HitPointerID != PInvalidPointerID) {
        return pointerID == m_HitPointerID;
    }
    if (!CanScrollHorizontally() && !CanScrollVertically()) {
        return false;
    }
    m_HitPointerID = pointerID;
    m_HitPos = position;

    BeginSwipe(position);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PScrollView::EndPointerScrollTracking(PPointerID pointerID)
{
    if (pointerID == m_HitPointerID)
    {
        m_HitPointerID = PInvalidPointerID;
        EndSwipe();
    }
}
