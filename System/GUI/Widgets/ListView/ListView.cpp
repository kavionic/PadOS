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

#include <stdio.h>
#include <limits.h>
#include <algorithm>

#include "ListViewScrolledView.h"
#include "ListViewHeaderView.h"
#include "ListViewColumnView.h"

#include <GUI/Widgets/ListView.h>
#include <GUI/Widgets/ListViewRow.h>
#include <GUI/Widgets/ScrollBar.h>
#include <Ptr/NoPtr.h>

using namespace os;

//  ListView -+
//            |
//            +-- ListViewHeaderView -+
//                                    |
//                                    +-- ListViewScrolledView -+
//                                                              |
//                                                              +-- ListViewColumnView
//                                                              |
//                                                              +-- ListViewColumnView
//                                                              |
//                                                              +-- ListViewColumnView
//
//
//  +--------------------------------------------------------------------------+
//  |                                 ListView                                 |
//  |+------------------------------------------------------------------------+|
//  ||                        |  ListViewHeaderView  |                        ||
//  || +--------------------------------------------------------------------+ ||
//  || |                        ListViewScrolledView                        | ||
//  || | +--------------------++--------------------++--------------------+ | ||
//  || | | ListViewColumnView || ListViewColumnView || ListViewColumnView | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | ^ ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | - ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | |                    ||                    ||                    | | ||
//  || | +--------------------++--------------------++--------------------+ | ||
//  || +--------------------------------------------------------------------+ ||
//  |+------<---------------------------------------------------------->------+|
//  +--------------------------------------------------------------------------+


