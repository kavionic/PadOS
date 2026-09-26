// This file is part of PadOS.
//
// Copyright (c) 2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 29.08.2020 00:30

#include <GUI/ViewScroller.h>


namespace osi
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ViewScrollerSignalTarget::ViewScrollerSignalTarget()
{
    m_InertialScroller.SignalUpdate.Connect(this, &ViewScrollerSignalTarget::SlotInertialScrollUpdate);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> ViewScrollerSignalTarget::SetScrolledView(Ptr<PView> view)
{
    Ptr<PView> prevView = m_ScrolledView.Lock();
    if (prevView != nullptr)
    {
        prevView->SignalContentSizeChanged.Disconnect(this, &ViewScrollerSignalTarget::UpdateScroller);
        prevView->SignalFrameSized.Disconnect(this, &ViewScrollerSignalTarget::UpdateScroller);
        prevView->RemoveThis();
        prevView->ScrollTo(0.0f, 0.0f);
    }
    m_ScrolledView = view;
    if (view != nullptr)
    {
        view->SignalContentSizeChanged.Connect(this, &ViewScrollerSignalTarget::UpdateScroller);
        view->SignalFrameSized.Connect(this, &ViewScrollerSignalTarget::UpdateScroller);
        UpdateScroller();
    }
    return prevView;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::BeginSwipe(const PPoint& position)
{
    Ptr<PView> view = m_ScrolledView.Lock();
    if (view != nullptr)
    {
        const PRect bounds = view->GetBounds();
        const PPoint contentSize = view->GetContentSize();
        if (contentSize.x > bounds.Width() || contentSize.y > bounds.Height())
        {
            m_InertialScroller.BeginDrag(view->GetScrollOffset(), position);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::SwipeMove(const PPoint& position)
{
    m_InertialScroller.AddUpdate(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::EndSwipe()
{
    m_InertialScroller.EndDrag();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint ViewScrollerSignalTarget::ScrollBy(const PPoint& offset)
{
    PPoint consumedOffset;
    Ptr<PView> view = m_ScrolledView.Lock();
    if (view != nullptr)
    {
        const PPoint previousOffset = view->GetScrollOffset();
        const PRect scrollBounds = m_InertialScroller.GetScrollBounds();
        PPoint targetOffset = previousOffset + offset;

        targetOffset.x = std::clamp(targetOffset.x, scrollBounds.left, scrollBounds.right);
        targetOffset.y = std::clamp(targetOffset.y, scrollBounds.top, scrollBounds.bottom);
        targetOffset.Round();

        m_InertialScroller.ScrollTo(targetOffset, PPoint());
        view->ScrollTo(targetOffset);
        consumedOffset = view->GetScrollOffset() - previousOffset;
    }
    return consumedOffset;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::SlotInertialScrollUpdate(const PPoint& position)
{
    Ptr<PView> view = m_ScrolledView.Lock();
    if (view != nullptr) {
        view->ScrollTo(position);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::UpdateScroller()
{
    Ptr<PView> view = m_ScrolledView.Lock();
    if (view != nullptr)
    {
        const PRect  frame       = view->GetFrame();
        const PPoint contentSize = view->GetContentSize();

        m_InertialScroller.SetScrollHBounds(std::min(0.0f, frame.Width()  - contentSize.x), 0.0f);
        m_InertialScroller.SetScrollVBounds(std::min(0.0f, frame.Height() - contentSize.y), 0.0f);

        if (m_InertialScroller.GetState() == PInertialScroller::State::Idle)
        {
            PPoint maxScroll    = view->GetBounds().Size() - view->GetContentSize();
            PPoint scrollOffset = view->GetScrollOffset();

            maxScroll.x = std::min(0.0f, maxScroll.x);
            maxScroll.y = std::min(0.0f, maxScroll.y);

            scrollOffset.x = std::clamp(scrollOffset.x, maxScroll.x, 0.0f);
            scrollOffset.y = std::clamp(scrollOffset.y, maxScroll.y, 0.0f);

            view->ScrollTo(scrollOffset);
        }
    }
}

} // namespace osi

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewScroller::PViewScroller()
{
    m_Handler.SetViewScroller(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PViewScroller* PViewScroller::GetViewScroller(PView* view)
{
    if (view == nullptr) return nullptr;

    PViewScroller* viewScroller = dynamic_cast<PViewScroller*>(view);
    return (viewScroller != nullptr) ? viewScroller : GetViewScroller(ptr_raw_pointer_cast(view->GetParent()));
}
