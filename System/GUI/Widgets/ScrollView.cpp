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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollView::PScrollView(const PString& name, Ptr<PView> parent, uint32_t flags) : PView(name, parent, flags | PViewFlags::WillDraw)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PScrollView::PScrollView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData) : PView(context, parent, xmlData)
{
    MergeFlags(PViewFlags::WillDraw);

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

bool PScrollView::OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (m_HitButton != PMouseButton::None) {
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

bool PScrollView::OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
{
    if (pointID != m_HitButton) {
        return true;
    }
    m_HitButton = PMouseButton::None;
    MakeFocus(pointID, false);

    EndSwipe();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PScrollView::OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event)
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
