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

const std::map<String, uint32_t> MVCListViewFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(MVCListViewFlags, MultiSelect)
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MVCListView::MVCListView(const String& name, Ptr<View> parent, uint32_t flags)
    : Control(name, parent, flags | ViewFlags::WillDraw)
{
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MVCListView::MVCListView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData)
    : Control(context, parent, xmlData)
{
    MergeFlags(ViewFlags::WillDraw);
    Construct();
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
    Control::OnLayoutChanged();

    UpdateWidgets();

    const Rect bounds = m_ContentView->GetBounds();

    if (m_FirstVisibleItem != INVALID_INDEX)
    {
        for (ssize_t i = m_FirstVisibleItem; i <= m_LastVisibleItem; ++i)
        {
            const ItemNode& itemNode = m_Items[i];
            if (itemNode.ItemWidget != nullptr)
            {

                itemNode.ItemWidget->SetFrame(Rect(bounds.left, itemNode.PositionY, bounds.right, itemNode.PositionY + itemNode.Height));
            }
        }
    }
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

void MVCListView::AddItem(Ptr<PtrTarget> item)
{
    const float prevItemBottom = m_Items.empty() ? 0.0f : (m_Items.back().PositionY + m_Items.back().Height + m_ItemSpacing);
    uint32_t classID = VFGetItemWidgetClassID.Empty() ? 0 : VFGetItemWidgetClassID(item);
    m_Items.push_back({ item, nullptr, classID, GetItemHeight(item, classID, Width()), prevItemBottom, false });
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> MVCListView::GetItemAt(size_t index)
{
    if (index < m_Items.size()) {
        return m_Items[index].ItemData;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::SetItemSelection(size_t index, bool isSelected)
{
    if (index < m_Items.size())
    {
        ItemNode& itemNode = m_Items[index];
        if (isSelected != itemNode.IsSelected)
        {
            if (isSelected && !HasFlags(MVCListViewFlags::MultiSelect)) {
                ClearSelection();
            }
            itemNode.IsSelected = isSelected;

            if (itemNode.ItemWidget != nullptr && !VFUpdateItemWidgetSelection.Empty()) {
                VFUpdateItemWidgetSelection(itemNode.ItemWidget, isSelected, itemNode.ItemData);
            }
            if (isSelected) {
                m_SelectedItems.insert(index);
            } else {
                m_SelectedItems.erase(index);
            }
            SignalSelectionChanged(index, isSelected, this);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MVCListView::GetItemSelection(size_t index) const
{
    if (index < m_Items.size())
    {
        return m_Items[index].IsSelected;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::ClearSelection()
{
    m_SelectedItems.clear();
    for (size_t i = 0; i < m_Items.size(); ++i)
    {
        ItemNode& itemNode = m_Items[i];
        if (itemNode.IsSelected)
        {
            SetItemSelection(i, false);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t MVCListView::GetItemIndex(float positionY) const
{
    const auto itemIter = std::upper_bound(m_Items.begin(), m_Items.end(), positionY, [](float y, const ItemNode& node) { return y < (node.PositionY + node.Height); });
    if (itemIter != m_Items.end() && (itemIter->PositionY + itemIter->Height) > positionY)
    {
        size_t index = itemIter - m_Items.begin();
        return index;
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

        if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemWidget, item, false);

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

void MVCListView::Construct()
{
    m_ScrollView = ptr_new<ScrollView>();
    m_ContentView = ptr_new<View>(String::zero, nullptr, ViewFlags::WillDraw);

    m_ContentView->VFCalculateContentSize.Connect(this, &MVCListView::SlotGetContentSize);
    m_ContentView->SignalViewScrolled.Connect(this, &MVCListView::SlotContentScrolled);
    
    m_ScrollView->SetScrolledView(m_ContentView);
    m_ScrollView->SetStartScrollThreshold(20.0f);
    m_ScrollView->VFTouchUp.Connect(this, &MVCListView::SlotScrollViewTouchUp);

    AddChild(m_ScrollView);
    SetLayoutNode(ptr_new<LayoutNode>());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<View> MVCListView::CreateItemWidget(uint32_t classID) const
{
    auto cachedWidgets = m_CachedItemWidgets.find(classID);
    
    if (cachedWidgets != m_CachedItemWidgets.end())
    {
        Ptr<View> itemWidget = cachedWidgets->second.back();
        cachedWidgets->second.pop_back();
        if (cachedWidgets->second.empty()) m_CachedItemWidgets.erase(cachedWidgets);

        return itemWidget;
    }
    return VFCreateItemWidget.Empty() ? nullptr : VFCreateItemWidget(classID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::AddItemWidget(size_t index)
{
    if (index < m_Items.size())
    {
        ItemNode& itemNode = m_Items[index];

        if (itemNode.ItemWidget == nullptr)
        {
            itemNode.ItemWidget = CreateItemWidget(itemNode.WidgetClassID);

            if (itemNode.ItemWidget != nullptr)
            {
                m_ContentView->AddChild(itemNode.ItemWidget);
                if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemNode.ItemWidget, itemNode.ItemData, itemNode.IsSelected);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::RemoveItemWidget(size_t index)
{
    if (index < m_Items.size())
    {
        ItemNode& itemNode = m_Items[index];
        if (itemNode.ItemWidget != nullptr)
        {
            CacheItemWidget(itemNode.ItemWidget, itemNode.WidgetClassID);
            itemNode.ItemWidget->RemoveThis();
            itemNode.ItemWidget = nullptr;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::CacheItemWidget(Ptr<View> itemWidget, int32_t widgetClassID) const
{
    if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemWidget, nullptr, false);
    m_CachedItemWidgets[widgetClassID].push_back(itemWidget);
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
        ItemNode& itemNode = m_Items[i];
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
        const auto firstVisible = std::lower_bound(m_Items.begin(), m_Items.end(), bounds.top, [](const ItemNode& node, float y) { return (node.PositionY + node.Height) < y; });

        if (firstVisible != m_Items.end())
        {
            const auto lastVisible = std::lower_bound(firstVisible, m_Items.end(), bounds.bottom, [](const ItemNode& node, float y) { return node.PositionY < y; });
            const ssize_t firstItemIndex = firstVisible - m_Items.begin();
            const ssize_t lastItemIndex = (lastVisible == m_Items.end()) ? (m_Items.size() - 1) : (lastVisible - m_Items.begin());


            if (m_FirstVisibleItem != INVALID_INDEX)
            {
                const ssize_t prevFirstItemIndex = (m_FirstVisibleItem != INVALID_INDEX) ? m_FirstVisibleItem : firstItemIndex;
                const ssize_t prevLastItemIndex = (m_LastVisibleItem != INVALID_INDEX) ? m_LastVisibleItem : lastItemIndex;

                if (prevLastItemIndex < firstItemIndex || prevFirstItemIndex > lastItemIndex) // No overlap.
                {
                    for (ssize_t i = prevLastItemIndex; i >= prevFirstItemIndex; --i) {
                        RemoveItemWidget(i);
                    }

                    assert(m_ContentView->GetChildList().empty());

                    for (ssize_t i = firstItemIndex; i <= lastItemIndex; ++i) {
                        AddItemWidget(i);
                    }
                }
                else if (prevFirstItemIndex >= firstItemIndex && prevLastItemIndex <= lastItemIndex) // Full overlap.
                {
                    for (ssize_t i = firstItemIndex; i < prevFirstItemIndex; ++i) {
                        AddItemWidget(i);
                    }
                    for (ssize_t i = prevLastItemIndex + 1; i <= lastItemIndex; ++i) {
                        AddItemWidget(i);
                    }
                }
                else if (prevFirstItemIndex < firstItemIndex) // Remove some before.
                {
                    for (ssize_t i = firstItemIndex - 1; i >= prevFirstItemIndex; --i) {
                        RemoveItemWidget(i);
                    }
                    for (ssize_t i = prevLastItemIndex + 1; i <= lastItemIndex; ++i) {
                        AddItemWidget(i);
                    }
                }
                else // Remove some after.
                {
                    for (ssize_t i = prevLastItemIndex; i > lastItemIndex; --i) {
                        RemoveItemWidget(i);
                    }
                    for (ssize_t i = firstItemIndex; i < prevFirstItemIndex; ++i) {
                        AddItemWidget(i);
                    }
                }
            }
            else
            {
                for (ssize_t i = firstItemIndex; i <= lastItemIndex; ++i) {
                    AddItemWidget(i);
                }
            }

            m_FirstVisibleItem = firstItemIndex;
            m_LastVisibleItem = lastItemIndex;

            return;
        }
    }
    if (m_FirstVisibleItem != INVALID_INDEX)
    {
        for (ssize_t i = m_LastVisibleItem; i >= m_FirstVisibleItem; --i) {
            RemoveItemWidget(i);
        }
    }
    m_FirstVisibleItem = INVALID_INDEX;
    m_LastVisibleItem = INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::ForceUpdateWidgets()
{
    m_SelectedItems.clear();

    float positionY = 0.0f;
    if (m_FirstVisibleItem != INVALID_INDEX)
    {
        for (size_t i = 0; i < m_FirstVisibleItem; ++i)
        {
            ItemNode& itemNode = m_Items[i];
            itemNode.PositionY = positionY;
            positionY += itemNode.Height + m_ItemSpacing;

            if (itemNode.ItemWidget != nullptr) {
                RemoveItemWidget(i);
            }
            if (itemNode.IsSelected) {
                m_SelectedItems.insert(i);
            }
        }
        for (size_t i = m_FirstVisibleItem; i <= m_LastVisibleItem; ++i)
        {
            ItemNode& itemNode = m_Items[i];
            itemNode.PositionY = positionY;
            positionY += itemNode.Height + m_ItemSpacing;

            if (itemNode.ItemWidget == nullptr) {
                AddItemWidget(i);
            }
            if (itemNode.IsSelected) {
                m_SelectedItems.insert(i);
            }
        }
        for (size_t i = m_LastVisibleItem + 1; i < m_Items.size(); ++i)
        {
            ItemNode& itemNode = m_Items[i];
            itemNode.PositionY = positionY;
            positionY += itemNode.Height + m_ItemSpacing;

            if (itemNode.ItemWidget != nullptr) {
                RemoveItemWidget(i);
            }
            if (itemNode.IsSelected) {
                m_SelectedItems.insert(i);
            }
        }
    }
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::ItemClicked(size_t index)
{
    SetItemSelection(index, !GetItemSelection(index));

    SignalItemClicked(index, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Point MVCListView::SlotGetContentSize()
{
    if (!m_Items.empty())
    {
        const ItemNode& lastItem = m_Items.back();
        return Point(0.0f, lastItem.PositionY + lastItem.Height);
    }
    return Point(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCListView::SlotScrollViewTouchUp(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    if (m_ScrollView->GetInertialScrollerState() == InertialScroller::State::WaitForThreshold)
    {
        float itemPosition = m_ContentView->ConvertFromScreen(view->ConvertToScreen(position)).y;
        ItemClicked(GetItemIndex(itemPosition));
    }
    view->OnTouchUp(pointID, position, motionEvent);
}

} // namespace os