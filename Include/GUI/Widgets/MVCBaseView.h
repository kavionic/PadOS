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

#pragma once


#include <functional>
#include <tuple>

#include <Utils/TypeTraits.h>
#include <GUI/Widgets/Control.h>

namespace os
{

class ScrollView;

enum class EMVCHighlightState : uint8_t
{
    Normal,
    Highlighted,
    Selected
};

namespace MVCBaseViewFlags
{
static constexpr uint32_t MultiSelect   = 0x0001 << ViewFlags::FirstUserBit;
static constexpr uint32_t NoAutoSelect  = 0x0002 << ViewFlags::FirstUserBit;

extern const std::map<String, uint32_t> FlagMap;
}

struct MVCBaseViewItemNode
{
    MVCBaseViewItemNode(const Ptr<PtrTarget>& itemData, const Ptr<View>& itemWidget, uint32_t widgetClassID, bool isSelected)
        : ItemData(itemData), ItemWidget(itemWidget), WidgetClassID(widgetClassID), IsSelected(isSelected) {}

    Ptr<PtrTarget>  ItemData;
    Ptr<View>       ItemWidget;
    uint32_t        WidgetClassID;
    bool            IsSelected;
};

///////////////////////////////////////////////////////////////////////////////
/// Base class for MVC views
/// \ingroup gui
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class MVCBaseView : public Control
{
public:
    MVCBaseView(const String& name = String::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    MVCBaseView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    virtual void Clear() = 0;

    virtual size_t GetItemIndexAtPosition(const Point& position) const = 0;
    Ptr<PtrTarget> GetItemAtPosition(const Point& position) const;
    
    Ptr<PtrTarget> GetItemAt(size_t index) const;
    template<typename T>
    Ptr<T> GetItemAt(size_t index) const { return ptr_dynamic_cast<T>(GetItemAt(index)); }

    virtual size_t GetItemCount() const = 0;

    void    SetHighlightedItem(size_t index);
    size_t  GetHighlightedItem() const { return m_HighlightedItem; }

    void    SetItemSelection(size_t index, bool isSelected);
    bool    GetItemSelection(size_t index) const;

    void    ClearSelection();

    size_t  GetFirstSelected() const { return m_SelectedItems.empty() ? INVALID_INDEX : *m_SelectedItems.begin(); }
    const std::set<size_t>& GetSelectionSet() const { return m_SelectedItems; }

    Signal<void(size_t itemIndex, bool isSelected, MVCBaseView* sourceView)>   SignalSelectionChanged;
    Signal<void(size_t itemIndex, MVCBaseView* sourceView)>                    SignalItemPressed;
    Signal<void(size_t itemIndex, MVCBaseView* sourceView)>                    SignalItemReleased;
    Signal<void(size_t itemIndex, MVCBaseView* sourceView)>                    SignalItemClicked;

    VFConnector<uint32_t(Ptr<const PtrTarget> itemData)>                                   VFGetItemWidgetClassID;
    VFConnector<Ptr<View>(uint32_t widgetClassID)>                                         VFCreateItemWidget;

    VFConnector<void(Ptr<View> widget, Ptr<const PtrTarget> itemData, bool isSelected, bool isHighlighted)> VFUpdateItemWidget;
    VFConnector<void(Ptr<View> widget, bool isSelected, bool isHighlighted, Ptr<const PtrTarget> itemData)> VFUpdateItemWidgetSelection;

protected:
    virtual void                    OnContentViewFrameSized(const Point& delta) {}
    virtual MVCBaseViewItemNode&    GetItemNode(size_t index) = 0;
    const MVCBaseViewItemNode&      GetItemNode(size_t index) const { return const_cast<MVCBaseView*>(this)->GetItemNode(index); }

    virtual void UpdateWidgets() = 0;
    virtual void OnItemsReordered();

    template<typename TItemNodeType, typename TCompareDelegate>
    void SortList(std::vector<TItemNodeType>& itemList, TCompareDelegate&& compareDelegate)
    {
        using ItemDataType = std::remove_reference_t<callable_argument_type_t<TCompareDelegate, 0>>;

        std::stable_sort(itemList.begin(), itemList.end(), [compareDelegate](const MVCBaseViewItemNode& lhs, const MVCBaseViewItemNode& rhs)
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
        OnItemsReordered();
    }

    void        CreateWidgetsForRange(size_t firstItemIndex, size_t lastItemIndex);
    void        RemoveAllWidgets();

    Ptr<View>   CreateItemWidget(uint32_t classID) const;
    void        AddItemWidget(size_t index);
    void        RemoveItemWidget(size_t index);
    void        CacheItemWidget(Ptr<View> itemWidget, int32_t widgetClassID) const;


    ssize_t     m_FirstVisibleItem = INVALID_INDEX;
    ssize_t     m_LastVisibleItem = INVALID_INDEX;

    Ptr<View>   m_ContentView;

private:
    void Construct();

    void ItemClicked(size_t index);

    void SlotContentScrolled() { InvalidateLayout(); }

    void SlotScrollViewTouchDown(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);
    void SlotScrollViewTouchUp(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);
    void SlotScrollViewTouchMove(View* view, MouseButton_e pointID, const Point& position, const MotionEvent& motionEvent);

    Ptr<ScrollView>         m_ScrollView;

    std::set<size_t>        m_SelectedItems;
    size_t                  m_HighlightedItem = INVALID_INDEX;

    mutable std::map<uint32_t, std::vector<Ptr<View>>> m_CachedItemWidgets;
};


} // namespace os