const std::map<String, uint32_t> ListViewFlags::FlagMap
{
    DEFINE_FLAG_MAP_ENTRY(ListViewFlags, MultiSelect),
    DEFINE_FLAG_MAP_ENTRY(ListViewFlags, NoAutoSort),
    DEFINE_FLAG_MAP_ENTRY(ListViewFlags, RenderBorder),
    DEFINE_FLAG_MAP_ENTRY(ListViewFlags, DontScroll),
    DEFINE_FLAG_MAP_ENTRY(ListViewFlags, NoHeader),
    DEFINE_FLAG_MAP_ENTRY(ListViewFlags, NoColumnRemap)
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ListView::ListView(const String& name, Ptr<View> parent, uint32_t flags) :
    Control(name, parent, flags | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize)
{
    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ListView::ListView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData) : Control(context, parent, xmlData)
{
    MergeFlags(context.GetFlagsAttribute<uint32_t>(xmlData, ListViewFlags::FlagMap, "flags", ListViewFlags::RenderBorder) | ViewFlags::WillDraw | ViewFlags::FullUpdateOnResize);

    Construct();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Construct()
{
    m_HeaderView = ptr_new<ListViewHeaderView>(ptr_tmp_cast(this));
    m_ScrolledContainerView = m_HeaderView->m_ScrolledContainerView;

    Layout();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ListView::~ListView()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListView::IsMultiSelect() const
{
    return HasFlags(ListViewFlags::MultiSelect);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::SetMultiSelect(bool multi)
{
    if (multi) {
        MergeFlags(ListViewFlags::MultiSelect);
    } else {
        ClearFlags(ListViewFlags::MultiSelect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListView::IsAutoSort() const
{
    return !HasFlags(ListViewFlags::NoAutoSort);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::SetAutoSort(bool autoSort)
{
    if (autoSort) {
        ClearFlags(ListViewFlags::NoAutoSort);
    } else {
        MergeFlags(ListViewFlags::NoAutoSort);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListView::HasBorder() const
{
    return HasFlags(ListViewFlags::RenderBorder);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::SetRenderBorder(bool render)
{
    if (render) {
        MergeFlags(ListViewFlags::RenderBorder);
    } else {
        ClearFlags(ListViewFlags::RenderBorder);
    }
    Layout();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListView::HasColumnHeader() const
{
    return !HasFlags(ListViewFlags::NoHeader);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::SetHasColumnHeader(bool value)
{
    if (value) {
        ClearFlags(ListViewFlags::NoHeader);
    } else {
        MergeFlags(ListViewFlags::NoHeader);
    }
    Layout();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListView::HasFocus(MouseButton_e button) const
{
    if (View::HasFocus(button)) {
        return true;
    }
    return m_HeaderView->HasFocus(button);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void ListView::KeyDown(const char* pzString, const char* pzRawString, uint32 nQualifiers)
//{
//  if (m_ScrolledContainerView->HandleKey(pzString[0], nQualifiers) == false) {
//      View::KeyDown(pzString, pzRawString, nQualifiers);
//  }
//}


///////////////////////////////////////////////////////////////////////////////
/// NAME:
/// DESC:
///  Calculate and set the range and proportion of scroll bars.
///  Scroll the view so upper/left corner
///  is at or above/left of 0,0 if needed
/// NOTE:
/// SEE ALSO:
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::AdjustScrollBars(bool okToHScroll)
{
    if (m_ScrolledContainerView->m_Rows.empty())
    {
        if (m_VScrollBar != nullptr) {
            RemoveChild(m_VScrollBar);
        }
        if (m_HScrollBar != nullptr) {
            RemoveChild(m_HScrollBar);
        }
        if (m_VScrollBar != nullptr || m_HScrollBar != nullptr)
        {
            m_VScrollBar = nullptr;
            m_HScrollBar = nullptr;
            m_ScrolledContainerView->ScrollTo(0, 0);
            m_HeaderView->ScrollTo(0, 0);
            Layout();
        }
    }
    else
    {
        float viewHeight = m_ScrolledContainerView->GetBounds().Height();
        float viewWidth = m_HeaderView->GetBounds().Width();
        const float contentHeight = m_ScrolledContainerView->m_ContentHeight;

        float proportion;
        if (contentHeight > 0.0f && contentHeight > viewHeight) {
            proportion = viewHeight / contentHeight;
        } else {
            proportion = 1.0f;
        }
        if (contentHeight > viewHeight)
        {
            if (m_VScrollBar == nullptr)
            {
                m_VScrollBar = ptr_new<ScrollBar>("v_scroll", ptr_tmp_cast(this), 0.0f, 1000.0f, Orientation::Vertical);
                m_VScrollBar->SetScrollTarget(m_ScrolledContainerView);
                Layout();
            }
            else
            {
                m_VScrollBar->SetSteps(ceil(contentHeight / float(m_ScrolledContainerView->m_Rows.size())), ceil(viewHeight * 0.8f));
                m_VScrollBar->SetProportion(proportion);
                m_VScrollBar->SetMinMax(0, std::max(contentHeight - viewHeight, 0.0f));
            }
        }
        else
        {
            if (m_VScrollBar != nullptr)
            {
                RemoveChild(m_VScrollBar);
                m_VScrollBar = nullptr;
                m_ScrolledContainerView->ScrollTo(0, 0);
                Layout();
            }
        }

        if (m_VScrollBar != nullptr) {
            viewWidth -= m_VScrollBar->GetFrame().Width();
        }

        if (m_ScrolledContainerView->m_TotalWidth > 0.0f && m_ScrolledContainerView->m_TotalWidth > viewWidth) {
            proportion = viewWidth / m_ScrolledContainerView->m_TotalWidth;
        } else {
            proportion = 1.0f;
        }

        if (m_ScrolledContainerView->m_TotalWidth > viewWidth)
        {
            if (m_HScrollBar == nullptr)
            {
                m_HScrollBar = ptr_new<ScrollBar>("h_scroll", ptr_tmp_cast(this), 0.0f, 1000.0f, Orientation::Horizontal);
                m_HScrollBar->SetScrollTarget(m_HeaderView);
                Layout();
            }
            else
            {
                m_HScrollBar->SetSteps(15.0f, ceil(viewWidth * 0.8f));
                m_HScrollBar->SetProportion(proportion);
                m_HScrollBar->SetMinMax(0.0f, m_ScrolledContainerView->m_TotalWidth - viewWidth);
            }
        }
        else
        {
            if (m_HScrollBar != nullptr)
            {
                RemoveChild(m_HScrollBar);
                m_HScrollBar = nullptr;
                m_HeaderView->ScrollTo(0.0f, 0.0f);
                Layout();
            }
        }
        if (okToHScroll)
        {
            const float offset = m_HeaderView->GetScrollOffset().x;
            if (offset < 0.0f)
            {
                viewWidth = m_HeaderView->GetBounds().Width();
                if (viewWidth - offset > m_ScrolledContainerView->m_TotalWidth) {
                    float deltaScroll = std::min((viewWidth - offset) - m_ScrolledContainerView->m_TotalWidth, -offset);
                    m_HeaderView->ScrollBy(deltaScroll, 0);
                } else if (viewWidth > m_ScrolledContainerView->m_TotalWidth) {
                    m_HeaderView->ScrollBy(-offset, 0);
                }
            }
        }
        const float offset = m_ScrolledContainerView->GetScrollOffset().y;
        if (offset < 0.0f)
        {
            viewHeight = m_ScrolledContainerView->GetBounds().Height();
            if (viewHeight - offset > contentHeight) {
                float nDeltaScroll = std::min((viewHeight - offset) - contentHeight, -offset);
                m_ScrolledContainerView->ScrollBy(0.0f, nDeltaScroll);
            } else if (viewHeight > contentHeight) {
                m_ScrolledContainerView->ScrollBy(0.0f, -offset);
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Layout()
{
    const float topHeight = m_HeaderView->m_HeaderHeight;

    Rect frame(GetBounds().Bounds());
    frame.Floor();
    if (HasFlags(ListViewFlags::RenderBorder)) {
        frame.Resize(2.0f, 2.0f, -2.0f, -2.0f);
    }

    Rect headerFrame(frame);

    if (m_HScrollBar != nullptr) {
        headerFrame.bottom -= 16.0f;
    }
    if (m_VScrollBar != nullptr)
    {
        Rect scrollBarFrame(frame);

        scrollBarFrame.top += topHeight;
        //  cVScrFrame.bottom -= 2.0f;
        if (m_HScrollBar != nullptr) {
            scrollBarFrame.bottom -= 16.0f;
        }
        scrollBarFrame.left = scrollBarFrame.right - 16.0f;

        m_VScrollBar->SetFrame(scrollBarFrame);
    }
    if (m_HScrollBar != nullptr)
    {
        Rect scrollBarFrame(frame);
        headerFrame.bottom = floor(headerFrame.bottom);
        if (m_VScrollBar != nullptr) {
            scrollBarFrame.right -= 16.0f;
        }
        //  cVScrFrame.left  += 2.0f;
        //  cVScrFrame.right -= 2.0f;
        scrollBarFrame.top = scrollBarFrame.bottom - 16.0f;
        headerFrame.bottom = scrollBarFrame.top;

        m_HScrollBar->SetFrame(scrollBarFrame);
    }
    m_HeaderView->SetFrame(headerFrame);
    AdjustScrollBars();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::OnFrameSized(const Point& delta)
{
    if (m_ScrolledContainerView->m_ColumnMap.empty() == false)
    {
        Ptr<ListViewColumnView> columnView = m_ScrolledContainerView->m_ColumnViews[m_ScrolledContainerView->m_ColumnMap[m_ScrolledContainerView->m_ColumnMap.size() - 1]];
        Rect frame = columnView->GetFrame();
        frame.right = GetBounds().right - m_HeaderView->GetScrollOffset().x;

        columnView->SetFrame(frame);
    }
    Layout();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Sort()
{
    if (m_ScrolledContainerView->m_SortColumn >= m_ScrolledContainerView->m_ColumnViews.size()) {
        return;
    }
    if (m_ScrolledContainerView->m_Rows.empty()) {
        return;
    }

    std::stable_sort(m_ScrolledContainerView->m_Rows.begin(), m_ScrolledContainerView->m_Rows.end(), [sortColumn = m_ScrolledContainerView->m_SortColumn](Ptr<const ListViewRow> lhs, Ptr<const ListViewRow> rhs) { return lhs->IsLessThan(rhs, sortColumn); });

    float y = 0.0f;
    for (size_t i = 0; i < m_ScrolledContainerView->m_Rows.size(); ++i) {
        m_ScrolledContainerView->m_Rows[i]->m_YPos = y;
        y += m_ScrolledContainerView->m_Rows[i]->m_Height + m_ScrolledContainerView->m_VSpacing;
    }
    if (m_ScrolledContainerView->m_FirstSel != INVALID_INDEX)
    {
        for (size_t i = 0; i < m_ScrolledContainerView->m_Rows.size(); ++i)
        {
            if (m_ScrolledContainerView->m_Rows[i]->m_Selected) {
                m_ScrolledContainerView->m_FirstSel = i;
                break;
            }
        }
        for (int i = m_ScrolledContainerView->m_Rows.size() - 1; i >= 0; --i)
        {
            if (m_ScrolledContainerView->m_Rows[i]->m_Selected) {
                m_ScrolledContainerView->m_LastSel = i;
                break;
            }
        }
    }
    for (size_t i = 0; i < m_ScrolledContainerView->m_ColumnViews.size(); ++i) {
        m_ScrolledContainerView->m_ColumnViews[i]->Invalidate();
    }
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::MakeVisible(size_t row, bool center)
{
    m_ScrolledContainerView->MakeVisible(row, center);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListView::InsertColumn(const char* title, float width, size_t index)
{
    const size_t column = m_ScrolledContainerView->InsertColumn(title, width, index);
    Layout();
    Flush();
    return column;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const ListView::ColumnMap& ListView::GetColumnMapping() const
{
    return m_ScrolledContainerView->m_ColumnMap;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::SetColumnMapping(const ColumnMap& map)
{
    for (size_t i = 0; i < m_ScrolledContainerView->m_ColumnViews.size(); ++i)
    {
        if (std::find(map.begin(), map.end(), i) == map.end())
        {
            if (std::find(m_ScrolledContainerView->m_ColumnMap.begin(), m_ScrolledContainerView->m_ColumnMap.end(), i) != m_ScrolledContainerView->m_ColumnMap.end()) {
                m_ScrolledContainerView->m_ColumnViews[i]->Show(false);
            }
        }
        else
        {
            if (std::find(m_ScrolledContainerView->m_ColumnMap.begin(), m_ScrolledContainerView->m_ColumnMap.end(), i) == m_ScrolledContainerView->m_ColumnMap.end()) {
                m_ScrolledContainerView->m_ColumnViews[i]->Show(true);
            }
        }
    }
    m_ScrolledContainerView->m_ColumnMap = map;
    m_ScrolledContainerView->LayoutColumns();
    m_HeaderView->Invalidate();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::InsertRow(size_t index, Ptr<ListViewRow> row, bool update)
{
    m_ScrolledContainerView->InsertRow(index, row, update);
    AdjustScrollBars();
    if (update) {
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::InsertRow(Ptr<ListViewRow> row, bool update)
{
    InsertRow(INVALID_INDEX, row, update);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ListViewRow> ListView::RemoveRow(size_t index, bool update)
{
    Ptr<ListViewRow> row = m_ScrolledContainerView->RemoveRow(index, update);
    AdjustScrollBars();
    if (update) {
        Flush();
    }
    return row;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::InvalidateRow(size_t row, uint32_t flags)
{
    m_ScrolledContainerView->InvalidateRow(row, flags);
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListView::GetRowCount() const
{
    return m_ScrolledContainerView->m_Rows.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ListViewRow> ListView::GetRow(size_t index) const
{
    if (index < m_ScrolledContainerView->m_Rows.size()) {
        return m_ScrolledContainerView->m_Rows[index];
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ListViewRow> ListView::GetRow(const Point& pos) const
{
    const size_t hitRow = m_ScrolledContainerView->GetRowIndex(pos.y);
    if (hitRow != INVALID_INDEX) {
        return m_ScrolledContainerView->m_Rows[hitRow];
    } else {
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListView::GetRowIndex(Ptr<ListViewRow> row) const
{
    return m_ScrolledContainerView->GetRowIndex(row);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListView::HitTest(const Point& pos) const
{
    const Point parentPos = ConvertToRoot(m_ScrolledContainerView->ConvertFromRoot(pos));
    return m_ScrolledContainerView->GetRowIndex(parentPos.y);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float ListView::GetRowPos(size_t row)
{
    if (row < m_ScrolledContainerView->m_Rows.size()) {
        return m_ScrolledContainerView->m_Rows[row]->m_YPos + m_ScrolledContainerView->GetFrame().top + m_ScrolledContainerView->GetScrollOffset().y;
    } else {
        return 0.0f;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Select(size_t first, size_t last, bool replace, bool select)
{
    if (m_ScrolledContainerView->m_Rows.empty()) {
        return;
    }
    if ((replace || !HasFlags(ListViewFlags::MultiSelect)) && m_ScrolledContainerView->m_FirstSel != INVALID_INDEX)
    {
        for (size_t i = m_ScrolledContainerView->m_FirstSel; i <= m_ScrolledContainerView->m_LastSel; ++i)
        {
            if (m_ScrolledContainerView->m_Rows[i]->m_Selected)
            {
                m_ScrolledContainerView->m_Rows[i]->m_Selected = false;
                m_ScrolledContainerView->InvalidateRow(i, INV_VISUAL);
            }
        }
        m_ScrolledContainerView->m_FirstSel = INVALID_HANDLE;
        m_ScrolledContainerView->m_LastSel  = INVALID_HANDLE;
    }
    if (m_ScrolledContainerView->SelectRange(first, last, select)) {
        SelectionChanged(m_ScrolledContainerView->m_FirstSel, m_ScrolledContainerView->m_LastSel);
    }
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Select(size_t row, bool replace, bool select)
{
    Select(row, row, replace, select);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::ClearSelection()
{
    if (m_ScrolledContainerView->ClearSelection()) {
        SelectionChanged(INVALID_INDEX, INVALID_INDEX);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Highlight(size_t first, size_t last, bool replace, bool highlight)
{
    if (replace)
    {
        for (size_t i = 0; i < m_ScrolledContainerView->m_Rows.size(); ++i)
        {
            bool highlighted = i >= first && i <= last;
            if (m_ScrolledContainerView->m_Rows[i]->m_Highlighted != highlighted)
            {
                m_ScrolledContainerView->m_Rows[i]->m_Highlighted = highlighted;
                m_ScrolledContainerView->InvalidateRow(i, ListView::INV_VISUAL);
            }
        }
    }
    else if (first != INVALID_INDEX && last != INVALID_INDEX)
    {
        for (size_t i = first; i <= last; ++i)
        {
            if (m_ScrolledContainerView->m_Rows[i]->m_Highlighted != highlight)
            {
                m_ScrolledContainerView->m_Rows[i]->m_Highlighted = highlight;
                m_ScrolledContainerView->InvalidateRow(i, ListView::INV_VISUAL);
            }
        }
    }
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Highlight(size_t row, bool replace, bool highlight)
{
    Highlight(row, row, replace, highlight);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListView::IsSelected(size_t row) const
{
    if (row < m_ScrolledContainerView->m_Rows.size()) {
        return m_ScrolledContainerView->m_Rows[row]->m_Selected;
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::Clear()
{
    m_ScrolledContainerView->Clear();
    AdjustScrollBars();
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::OnPaint(const Rect& updateRect)
{
    if (m_VScrollBar != nullptr && m_HScrollBar != nullptr)
    {
        const Rect scrollBarFrameH = m_HScrollBar->GetFrame();
        const Rect scrollBarFrameV = m_VScrollBar->GetFrame();
        const Rect frame(scrollBarFrameH.right, scrollBarFrameV.bottom, scrollBarFrameV.right, scrollBarFrameH.bottom);
        FillRect(frame, get_standard_color(StandardColorID::DefaultBackground));
    }
    if (HasFlags(ListViewFlags::RenderBorder)) {
        DrawFrame(GetBounds(), FRAME_RECESSED | FRAME_TRANSPARENT);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::SelectionChanged(size_t firstRow, size_t lastRow)
{
    SignalSelectionChanged(firstRow, lastRow, ptr_tmp_cast(this));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListView::GetFirstSelected() const
{
    return m_ScrolledContainerView->m_FirstSel;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListView::GetLastSelected() const
{
    return m_ScrolledContainerView->m_LastSel;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::StartScroll(ScrollDirection direction, bool select)
{
    m_ScrolledContainerView->StartScroll(direction, select);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListView::StopScroll()
{
    m_ScrolledContainerView->StopScroll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListView::DragSelection(const Point& pos)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ListView::const_iterator ListView::begin() const
{
    return m_ScrolledContainerView->m_Rows.begin();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ListView::const_iterator ListView::end() const
{
    return m_ScrolledContainerView->m_Rows.end();
}
