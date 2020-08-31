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

#include <GUI/ScrollView.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ScrollView::ScrollView(const String& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::WillDraw)
{
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ScrollView::ScrollView(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData) : View(context, parent, xmlData)
{
    MergeFlags(ViewFlags::WillDraw);
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::Initialize()
{
    m_InertialScroller.SetScrollHBounds(0.0f, 0.0f);
    UpdateScroller();
    m_InertialScroller.SignalUpdate.Connect(this, &ScrollView::SlotInertialScrollUpdate);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ScrollView> ScrollView::GetScrollView(View* view)
{
    if (view == nullptr) return nullptr;

    ScrollView* scrollView = dynamic_cast<ScrollView*>(view);
    return (scrollView != nullptr) ? ptr_tmp_cast(scrollView) : GetScrollView(ptr_raw_pointer_cast(view->GetParent()));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::FrameSized(const Point& delta)
{
    if (m_ClientView != nullptr)
    {
        Rect clientFrame = GetBounds();
        Rect clientBorders = m_ClientView->GetBorders();
        clientFrame.Resize(clientBorders.left, clientBorders.top, clientBorders.right, clientBorders.bottom);
        m_ClientView->SetFrame(clientFrame);
        UpdateScroller();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ScrollView::OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (m_HitButton != MouseButton_e::None) {
        return true;
    }
    m_HitButton = pointID;

    BeginSwipe(position);

    MakeFocus(pointID, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ScrollView::OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return true;
    }
    m_HitButton = MouseButton_e::None;
    MakeFocus(pointID, false);

    EndSwipe();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ScrollView::OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return true;
    }
    SwipeMove(position);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    if (m_ClientView != nullptr)
    {
        *minSize = m_ClientView->GetPreferredSize(PrefSizeType::Smallest);
        *maxSize = m_ClientView->GetPreferredSize(PrefSizeType::Greatest);
        Rect  clientBorders = m_ClientView->GetBorders();
        Point borderSize(clientBorders.left + clientBorders.right, clientBorders.top + clientBorders.bottom);

        *minSize += borderSize;
        *maxSize += borderSize;
    }
    else
    {
        View::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::BeginSwipe(const Point& position)
{
    if (m_ClientView != nullptr)
    {
        const Rect bounds = m_ClientView->GetBounds();
        if (m_ClientView->GetContentSize().y > bounds.Height())
        {
            m_InertialScroller.BeginDrag(m_ClientView->GetScrollOffset(), ConvertToRoot(position));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::SwipeMove(const Point& position)
{
    m_InertialScroller.AddUpdate(ConvertToRoot(position));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::EndSwipe()
{
    m_InertialScroller.EndDrag();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::SlotInertialScrollUpdate(const Point& position)
{
    if (m_ClientView != nullptr) {
        m_ClientView->ScrollTo(position);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> ScrollView::SetClientView(Ptr<View> view)
{
    if (m_ClientView != nullptr)
    {
        m_ClientView->SignalContentSizeChanged.Disconnect(this, &ScrollView::UpdateScroller);
        m_ClientView->RemoveThis();
        m_ClientView->ScrollTo(0.0f, 0.0f);
    }
    Ptr<View> prevClient = m_ClientView;
    m_ClientView = view;
    if (m_ClientView != nullptr)
    {
        AddChild(m_ClientView);
        m_ClientView->SignalContentSizeChanged.Connect(this, &ScrollView::UpdateScroller);
        UpdateScroller();
    }
    return prevClient;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::UpdateScroller()
{
    if (m_ClientView != nullptr)
    {
        Rect  frame = m_ClientView->GetFrame();
        Point contentSize = m_ClientView->GetContentSize();
        m_InertialScroller.SetScrollVBounds(frame.Height() - contentSize.y, 0.0f);
    }
}
