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

#pragma once

#include <functional>
#include <tuple>

#include <Utils/TypeTraits.h>
#include <GUI/Widgets/Control.h>

namespace os
{

class ScrollView;

namespace MVCListViewFlags
{
static constexpr uint32_t MultiSelect = 0x0001 << ViewFlags::FirstUserBit;

extern const std::map<String, uint32_t> FlagMap;
}

///////////////////////////////////////////////////////////////////////////////
/// Flexible MVC list view
/// \ingroup gui
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class MVCListView : public Control
{
public:
    MVCListView(const String& name = String::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    MVCListView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    virtual void OnFrameSized(const Point& delta) override;
    virtual void OnLayoutChanged() override;
//    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;

    void Clear();
    void AddItem(Ptr<PtrTarget> item);

    Ptr<PtrTarget> GetItemAt(size_t index);
    template<typename T>
    Ptr<T> GetItemAt(size_t index) { return ptr_dynamic_cast<T>(GetItemAt(index)); }

    void SetItemSelection(size_t index, bool isSelected);
    bool GetItemSelection(size_t index) const;

    void ClearSelection();

    size_t GetFirstSelected() const { return m_SelectedItems.empty() ? INVALID_INDEX : *m_SelectedItems.begin(); }
    const std::set<size_t>& GetSelectionSet() const { return m_SelectedItems; }

    size_t GetItemIndex(float positionY) const;

    float GetItemHeight(Ptr<const PtrTarget> item, uint32_t widgetClassID, float width) const;


    template<typename CompareDelegate>
    void Sort(CompareDelegate&& compareDelegate)
    {
        using ItemDataType = std::remove_reference_t<callable_argument_type_t<CompareDelegate, 0>>;

        std::stable_sort(m_Items.begin(), m_Items.end(), [compareDelegate](const ItemNode& lhs, const ItemNode& rhs)
            {
                const ItemDataType* const lhsData = ptr_raw_pointer_dynamic_cast<ItemDataType>(lhs.ItemData);
                const ItemDataType* const rhsData = ptr_raw_pointer_dynamic_cast<ItemDataType>(rhs.ItemData);

                if (lhsData != nullptr && rhsData != nullptr) {
                    return compareDelegate(*lhsData, *rhsData);
                } else {
                    return lhsData < rhsData;
                }
            }
        );
        ForceUpdateWidgets();
    }

    Signal<void (size_t itemIndex, bool isSelected, MVCListView* sourceView)>   SignalSelectionChanged;
    Signal<void (size_t itemIndex, MVCListView* sourceView)>                    SignalItemClicked;

    VFConnector<uint32_t (Ptr<const PtrTarget> itemData)>                                   VFGetItemWidgetClassID;
    VFConnector<Ptr<View> (uint32_t widgetClassID)>                                         VFCreateItemWidget;
    VFConnector<float (Ptr<const PtrTarget> itemData, float width)>                         VFGetItemHeight;

    VFConnector<void (Ptr<View> widget, Ptr<const PtrTarget> itemData, bool isSelected)>    VFUpdateItemWidget;
    VFConnector<void (Ptr<View> widget, bool isSelected, Ptr<const PtrTarget> itemData)>    VFUpdateItemWidgetSelection;

private:
    void Construct();

    Ptr<View> CreateItemWidget(uint32_t classID) const;
    void AddItemWidget(size_t index);
    void RemoveItemWidget(size_t index);
    void CacheItemWidget(Ptr<View> itemWidget, int32_t widgetClassID) const;

    void UpdateItemHeights();
    void UpdateWidgets();
    void ForceUpdateWidgets();

    void ItemClicked(size_t index);

    Point SlotGetContentSize();
    void SlotContentScrolled() { InvalidateLayout(); }

    void SlotScrollViewTouchUp(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);

    struct ItemNode
    {
        Ptr<PtrTarget>  ItemData;
        Ptr<View>       ItemWidget;
        uint32_t        WidgetClassID;
        float           Height;
        float           PositionY;
        bool            IsSelected;
    };

    Ptr<ScrollView>         m_ScrollView;
    Ptr<View>               m_ContentView;

    std::vector<ItemNode>   m_Items;
    std::set<size_t>        m_SelectedItems;
    float                   m_ItemSpacing = 5.0f;
    ssize_t                 m_FirstVisibleItem = INVALID_INDEX;
    ssize_t                 m_LastVisibleItem = INVALID_INDEX;

    mutable std::map<uint32_t, std::vector<Ptr<View>>> m_CachedItemWidgets;
};


} // namespace os


