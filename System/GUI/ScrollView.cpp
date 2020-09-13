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
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ScrollView::ScrollView(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData) : View(context, parent, xmlData)
{
    MergeFlags(ViewFlags::WillDraw);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ScrollView::FrameSized(const Point& delta)
{
    Ptr<View> clientView = GetScrolledView();
    if (clientView != nullptr)
    {
        Rect clientFrame = GetBounds();
        Rect clientBorders = clientView->GetBorders();
        clientFrame.Resize(clientBorders.left, clientBorders.top, clientBorders.right, clientBorders.bottom);
        clientView->SetFrame(clientFrame);
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
    Ptr<View> clientView = GetScrolledView();
    if (clientView != nullptr)
    {
        *minSize = clientView->GetPreferredSize(PrefSizeType::Smallest);
        *maxSize = clientView->GetPreferredSize(PrefSizeType::Greatest);
        Rect  clientBorders = clientView->GetBorders();
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

Ptr<View> ScrollView::SetScrolledView(Ptr<View> view)
{
    Ptr<View> prevClient = ViewScroller::SetScrolledView(view);
    if (prevClient == view) {
        return prevClient;
    }
    if (prevClient != nullptr)
    {
        RemoveChild(prevClient);
    }
    if (view != nullptr)
    {
        AddChild(view);
    }
    return prevClient;
}
