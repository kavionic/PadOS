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
// Created: 17.05.2025 22:00

#include <GUI/Widgets/ScrollView.h>
#include <GUI/Widgets/MVCBaseView.h>

namespace os
{

const std::map<String, uint32_t> MVCBaseViewFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(MVCBaseViewFlags, MultiSelect),
    DEFINE_FLAG_MAP_ENTRY(MVCBaseViewFlags, NoAutoSelect),
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MVCBaseView::MVCBaseView(const String& name, Ptr<View> parent, uint32_t flags)
    : Control(name, parent, flags | ViewFlags::WillDraw)
{
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

MVCBaseView::MVCBaseView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData)
    : Control(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, MVCBaseViewFlags::FlagMap, "flags", 0) | ViewFlags::WillDraw);
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> MVCBaseView::GetItemAtPosition(const Point& position) const
{
    const size_t index = GetItemIndexAtPosition(position);

    if (index != INVALID_INDEX) {
        return GetItemAt(index);
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> MVCBaseView::GetItemAt(size_t index) const
{
    if (index < GetItemCount()) {
        return GetItemNode(index).ItemData;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::SetHighlightedItem(size_t index)
{
    if (index != m_HighlightedItem)
    {
        const size_t prevHighlighted = m_HighlightedItem;
        m_HighlightedItem = index;

        if (prevHighlighted < GetItemCount() && !VFUpdateItemWidgetSelection.Empty())
        {
            const MVCBaseViewItemNode& itemNode = GetItemNode(prevHighlighted);
            if (itemNode.ItemWidget != nullptr) {
                VFUpdateItemWidgetSelection(itemNode.ItemWidget, itemNode.IsSelected, false, itemNode.ItemData);
            }
            SignalItemReleased(prevHighlighted, this);
        }

        if (m_HighlightedItem < GetItemCount() && !VFUpdateItemWidgetSelection.Empty())
        {
            const MVCBaseViewItemNode& itemNode = GetItemNode(m_HighlightedItem);
            if (itemNode.ItemWidget != nullptr) {
                VFUpdateItemWidgetSelection(itemNode.ItemWidget, itemNode.IsSelected, true, itemNode.ItemData);
            }
            SignalItemPressed(m_HighlightedItem, this);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::SetItemSelection(size_t index, bool isSelected)
{
    if (index < GetItemCount())
    {
        MVCBaseViewItemNode& itemNode = GetItemNode(index);
        if (isSelected != itemNode.IsSelected)
        {
            if (isSelected && !HasFlags(MVCBaseViewFlags::MultiSelect)) {
                ClearSelection();
            }
            itemNode.IsSelected = isSelected;

            if (itemNode.ItemWidget != nullptr && !VFUpdateItemWidgetSelection.Empty()) {
                VFUpdateItemWidgetSelection(itemNode.ItemWidget, isSelected, index == m_HighlightedItem, itemNode.ItemData);
            }
            if (isSelected) {
                m_SelectedItems.insert(index);
            }
            else {
                m_SelectedItems.erase(index);
            }
            SignalSelectionChanged(index, isSelected, this);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool MVCBaseView::GetItemSelection(size_t index) const
{
    if (index < GetItemCount())
    {
        return GetItemNode(index).IsSelected;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::ClearSelection()
{
    m_SelectedItems.clear();
    for (size_t i = 0; i < GetItemCount(); ++i)
    {
        MVCBaseViewItemNode& itemNode = GetItemNode(i);
        if (itemNode.IsSelected)
        {
            SetItemSelection(i, false);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::OnItemsReordered()
{
    m_SelectedItems.clear();

    if (m_FirstVisibleItem != INVALID_INDEX)
    {
        for (size_t i = 0; i < m_FirstVisibleItem; ++i)
        {
            MVCBaseViewItemNode& itemNode = GetItemNode(i);

            if (itemNode.ItemWidget != nullptr) {
                RemoveItemWidget(i);
            }
            if (itemNode.IsSelected) {
                m_SelectedItems.insert(i);
            }
        }
        for (size_t i = m_FirstVisibleItem; i <= m_LastVisibleItem; ++i)
        {
            MVCBaseViewItemNode& itemNode = GetItemNode(i);

            if (itemNode.ItemWidget == nullptr) {
                AddItemWidget(i);
            }
            if (itemNode.IsSelected) {
                m_SelectedItems.insert(i);
            }
        }
        for (size_t i = m_LastVisibleItem + 1; i < GetItemCount(); ++i)
        {
            MVCBaseViewItemNode& itemNode = GetItemNode(i);

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

void MVCBaseView::CreateWidgetsForRange(size_t firstItemIndex, size_t lastItemIndex)
{
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
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::RemoveAllWidgets()
{
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

Ptr<View> MVCBaseView::CreateItemWidget(uint32_t classID) const
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

void MVCBaseView::AddItemWidget(size_t index)
{
    if (index < GetItemCount())
    {
        MVCBaseViewItemNode& itemNode = GetItemNode(index);

        if (itemNode.ItemWidget == nullptr)
        {
            itemNode.ItemWidget = CreateItemWidget(itemNode.WidgetClassID);

            if (itemNode.ItemWidget != nullptr)
            {
                m_ContentView->AddChild(itemNode.ItemWidget);
                if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemNode.ItemWidget, itemNode.ItemData, itemNode.IsSelected, index == m_HighlightedItem);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::RemoveItemWidget(size_t index)
{
    if (index < GetItemCount())
    {
        MVCBaseViewItemNode& itemNode = GetItemNode(index);
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

void MVCBaseView::CacheItemWidget(Ptr<View> itemWidget, int32_t widgetClassID) const
{
    if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemWidget, nullptr, false, false);
    m_CachedItemWidgets[widgetClassID].push_back(itemWidget);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::Construct()
{
    m_ScrollView = ptr_new<ScrollView>();
    m_ContentView = ptr_new<View>(String::zero, nullptr, ViewFlags::WillDraw);

    m_ContentView->VFCalculateContentSize.Connect(this, &View::GetContentSize);
    m_ContentView->SignalFrameSized.Connect(this, &MVCBaseView::OnContentViewFrameSized);
    m_ContentView->SignalViewScrolled.Connect(this, &MVCBaseView::SlotContentScrolled);

    m_ScrollView->SetScrolledView(m_ContentView);
    m_ScrollView->SetStartScrollThreshold(20.0f);

    m_ScrollView->VFTouchDown.Connect(this, &MVCBaseView::SlotScrollViewTouchDown);
    m_ScrollView->VFTouchUp.Connect(this, &MVCBaseView::SlotScrollViewTouchUp);
    m_ScrollView->VFTouchMove.Connect(this, &MVCBaseView::SlotScrollViewTouchMove);

    AddChild(m_ScrollView);
    SetLayoutNode(ptr_new<LayoutNode>());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::ItemClicked(size_t index)
{
    if (!HasFlags(MVCBaseViewFlags::NoAutoSelect)) {
        SetItemSelection(index, !GetItemSelection(index));
    }
    SignalItemClicked(index, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::SlotScrollViewTouchDown(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    const Point& itemPosition = m_ContentView->ConvertFromScreen(view->ConvertToScreen(position));

    const size_t index = GetItemIndexAtPosition(itemPosition);
    SetHighlightedItem(index);

    view->OnTouchDown(pointID, position, motionEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::SlotScrollViewTouchUp(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    if (m_ScrollView->GetInertialScrollerState() == InertialScroller::State::WaitForThreshold)
    {
        const Point& itemPosition = m_ContentView->ConvertFromScreen(view->ConvertToScreen(position));

        const size_t index = GetItemIndexAtPosition(itemPosition);
        if (index != INVALID_INDEX && index == m_HighlightedItem)
        {
            SetHighlightedItem(INVALID_INDEX);
            ItemClicked(index);
        }
    }
    view->OnTouchUp(pointID, position, motionEvent);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void MVCBaseView::SlotScrollViewTouchMove(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent)
{
    if (m_HighlightedItem != INVALID_INDEX && m_ScrollView->GetInertialScrollerState() == InertialScroller::State::Dragging)
    {
        SetHighlightedItem(INVALID_INDEX);
    }
    view->OnTouchMove(pointID, position, motionEvent);
}

} // namespace os
