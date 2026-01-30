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

    if (m_RowCount == 0 || m_ColumnCount == 0) {
        return;
    }

    UpdateWidgets();

    const PRect& bounds = m_ContentView->GetBounds();

    const int32_t firstColumn   = int32_t(bounds.top / m_GridSize.y);

    const float column0center = roundf(m_GridSize.x * 0.5f + m_LeftMargin);
    PPoint centerPos(0.0f, roundf(float(firstColumn) * m_GridSize.y + m_GridSize.y * 0.5f));

    size_t index = firstColumn * m_ColumnCount;

    for (int32_t y = 0; y < m_RowCount; ++y)
    {
        centerPos.x = column0center;
        for (int32_t x = 0; x < m_ColumnCount; ++x)
        {
            if (index >= m_Items.size()) {
                return;
            }

            const PMVCGridViewItemNode& itemNode = m_Items[index];
            if (itemNode.ItemWidget != nullptr)
            {
                const PPoint preferredSize = itemNode.ItemWidget->GetPreferredSize(PPrefSizeType::Smallest);
                itemNode.ItemWidget->SetFrame(PRect::Centered(centerPos, preferredSize));
            }

            centerPos.x += m_GridSize.x;

            index++;
        }
        centerPos.y += m_GridSize.y;
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

    const int32_t rowCount = int32_t((bounds.Height() + m_GridSize.y - 1.0f) / m_GridSize.y);
    const int32_t columnCount = int32_t(bounds.Width() / m_GridSize.x);
    const float   leftMargin = roundf((bounds.Width() - float(columnCount) * m_GridSize.x) * 0.5f);

    if (rowCount != m_RowCount || columnCount != m_ColumnCount || leftMargin != m_LeftMargin)
    {
        m_RowCount = rowCount;
        m_ColumnCount = columnCount;
        m_LeftMargin = leftMargin;
        m_ContentView->ContentSizeChanged();
        InvalidateLayout();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PMVCGridView::UpdateWidgets()
{
    const PRect& bounds = m_ContentView->GetBounds();
    if (bounds.IsValid())
    {
        const int32_t firstColumn = int32_t(bounds.top / m_GridSize.y);
        const int32_t lastColumn = firstColumn + m_ColumnCount;

        const ssize_t firstItemIndex = std::max<ssize_t>(0, firstColumn * m_ColumnCount);
        const ssize_t lastItemIndex  = std::min<ssize_t>(lastColumn * m_ColumnCount, m_Items.size() - 1);

        CreateWidgetsForRange(firstItemIndex, lastItemIndex);
    }
    else
    {
        RemoveAllWidgets();
    }
}
