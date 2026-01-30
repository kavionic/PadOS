// This file is part of PadOS.
//
// Copyright (C) 1999-2025 Kurt Skauen <http://kavionic.com/>
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

#include "ListViewColumnView.h"
#include "ListViewScrolledView.h"
#include <GUI/Widgets/ListView.h>
#include <GUI/Widgets/ListViewRow.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PListViewColumnView::PListViewColumnView(Ptr<PListViewScrolledView> parent, const PString& title)
    : PView("_lv_column", parent, PViewFlags::WillDraw)
    , m_Title(title)
{
    m_ContentWidth = 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PListViewColumnView::~PListViewColumnView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewColumnView::Refresh(const PRect& updateRect)
{
    Ptr<PListViewScrolledView> parent = ptr_static_cast<PListViewScrolledView>(GetParent());

    if (parent == nullptr) {
        return;
    }

    if (parent->m_Rows.empty())
    {
        SetFgColor(255, 255, 255);
        FillRect(updateRect);
        return;
    }
//    const Rect bounds = GetBounds();

    Ptr<PListViewRow> lastRow = parent->m_Rows[parent->m_Rows.size() - 1];
    if (updateRect.top > lastRow->m_YPos + lastRow->m_Height + parent->m_VSpacing)
    {
        SetFgColor(255, 255, 255);
        FillRect(updateRect);
        return;
    }

    PRect frame = GetBounds(); // (bounds.left, 0.0f, bounds.right, 0.0f);

    const size_t firstRow = parent->GetRowIndex(updateRect.top);

    if (firstRow != INVALID_INDEX)
    {
        size_t column = std::find(parent->m_ColumnViews.begin(), parent->m_ColumnViews.end(), this) - parent->m_ColumnViews.begin();

        std::vector<Ptr<PListViewRow>>& rowList = parent->m_Rows;
        bool hasFocus = false; // parent->m_ListView->HasFocus();


        for (size_t i = firstRow; i < rowList.size(); ++i)
        {
            if (rowList[i]->m_YPos > updateRect.bottom) {
                break;
            }
            frame.top = rowList[i]->m_YPos;
            frame.bottom = frame.top + rowList[i]->m_Height + parent->m_VSpacing;

            rowList[i]->Paint(frame, ptr_tmp_cast(this), column, rowList[i]->m_Selected, rowList[i]->m_Highlighted, hasFocus);
        }
    }
    if (frame.bottom < updateRect.bottom)
    {
        // Clear area below last row.
        frame.top = frame.bottom;
        frame.bottom = updateRect.bottom;
        SetFgColor(255, 255, 255);
        FillRect(frame);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewColumnView::OnPaint(const PRect& updateRect)
{
    if (!updateRect.IsValid()) {
        return; // FIXME: Workaround for appserver bug. Fix appserver.
    }
    Ptr<PListViewScrolledView> parent = ptr_static_cast<PListViewScrolledView>(GetParent());
    assert(parent != nullptr);

    if (parent->m_IsSelecting)
    {
        parent->SetDrawingMode(PDrawingMode::Invert);
        parent->DrawFrame(parent->m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
        parent->SetDrawingMode(PDrawingMode::Copy);
    }
    Refresh(updateRect);
    if (parent->m_IsSelecting)
    {
        parent->SetDrawingMode(PDrawingMode::Invert);
        parent->DrawFrame(parent->m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
        parent->SetDrawingMode(PDrawingMode::Copy);
    }
}


