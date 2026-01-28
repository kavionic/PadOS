// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 01.05.2025 17:30

#include <algorithm>
#include <GUI/Widgets/MVCListView.h>
#include <GUI/Widgets/ScrollView.h>

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MVCListView::MVCListView(const PString& name, Ptr<View> parent, uint32_t flags)
    : MVCBaseView(name, parent, flags)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MVCListView::MVCListView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData)
    : MVCBaseView(context, parent, xmlData)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::OnFrameSized(const Point& delta)
{
    if (delta.x != 0.0f) {
        UpdateItemHeights();
    }
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::OnLayoutChanged()
{
    MVCBaseView::OnLayoutChanged();

    UpdateWidgets();

    const Rect bounds = m_ContentView->GetBounds();

    if (m_FirstVisibleItem != INVALID_INDEX)
    {
        for (ssize_t i = m_FirstVisibleItem; i <= m_LastVisibleItem; ++i)
        {
            const MVCListViewItemNode& itemNode = m_Items[i];
            if (itemNode.ItemWidget != nullptr) {
                itemNode.ItemWidget->SetFrame(Rect(bounds.left, itemNode.PositionY, bounds.right, itemNode.PositionY + itemNode.Height));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point MVCListView::CalculateContentSize() const
{
    if (!m_Items.empty())
    {
        const MVCListViewItemNode& lastItem = m_Items.back();
        return Point(0.0f, lastItem.PositionY + lastItem.Height);
    }
    return Point(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::AddItem(Ptr<PtrTarget> item)
{
    const float prevItemBottom = m_Items.empty() ? 0.0f : (m_Items.back().PositionY + m_Items.back().Height + m_ItemSpacing);
    uint32_t classID = VFGetItemWidgetClassID.Empty() ? 0 : VFGetItemWidgetClassID(item);
    m_Items.emplace_back(item, nullptr, classID, false, GetItemHeight(item, classID, Width()), prevItemBottom);
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::Clear()
{
    m_Items.clear();
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t MVCListView::GetItemIndexAtPosition(const Point& position) const
{
    const Rect& bounds = GetBounds();
    if (position.x >= bounds.left && position.x <= bounds.right - 1.0f)
    {
        const auto itemIter = std::upper_bound(m_Items.begin(), m_Items.end(), position.y, [](float y, const MVCListViewItemNode& node) { return y < (node.PositionY + node.Height); });
        if (itemIter != m_Items.end() && (itemIter->PositionY + itemIter->Height) > position.y)
        {
            size_t index = itemIter - m_Items.begin();
            return index;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float MVCListView::GetItemHeight(Ptr<const PtrTarget> item, uint32_t widgetClassID, float width) const
{
    if (!VFGetItemHeight.Empty()) {
        return VFGetItemHeight(item, width);
    }
    Ptr<View> itemWidget = CreateItemWidget(widgetClassID);
    if (itemWidget != nullptr)
    {
        itemWidget->SetFrame(Rect(0.0f, 0.0f, width, COORD_MAX));

        if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemWidget, item, false, false);

        itemWidget->RefreshLayout(3, true);

        const float height = itemWidget->GetPreferredSize(PrefSizeType::Smallest).y;
        CacheItemWidget(itemWidget, widgetClassID);
        return height;
    }
    return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::UpdateItemHeights()
{
    const float width = Width();
    float positionY = 0.0f;
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
        MVCListViewItemNode& itemNode = m_Items[i];
        itemNode.Height = GetItemHeight(itemNode.ItemData, itemNode.WidgetClassID, width);
        itemNode.PositionY = positionY;
        positionY += itemNode.Height + m_ItemSpacing;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::UpdateWidgets()
{
    const Rect& bounds = m_ContentView->GetBounds();

    if (bounds.IsValid())
    {
        const auto firstVisible = std::lower_bound(m_Items.begin(), m_Items.end(), bounds.top, [](const MVCListViewItemNode& node, float y) { return (node.PositionY + node.Height) < y; });

        if (firstVisible != m_Items.end())
        {
            const auto lastVisible = std::lower_bound(firstVisible, m_Items.end(), bounds.bottom, [](const MVCListViewItemNode& node, float y) { return node.PositionY < y; });
            const ssize_t firstItemIndex = firstVisible - m_Items.begin();
            const ssize_t lastItemIndex = (lastVisible == m_Items.end()) ? (m_Items.size() - 1) : (lastVisible - m_Items.begin());

            CreateWidgetsForRange(firstItemIndex, lastItemIndex);
            return;
        }
    }
    RemoveAllWidgets();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::OnItemsReordered()
{
    float positionY = 0.0f;

    for (MVCListViewItemNode& itemNode : m_Items)
    {
        itemNode.PositionY = positionY;
        positionY += itemNode.Height + m_ItemSpacing;
    }
    MVCBaseView::OnItemsReordered();
}

} // namespace os
