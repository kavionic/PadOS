// This file is part of PadOS.
//
// Copyright (C) 2020 Kurt Skauen <http://kavionic.com/>
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

#include <GUI/ViewScroller.h>

using namespace os;

namespace os
{

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

Ptr<View> ViewScrollerSignalTarget::SetScrolledView(Ptr<View> view)
{
    if (m_ScrolledView != nullptr)
    {
        m_ScrolledView->SignalContentSizeChanged.Disconnect(this, &ViewScrollerSignalTarget::UpdateScroller);
        m_ScrolledView->SignalFrameSized.Disconnect(this, &ViewScrollerSignalTarget::UpdateScroller);
        m_ScrolledView->RemoveThis();
        m_ScrolledView->ScrollTo(0.0f, 0.0f);
    }
    Ptr<View> prevClient = m_ScrolledView;
    m_ScrolledView = view;
    if (m_ScrolledView != nullptr)
    {
        m_ScrolledView->SignalContentSizeChanged.Connect(this, &ViewScrollerSignalTarget::UpdateScroller);
        m_ScrolledView->SignalFrameSized.Connect(this, &ViewScrollerSignalTarget::UpdateScroller);
        UpdateScroller();
    }
    return prevClient;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::BeginSwipe(const Point& position)
{
    if (m_ScrolledView != nullptr)
    {
        const Rect bounds = m_ScrolledView->GetBounds();
        const Point contentSize = m_ScrolledView->GetContentSize();
        if (contentSize.x > bounds.Width() || contentSize.y > bounds.Height())
        {
            m_InertialScroller.BeginDrag(m_ScrolledView->GetScrollOffset(), position);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::SwipeMove(const Point& position)
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

void ViewScrollerSignalTarget::SlotInertialScrollUpdate(const Point& position)
{
    if (m_ScrolledView != nullptr) {
        m_ScrolledView->ScrollTo(position);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ViewScrollerSignalTarget::UpdateScroller()
{
    if (m_ScrolledView != nullptr)
    {
        const Rect  frame       = m_ScrolledView->GetFrame();
        const Point contentSize = m_ScrolledView->GetContentSize();

        m_InertialScroller.SetScrollHBounds(std::min(0.0f, frame.Width()  - contentSize.x), 0.0f);
        m_InertialScroller.SetScrollVBounds(std::min(0.0f, frame.Height() - contentSize.y), 0.0f);

        if (m_InertialScroller.GetState() == InertialScroller::State::Idle)
        {
            Point maxScroll    = m_ScrolledView->GetBounds().Size() - m_ScrolledView->GetContentSize();
            Point scrollOffset = m_ScrolledView->GetScrollOffset();

            maxScroll.x = std::min(0.0f, maxScroll.x);
            maxScroll.y = std::min(0.0f, maxScroll.y);

            scrollOffset.x = std::clamp(scrollOffset.x, maxScroll.x, 0.0f);
            scrollOffset.y = std::clamp(scrollOffset.y, maxScroll.y, 0.0f);

            m_ScrolledView->ScrollTo(scrollOffset);
        }
    }
}

} // namespace osi

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ViewScroller::ViewScroller()
{
    m_Handler.SetViewScroller(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ViewScroller* ViewScroller::GetViewScroller(View* view)
{
    if (view == nullptr) return nullptr;

    ViewScroller* viewScroller = dynamic_cast<ViewScroller*>(view);
    return (viewScroller != nullptr) ? viewScroller : GetViewScroller(ptr_raw_pointer_cast(view->GetParent()));
}

} // namespace os

