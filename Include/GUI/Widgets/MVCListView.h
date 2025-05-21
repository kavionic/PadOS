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

#include <GUI/Widgets/MVCBaseView.h>

namespace os
{

struct MVCListViewItemNode : MVCBaseViewItemNode
{
    MVCListViewItemNode() noexcept = default;
    MVCListViewItemNode(const Ptr<PtrTarget>& itemData, const Ptr<View>& itemWidget, uint32_t widgetClassID, bool isSelected, float height, float positionY) noexcept
        : MVCBaseViewItemNode(itemData, itemWidget, widgetClassID, isSelected)
        , Height(height), PositionY(positionY) {}

    float           Height;
    float           PositionY;
};

///////////////////////////////////////////////////////////////////////////////
/// Flexible MVC list view
/// \ingroup gui
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class MVCListView : public MVCBaseView
{
public:
    MVCListView(const String& name = String::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    MVCListView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    virtual void OnFrameSized(const Point& delta) override;
    virtual void OnLayoutChanged() override;
    virtual Point CalculateContentSize() const override;

    void AddItem(Ptr<PtrTarget> item);

    virtual void Clear() override;
    virtual size_t GetItemIndexAtPosition(const Point& position) const override;

    virtual size_t GetItemCount() const override { return m_Items.size(); }

    template<typename CompareDelegate>
    void Sort(CompareDelegate&& compareDelegate) { SortList(m_Items, std::move(compareDelegate)); }

    float GetItemHeight(Ptr<const PtrTarget> item, uint32_t widgetClassID, float width) const;

    VFConnector<float (Ptr<const PtrTarget> itemData, float width)> VFGetItemHeight;

protected:
    virtual MVCBaseViewItemNode& GetItemNode(size_t index) override { return m_Items[index]; }

    virtual void UpdateWidgets() override;
    virtual void OnItemsReordered() override;

private:
    void UpdateItemHeights();

    void SlotContentScrolled() { InvalidateLayout(); }

    std::vector<MVCListViewItemNode>    m_Items;
    float                               m_ItemSpacing = 5.0f;
};


} // namespace os
