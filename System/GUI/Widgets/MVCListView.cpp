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


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMVCListView::PMVCListView(const PString& name, Ptr<PView> parent, uint32_t flags)
    : PMVCBaseView(name, parent, flags)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMVCListView::PMVCListView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData)
    : PMVCBaseView(context, parent, xmlData)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCListView::OnFrameSized(const PPoint& delta)
{
    if (delta.x != 0.0f) {
        UpdateItemHeights();
    }
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCListView::OnLayoutChanged()
{
    PMVCBaseView::OnLayoutChanged();

    UpdateWidgets();

    const PRect bounds = m_ContentView->GetBounds();

    if (m_FirstVisibleItem != INVALID_INDEX)
    {
        for (ssize_t i = m_FirstVisibleItem; i <= m_LastVisibleItem; ++i)
        {
            const PMVCListViewItemNode& itemNode = m_Items[i];
            if (itemNode.ItemWidget != nullptr) {
                itemNode.ItemWidget->SetFrame(PRect(bounds.left, itemNode.PositionY, bounds.right, itemNode.PositionY + itemNode.Height));
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PMVCListView::CalculateContentSize() const
{
    if (!m_Items.empty())
    {
        const PMVCListViewItemNode& lastItem = m_Items.back();
        return PPoint(0.0f, lastItem.PositionY + lastItem.Height);
    }
    return PPoint(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCListView::AddItem(Ptr<PtrTarget> item)
{
    const float prevItemBottom = m_Items.empty() ? 0.0f : (m_Items.back().PositionY + m_Items.back().Height + m_ItemSpacing);
    uint32_t classID = VFGetItemWidgetClassID.Empty() ? 0 : VFGetItemWidgetClassID(item);
    m_Items.emplace_back(item, nullptr, classID, false, GetItemHeight(item, classID, Width()), prevItemBottom);
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCListView::Clear()
{
    m_Items.clear();
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PMVCListView::GetItemIndexAtPosition(const PPoint& position) const
{
    const PRect& bounds = GetBounds();
    if (position.x >= bounds.left && position.x <= bounds.right - 1.0f)
    {
        const auto itemIter = std::upper_bound(m_Items.begin(), m_Items.end(), position.y, [](float y, const PMVCListViewItemNode& node) { return y < (node.PositionY + node.Height); });
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

float PMVCListView::GetItemHeight(Ptr<const PtrTarget> item, uint32_t widgetClassID, float width) const
{
    if (!VFGetItemHeight.Empty()) {
        return VFGetItemHeight(item, width);
    }
    Ptr<PView> itemWidget = CreateItemWidget(widgetClassID);
    if (itemWidget != nullptr)
    {
        itemWidget->SetFrame(PRect(0.0f, 0.0f, width, COORD_MAX));

        if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemWidget, item, false, false);

        itemWidget->RefreshLayout(3, true);

        const float height = itemWidget->GetPreferredSize(PPrefSizeType::Smallest).y;
        CacheItemWidget(itemWidget, widgetClassID);
        return height;
    }
    return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCListView::UpdateItemHeights()
{
    const float width = Width();
    float positionY = 0.0f;
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
        PMVCListViewItemNode& itemNode = m_Items[i];
        itemNode.Height = GetItemHeight(itemNode.ItemData, itemNode.WidgetClassID, width);
        itemNode.PositionY = positionY;
        positionY += itemNode.Height + m_ItemSpacing;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCListView::UpdateWidgets()
{
    const PRect& bounds = m_ContentView->GetBounds();

    if (bounds.IsValid())
    {
        const auto firstVisible = std::lower_bound(m_Items.begin(), m_Items.end(), bounds.top, [](const PMVCListViewItemNode& node, float y) { return (node.PositionY + node.Height) < y; });

        if (firstVisible != m_Items.end())
        {
            const auto lastVisible = std::lower_bound(firstVisible, m_Items.end(), bounds.bottom, [](const PMVCListViewItemNode& node, float y) { return node.PositionY < y; });
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

void PMVCListView::OnItemsReordered()
{
    float positionY = 0.0f;

    for (PMVCListViewItemNode& itemNode : m_Items)
    {
        itemNode.PositionY = positionY;
        positionY += itemNode.Height + m_ItemSpacing;
    }
    PMVCBaseView::OnItemsReordered();
}
