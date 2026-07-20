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


const std::map<PString, uint32_t> PMVCBaseViewFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(PMVCBaseViewFlags, MultiSelect),
    DEFINE_FLAG_MAP_ENTRY(PMVCBaseViewFlags, NoAutoSelect),
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMVCBaseView::PMVCBaseView(const PString& name, Ptr<PView> parent, uint32_t flags)
    : PControl(name, parent, flags | PViewFlags::WillDraw)
{
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMVCBaseView::PMVCBaseView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData)
    : PControl(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, PMVCBaseViewFlags::FlagMap, "flags", 0) | PViewFlags::WillDraw);
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCBaseView::OnLayoutChanged()
{
    PControl::OnLayoutChanged();

    m_ScrollView->RefreshLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PtrTarget> PMVCBaseView::GetItemAtPosition(const PPoint& position) const
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

Ptr<PtrTarget> PMVCBaseView::GetItemAt(size_t index) const
{
    if (index < GetItemCount()) {
        return GetItemNode(index).ItemData;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCBaseView::SetHighlightedItem(size_t index)
{
    if (index != m_HighlightedItem)
    {
        const size_t prevHighlighted = m_HighlightedItem;
        m_HighlightedItem = index;

        if (prevHighlighted < GetItemCount() && !VFUpdateItemWidgetSelection.Empty())
        {
            const PMVCBaseViewItemNode& itemNode = GetItemNode(prevHighlighted);
            if (itemNode.ItemWidget != nullptr) {
                VFUpdateItemWidgetSelection(itemNode.ItemWidget, itemNode.IsSelected, false, itemNode.ItemData);
            }
            SignalItemReleased(prevHighlighted, this);
        }

        if (m_HighlightedItem < GetItemCount() && !VFUpdateItemWidgetSelection.Empty())
        {
            const PMVCBaseViewItemNode& itemNode = GetItemNode(m_HighlightedItem);
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

void PMVCBaseView::SetItemSelection(size_t index, bool isSelected)
{
    if (index < GetItemCount())
    {
        PMVCBaseViewItemNode& itemNode = GetItemNode(index);
        if (isSelected != itemNode.IsSelected)
        {
            if (isSelected && !HasFlags(PMVCBaseViewFlags::MultiSelect)) {
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

bool PMVCBaseView::GetItemSelection(size_t index) const
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

void PMVCBaseView::ClearSelection()
{
    m_SelectedItems.clear();
    for (size_t i = 0; i < GetItemCount(); ++i)
    {
        PMVCBaseViewItemNode& itemNode = GetItemNode(i);
        if (itemNode.IsSelected)
        {
            SetItemSelection(i, false);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCBaseView::OnItemsReordered()
{
    m_SelectedItems.clear();

    if (m_FirstVisibleItem != INVALID_INDEX)
    {
        for (size_t i = 0; i < m_FirstVisibleItem; ++i)
        {
            PMVCBaseViewItemNode& itemNode = GetItemNode(i);

            if (itemNode.ItemWidget != nullptr) {
                RemoveItemWidget(i);
            }
            if (itemNode.IsSelected) {
                m_SelectedItems.insert(i);
            }
        }
        for (size_t i = m_FirstVisibleItem; i <= m_LastVisibleItem; ++i)
        {
            PMVCBaseViewItemNode& itemNode = GetItemNode(i);

            if (itemNode.ItemWidget == nullptr) {
                AddItemWidget(i);
            }
            if (itemNode.IsSelected) {
                m_SelectedItems.insert(i);
            }
        }
        for (size_t i = m_LastVisibleItem + 1; i < GetItemCount(); ++i)
        {
            PMVCBaseViewItemNode& itemNode = GetItemNode(i);

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

void PMVCBaseView::CreateWidgetsForRange(size_t firstItemIndex, size_t lastItemIndex)
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

            if (!m_ContentView->GetChildList().empty())
            {
                p_system_log<PLogSeverity::ERROR>(LogCategoryGUITK, "{}: no overlap, but {} children remain.", __PRETTY_FUNCTION__, m_ContentView->GetChildList().size());
                while (!m_ContentView->GetChildList().empty()) {
                    m_ContentView->RemoveChild(m_ContentView->GetChildAt(0));
                }
            }

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

void PMVCBaseView::RemoveAllWidgets()
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

Ptr<PView> PMVCBaseView::CreateItemWidget(uint32_t classID) const
{
    auto cachedWidgets = m_CachedItemWidgets.find(classID);

    if (cachedWidgets != m_CachedItemWidgets.end())
    {
        Ptr<PView> itemWidget = cachedWidgets->second.back();
        cachedWidgets->second.pop_back();
        if (cachedWidgets->second.empty()) m_CachedItemWidgets.erase(cachedWidgets);

        return itemWidget;
    }
    return VFCreateItemWidget.Empty() ? nullptr : VFCreateItemWidget(classID);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCBaseView::AddItemWidget(size_t index)
{
    if (index < GetItemCount())
    {
        PMVCBaseViewItemNode& itemNode = GetItemNode(index);

        if (itemNode.ItemWidget == nullptr)
        {
            itemNode.ItemWidget = CreateItemWidget(itemNode.WidgetClassID);

            if (itemNode.ItemWidget != nullptr)
            {
                itemNode.ItemWidget->SetHitMode(PViewHitMode::IgnoreRecursive);
                m_ContentView->AddChild(itemNode.ItemWidget);
                if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemNode.ItemWidget, itemNode.ItemData, itemNode.IsSelected, index == m_HighlightedItem);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCBaseView::RemoveItemWidget(size_t index)
{
    if (index < GetItemCount())
    {
        PMVCBaseViewItemNode& itemNode = GetItemNode(index);
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

void PMVCBaseView::CacheItemWidget(Ptr<PView> itemWidget, int32_t widgetClassID) const
{
    if (!VFUpdateItemWidget.Empty()) VFUpdateItemWidget(itemWidget, nullptr, false, false);
    m_CachedItemWidgets[widgetClassID].push_back(itemWidget);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCBaseView::Construct()
{
    m_ScrollView = ptr_new<PScrollView>();
    m_ContentView = ptr_new<PView>(PString::zero, nullptr, PViewFlags::WillDraw);

    m_ContentView->VFCalculateContentSize.Connect(this, &PView::GetContentSize);
    m_ContentView->SignalFrameSized.Connect(this, &PMVCBaseView::OnContentViewFrameSized);
    m_ContentView->SignalViewScrolled.Connect(this, &PMVCBaseView::SlotContentScrolled);
    m_ContentView->VFPointerDown.Connect(this, &PMVCBaseView::SlotContentViewPointerDown);
    m_ContentView->VFPointerUp.Connect(this, &PMVCBaseView::SlotContentViewPointerUp);
    m_ContentView->VFPointerMove.Connect(this, &PMVCBaseView::SlotContentViewPointerMove);
    m_ContentView->VFPointerCancel.Connect(this, &PMVCBaseView::SlotContentViewPointerCancel);

    m_ScrollView->SetScrolledView(m_ContentView);
    m_ScrollView->SetStartScrollThreshold(BEGIN_DRAG_OFFSET);

    AddChild(m_ScrollView);
    SetLayoutNode(ptr_new<PLayoutNode>());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCBaseView::ItemClicked(size_t index)
{
    if (!HasFlags(PMVCBaseViewFlags::NoAutoSelect)) {
        SetItemSelection(index, !GetItemSelection(index));
    }
    SignalItemClicked(index, this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PMVCBaseView::SlotContentViewPointerDown(PView* view, PPointerID pointerID, const PPoint& position, const PPointerEvent& motionEvent, PEventPhase phase)
{
    if (phase != PEventPhase::Target) {
        return false;
    }

    const size_t index = GetItemIndexAtPosition(position);
    m_PressedItem = index;
    SetHighlightedItem(index);
    return index != INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PMVCBaseView::SlotContentViewPointerUp(PView* view, PPointerID pointerID, const PPoint& position, const PPointerEvent& motionEvent, PEventPhase phase)
{
    if (phase != PEventPhase::Target) {
        return false;
    }

    const bool handled = m_PressedItem != INVALID_INDEX;

    if (m_HighlightedItem != INVALID_INDEX)
    {
        const size_t clickedItem = m_HighlightedItem;
        SetHighlightedItem(INVALID_INDEX);

        if (clickedItem < GetItemCount()) {
            ItemClicked(clickedItem);
        }
    }
    m_PressedItem = INVALID_INDEX;
    return handled;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PMVCBaseView::SlotContentViewPointerMove(PView* view, PPointerID pointerID, const PPoint& position, const PPointerEvent& motionEvent, PEventPhase phase)
{
    if (phase != PEventPhase::Target) {
        return false;
    }
    if (m_PressedItem == INVALID_INDEX) {
        return false;
    }

    const size_t itemIndex = GetItemIndexAtPosition(position);
    const size_t highlightedItem = (itemIndex == m_PressedItem) ? m_PressedItem : INVALID_INDEX;
    SetHighlightedItem(highlightedItem);

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PMVCBaseView::SlotContentViewPointerCancel(PView* view, PPointerID pointerID, const PPoint& position, const PPointerEvent& motionEvent, PEventPhase phase)
{
    if (phase != PEventPhase::Target) {
        return false;
    }

    const bool handled = m_PressedItem != INVALID_INDEX || m_HighlightedItem != INVALID_INDEX;
    m_PressedItem = INVALID_INDEX;

    if (m_HighlightedItem != INVALID_INDEX)
    {
        SetHighlightedItem(INVALID_INDEX);
    }
    return handled;
}
