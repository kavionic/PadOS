// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 17.05.2025 22:00


#include <algorithm>
#include <GUI/Widgets/MVCGridView.h>
#include <GUI/Widgets/ScrollView.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMVCGridView::PMVCGridView(const PString& name, Ptr<PView> parent, uint32_t flags)
    : PMVCBaseView(name, parent, flags | PViewFlags::WillDraw)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PMVCGridView::PMVCGridView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData)
    : PMVCBaseView(context, parent, xmlData)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCGridView::OnLayoutChanged()
{
    PMVCBaseView::OnLayoutChanged();

    UpdateWidgets();

    if (m_FirstVisibleItem == INVALID_INDEX) {
        return;
    }

    const size_t firstRow = size_t(m_FirstVisibleItem) / m_ColumnCount;
    const size_t endRow = size_t(m_LastVisibleItem) / m_ColumnCount + 1;

    const float firstColumnCenter = roundf(m_GridSize.x * 0.5f + m_LeftMargin);
    PPoint centerPosition(0.0f, roundf(float(firstRow) * m_GridSize.y + m_GridSize.y * 0.5f));

    size_t itemIndex = m_FirstVisibleItem;

    for (size_t row = firstRow; row < endRow; ++row)
    {
        centerPosition.x = firstColumnCenter;
        for (size_t column = 0; column < size_t(m_ColumnCount); ++column)
        {
            if (itemIndex >= m_Items.size()) {
                return;
            }

            const PMVCGridViewItemNode& itemNode = m_Items[itemIndex];
            if (itemNode.ItemWidget != nullptr)
            {
                const PPoint preferredSize = itemNode.ItemWidget->GetPreferredSize(PPrefSizeType::Smallest);
                itemNode.ItemWidget->SetFrame(PRect::Centered(centerPosition, preferredSize));
            }

            centerPosition.x += m_GridSize.x;

            itemIndex++;
        }
        centerPosition.y += m_GridSize.y;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PMVCGridView::CalculateContentSize() const
{
    if (m_ColumnCount > 0 && !m_Items.empty())
    {
        return PPoint(0.0f, float((m_Items.size() + m_ColumnCount - 1) / m_ColumnCount) * m_GridSize.y);
    }
    return PPoint(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCGridView::SetGridSize(const PPoint& gridSize)
{
    if (gridSize != m_GridSize)
    {
        m_GridSize = gridSize;
        InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCGridView::AddItem(Ptr<PtrTarget> item)
{
    uint32_t classID = VFGetItemWidgetClassID.Empty() ? 0 : VFGetItemWidgetClassID(item);
    m_Items.emplace_back(item, nullptr, classID, false);
    m_ContentView->ContentSizeChanged();
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCGridView::Clear()
{
    m_Items.clear();
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PMVCGridView::GetItemIndexAtPosition(const PPoint& position) const
{
    const int32_t x = int32_t((position.x - m_LeftMargin) / m_GridSize.x);

    if (x < m_ColumnCount)
    {
        const int32_t y = int32_t(position.y / m_GridSize.y);
        const int32_t index = y * m_ColumnCount + x;

        if (index < m_Items.size()) {
            return index;
        }
    }
    return INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCGridView::OnContentViewFrameSized(const PPoint& delta)
{
    const PRect& bounds = m_ContentView->GetBounds();

    const int32_t columnCount = int32_t(bounds.Width() / m_GridSize.x);
    const float   leftMargin = roundf((bounds.Width() - float(columnCount) * m_GridSize.x) * 0.5f);

    if (columnCount != m_ColumnCount || leftMargin != m_LeftMargin)
    {
        m_ColumnCount = columnCount;
        m_LeftMargin = leftMargin;
        m_ContentView->ContentSizeChanged();
    }
    InvalidateLayout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCGridView::UpdateWidgets()
{
    const PRect& bounds = m_ContentView->GetBounds();
    if (bounds.IsValid() && m_ColumnCount > 0 && !m_Items.empty())
    {
        const size_t firstRow = size_t(floorf(std::max(0.0f, bounds.top) / m_GridSize.y));
        const size_t endRow = size_t(ceilf(std::max(0.0f, bounds.bottom) / m_GridSize.y));

        const size_t firstItemIndex = firstRow * m_ColumnCount;
        const size_t endItemIndex = std::min<size_t>(endRow * m_ColumnCount, m_Items.size());

        if (firstItemIndex < endItemIndex)
        {
            CreateWidgetsForRange(firstItemIndex, endItemIndex - 1);
            return;
        }
    }
    RemoveAllWidgets();
}
