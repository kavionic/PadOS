// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 01.05.2025 17:30

#pragma once

#include <GUI/Widgets/MVCBaseView.h>


struct PMVCListViewItemNode : PMVCBaseViewItemNode
{
    PMVCListViewItemNode() noexcept = default;
    PMVCListViewItemNode(const Ptr<PtrTarget>& itemData, const Ptr<PView>& itemWidget, uint32_t widgetClassID, bool isSelected, float height, float positionY) noexcept
        : PMVCBaseViewItemNode(itemData, itemWidget, widgetClassID, isSelected)
        , Height(height), PositionY(positionY) {}

    float           Height;
    float           PositionY;
};

///////////////////////////////////////////////////////////////////////////////
/// Flexible MVC list view
/// \ingroup gui
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PMVCListView : public PMVCBaseView
{
public:
    PMVCListView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PMVCListView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    virtual void OnFrameSized(const PPoint& delta) override;
    virtual void OnLayoutChanged() override;
    virtual PPoint CalculateContentSize() const override;

    void AddItem(Ptr<PtrTarget> item);

    virtual void Clear() override;
    virtual size_t GetItemIndexAtPosition(const PPoint& position) const override;

    virtual size_t GetItemCount() const override { return m_Items.size(); }

    template<typename CompareDelegate>
    void Sort(CompareDelegate&& compareDelegate) { SortList(m_Items, std::move(compareDelegate)); }

    float GetItemHeight(Ptr<const PtrTarget> item, uint32_t widgetClassID, float width) const;

    VFConnector<float (Ptr<const PtrTarget> itemData, float width)> VFGetItemHeight;

protected:
    virtual PMVCBaseViewItemNode& GetItemNode(size_t index) override { return m_Items[index]; }

    virtual void UpdateWidgets() override;
    virtual void OnItemsReordered() override;

private:
    void UpdateItemHeights();

    void SlotContentScrolled() { InvalidateLayout(); }

    std::vector<PMVCListViewItemNode>    m_Items;
    float                               m_ItemSpacing = 5.0f;
};
