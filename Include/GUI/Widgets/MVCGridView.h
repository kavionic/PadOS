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

#include <GUI/Widgets/MVCBaseView.h>


struct PMVCGridViewItemNode : PMVCBaseViewItemNode
{
    PMVCGridViewItemNode() noexcept = default;
    PMVCGridViewItemNode(const Ptr<PtrTarget>& itemData, const Ptr<PView>& itemWidget, uint32_t widgetClassID, bool isSelected) noexcept
        : PMVCBaseViewItemNode(itemData, itemWidget, widgetClassID, isSelected) {}
};

///////////////////////////////////////////////////////////////////////////////
/// Flexible MVC list view
/// \ingroup gui
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PMVCGridView : public PMVCBaseView
{
public:
    PMVCGridView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PMVCGridView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    virtual void OnLayoutChanged() override;
    virtual PPoint CalculateContentSize() const override;

    void SetGridSize(const PPoint& gridSize);
    PPoint GetGridSize() const { return m_GridSize; }

    void AddItem(Ptr<PtrTarget> item);

    virtual void Clear() override;
    virtual size_t GetItemIndexAtPosition(const PPoint& position) const override;

    virtual size_t GetItemCount() const override { return m_Items.size(); }

    template<typename CompareDelegate>
    void Sort(CompareDelegate&& compareDelegate) { SortList(m_Items, std::move(compareDelegate)); }

protected:
    virtual void OnContentViewFrameSized(const PPoint& delta) override;

    virtual PMVCBaseViewItemNode& GetItemNode(size_t index) override { return m_Items[index]; }

    virtual void UpdateWidgets() override;

private:
    void UpdateItemHeights();

    void SlotContentScrolled() { InvalidateLayout(); }

    std::vector<PMVCGridViewItemNode>    m_Items;

    PPoint   m_GridSize = PPoint(100.0f, 100.0f);
    int32_t m_RowCount = 0;
    int32_t m_ColumnCount = 0;
    float   m_LeftMargin = 0.0f;
};
