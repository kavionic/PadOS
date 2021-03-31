// This file is part of PadOS.
//
// Copyright (C) 1999-2020 Kurt Skauen <http://kavionic.com/>
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

#include "ListViewScrolledView.h"
#include "ListViewColumnView.h"
#include <GUI/Widgets/ListView.h>
#include <GUI/Widgets/ListViewRow.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ListViewScrolledView::ListViewScrolledView(Ptr<ListView> listView)
    : View("main_view", nullptr, ViewFlags::WillDraw | ViewFlags::DrawOnChildren)
{
    m_ListView = ptr_raw_pointer_cast(listView);

    m_InertialScroller.SetScrollHBounds(0.0f, 0.0f);
    m_InertialScroller.SignalUpdate.Connect(this, &ListViewScrolledView::SlotInertialScrollUpdate);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::HasFocus(MouseButton_e button) const
{
    if (View::HasFocus(button)) {
        return true;
    }
    for (size_t i = 0; i < m_ColumnMap.size(); ++i) {
        if (m_ColumnViews[m_ColumnMap[i]]->HasFocus(button)) {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::DetachedFromWindow()
{
    StopScroll();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::StartScroll(ScrollDirection direction, bool select)
{
    m_AutoScrollSelects = select;

    if (direction == ScrollDirection::Down)
    {
        if (!m_AutoScrollDown)
        {
            m_AutoScrollDown = true;
            if (!m_AutoScrollUp)
            {
                Looper* looper = GetLooper();
                if (looper != nullptr) {
                    //                  looper->AddTimer(this, AUTOSCROLL_TIMER, 50000, false);
                }
            }
            else
            {
                m_AutoScrollUp = false;
            }
        }
    }
    else
    {
        if (!m_AutoScrollUp)
        {
            m_AutoScrollUp = true;
            if (!m_AutoScrollDown)
            {
                Looper* looper = GetLooper();
                if (looper != nullptr) {
                    //                  looper->AddTimer(this, AUTOSCROLL_TIMER, 50000, false);
                }
            }
            else
            {
                m_AutoScrollDown = false;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::StopScroll()
{
    if (m_AutoScrollUp || m_AutoScrollDown)
    {
        m_AutoScrollUp = m_AutoScrollDown = false;
        Looper* looper = GetLooper();
        if (looper != nullptr) {
            //          looper->RemoveTimer(this, AUTOSCROLL_TIMER);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (m_HitButton != MouseButton_e::None) {
        return true;
    }
    m_HitButton = pointID;

    const Rect bounds = GetBounds();
    if (m_ContentHeight > bounds.Height())
    {
        m_InertialScroller.BeginDrag(GetScrollOffset(), ConvertToRoot(position));
    }
    m_MouseMoved = false;
    MakeFocus(pointID, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return true;
    }
    m_HitButton = MouseButton_e::None;
    MakeFocus(pointID, false);

    m_InertialScroller.EndDrag();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event)
{
    if (pointID != m_HitButton) {
        return true;
    }
    m_MouseMoved = true;
    m_InertialScroller.AddUpdate(ConvertToRoot(position));
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::SlotInertialScrollUpdate(const Point& position)
{
    ScrollTo(position);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    MakeFocus(button, true);

    if (m_Rows.empty()) {
        return false;
    }

    m_MousDownSeen = true;

    size_t hitColumn = INVALID_INDEX;
    size_t hitRowIndex;
    if (position.y >= m_ContentHeight) {
        hitRowIndex = INVALID_INDEX;
    } else {
        hitRowIndex = GetRowIndex(position.y);
    }

    m_LastHitRow = hitRowIndex;
    if (hitRowIndex == INVALID_INDEX) {
        return true;
    }

    Ptr<ListViewRow> hitRow = m_Rows[hitRowIndex];

    TimeValMicros curTime = get_system_time();
    bool  doubleClick = false;
    if (hitRowIndex == m_LastHitRow && curTime - m_MouseDownTime < TimeValMicros::FromMilliseconds(500)) {
        doubleClick = true;
    }
    m_MouseDownTime = curTime;

    uint32_t qualifiers = GetApplication()->GetQualifiers();

    if ((qualifiers & QUAL_SHIFT) && m_ListView->HasFlags(ListViewFlags::MultiSelect)) {
        m_EndSel = m_BeginSel;
    } else {
        m_BeginSel = hitRowIndex;
        m_EndSel = hitRowIndex;
    }

    if (doubleClick) {
        m_ListView->SignalItemClicked(m_FirstSel, m_LastSel, ptr_tmp_cast(m_ListView));
        return true;
    }
    if (hitRow != nullptr)
    {
        for (size_t i = 0; i < m_ColumnMap.size(); ++i)
        {
            Rect frame = GetColumn(i)->GetFrame();
            if (position.x >= frame.left && position.x < frame.right)
            {
                frame.top = hitRow->m_YPos;
                frame.bottom = frame.top + hitRow->m_Height + m_VSpacing;
                if (hitRow->HitTest(GetColumn(i), frame, i, position)) {
                    hitColumn = i;
                }
                break;
            }
        }
    }
    m_MouseMoved = false;

    if (hitColumn == 1 && (qualifiers & QUAL_CTRL) == 0 && m_Rows[hitRowIndex]->m_Selected)
    {
        m_DragIfMoved = true;
    }
    else
    {
        ExpandSelect(hitRowIndex, (qualifiers & QUAL_CTRL), ((qualifiers & (QUAL_CTRL | QUAL_SHIFT)) == 0 || !m_ListView->HasFlags(ListViewFlags::MultiSelect)));
        if (m_ListView->HasFlags(ListViewFlags::MultiSelect))
        {
            m_IsSelecting = true;
            m_SelectRect = Rect(position.x, position.y, position.x, position.y);
            SetDrawingMode(DrawingMode::Invert);
            DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
            SetDrawingMode(DrawingMode::Copy);
        }
    }
    Flush();
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    m_MousDownSeen = false;

    Looper* looper = GetLooper();
    if (looper != nullptr) {
        //      looper->RemoveTimer(this, AUTOSCROLL_TIMER);
    }

    if (m_DragIfMoved) {
        ExpandSelect(m_EndSel, false, true);
        m_DragIfMoved = false;
    }

    if (m_IsSelecting) {
        SetDrawingMode(DrawingMode::Invert);
        DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
        SetDrawingMode(DrawingMode::Copy);
        m_IsSelecting = false;
        Flush();
    }
    //  if (pcData != NULL) {
    //      View::OnMouseUp(button, position);
    //      return;
    //  }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    m_MouseMoved = true;
    if (!m_MousDownSeen) {
        return false;
    }
    if (m_DragIfMoved)
    {
        m_DragIfMoved = false;
        if (m_ListView->DragSelection(m_ListView->ConvertFromRoot(ConvertToRoot(position))))
        {
            m_MousDownSeen = false;
            if (m_IsSelecting)
            {
                SetDrawingMode(DrawingMode::Invert);
                DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
                SetDrawingMode(DrawingMode::Copy);
                m_IsSelecting = false;
            }
            return true;
        }
    }

    if (m_IsSelecting)
    {
        SetDrawingMode(DrawingMode::Invert);
        DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
        SetDrawingMode(DrawingMode::Copy);
        m_SelectRect.right = position.x;
        m_SelectRect.bottom = position.y;

        size_t hitRowIndex;
        if (position.y >= m_ContentHeight)
        {
            hitRowIndex = m_Rows.size() - 1;
        }
        else
        {
            hitRowIndex = GetRowIndex(position.y);
            if (hitRowIndex == INVALID_INDEX) {
                hitRowIndex = 0;
            }
        }
        if (hitRowIndex != m_EndSel) {
            ExpandSelect(hitRowIndex, true, false);
        }
        SetDrawingMode(DrawingMode::Invert);
        DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
        SetDrawingMode(DrawingMode::Copy);
        Flush();
        Rect bounds = GetBounds();

        if (position.y < bounds.top + ListView::AUTOSCROLL_BORDER) {
            StartScroll(ScrollDirection::Down, true);
        } else if (position.y > bounds.bottom - ListView::AUTOSCROLL_BORDER) {
            StartScroll(ScrollDirection::Up, true);
        } else {
            StopScroll();
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void ListViewScrolledView::WheelMoved(const Point& delta)
//{
//  if (GetVScrollBar() != NULL) {
//      GetVScrollBar()->WheelMoved(delta);
//  } else {
//      View::WheelMoved(delta);
//  }
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListViewScrolledView::GetRowIndex(float y) const
{
    if (y < 0.0f || m_Rows.empty()) {
        return INVALID_INDEX;
    }

    auto iterator = std::upper_bound(m_Rows.begin(), m_Rows.end(), y, [](float lhs, Ptr<const ListViewRow> rhs) { return lhs < rhs->m_YPos; });
    int index = iterator - m_Rows.begin() - 1;
    if (index < 0 || y > (m_Rows[index]->m_YPos + m_Rows[index]->m_Height + m_VSpacing)) {
        return INVALID_INDEX;
    }
    return index;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListViewScrolledView::GetRowIndex(Ptr<const ListViewRow> row) const
{
    auto iterator = std::find(m_Rows.begin(), m_Rows.end(), row);
    if (iterator != m_Rows.end()) {
        return iterator - m_Rows.begin();
    } else {
        return INVALID_INDEX;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::FrameSized(const Point& delta)
{
    LayoutColumns();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//void ListViewScrolledView::TimerTick(int nID)
//{
//  if (nID != AUTOSCROLL_TIMER) {
//      View::TimerTick(nID);
//      return;
//  }
//
//  Rect cBounds = GetBounds();
//
//  if (cBounds.Height() >= m_ContentHeight) {
//      return;
//  }
//  Point cMousePos;
//  GetMouse(&cMousePos, NULL);
//
//  float vPrevScroll = GetScrollOffset().y;
//  float vCurScroll = vPrevScroll;
//
//  if (m_AutoScrollDown)
//  {
//      float nScrollStep = (cBounds.top + ListView::AUTOSCROLL_BORDER) - cMousePos.y;
//      nScrollStep /= 2.0f;
//      nScrollStep++;
//
//      vCurScroll += nScrollStep;
//
//      if (vCurScroll > 0) {
//          vCurScroll = 0;
//      }
//  }
//  else if (m_AutoScrollUp)
//  {
//      float vMaxScroll = -(m_ContentHeight - GetBounds().Height());
//
//      float vScrollStep = cMousePos.y - (cBounds.bottom - ListView::AUTOSCROLL_BORDER);
//      vScrollStep *= 0.5f;
//      vScrollStep = vScrollStep + 1.0f;
//
//      vCurScroll -= vScrollStep;
//      cMousePos.y += vScrollStep;
//
//      if (vCurScroll < vMaxScroll) {
//          vCurScroll = vMaxScroll;
//      }
//  }
//  if (vCurScroll != vPrevScroll) {
//      if (m_IsSelecting) {
//          SetDrawingMode(DrawingMode::DM_INVERT);
//          DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
//          SetDrawingMode(DrawingMode::Copy);
//      }
//      ScrollTo(0, vCurScroll);
//      if (m_IsSelecting) {
//          m_SelectRect.right = cMousePos.x;
//          m_SelectRect.bottom = cMousePos.y;
//
//          int hitRow;
//          if (cMousePos.y >= m_ContentHeight) {
//              hitRow = m_Rows.size() - 1;
//          } else {
//              hitRow = GetRowIndex(cMousePos.y);
//              if (hitRow < 0) {
//                  hitRow = 0;
//              }
//          }
//
//          if (hitRow != m_EndSel) {
//              ExpandSelect(hitRow, true, false);
//          }
//          SetDrawingMode(DrawingMode::DM_INVERT);
//          DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
//          SetDrawingMode(DrawingMode::Copy);
//      }
//      Flush();
//  }
//}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

//bool ListViewScrolledView::HandleKey(char nChar, uint32_t nQualifiers)
//{
//  if (m_Rows.empty()) {
//      return(false);
//  }
//  switch (nChar)
//  {
//      case 0:
//          return(false);
//      case VK_PAGE_UP:
//      case VK_UP_ARROW:
//      {
//          int nPageHeight = 0;
//
//          if (m_EndSel == 0) {
//              return(true);
//          }
//          if (m_Rows.empty() == false) {
//              nPageHeight = int(GetBounds().Height() / (m_ContentHeight / float(m_Rows.size())));
//          }
//
//          if (m_EndSel == -1) {
//              m_EndSel = m_BeginSel;
//          }
//          if (m_BeginSel == -1) {
//              m_BeginSel = m_EndSel = m_Rows.size() - 1;
//              ExpandSelect(m_EndSel, false, (nQualifiers & QUAL_SHIFT) == 0 || !m_ListView->HasFlags(ListViewFlags::MultiSelect));
//          } else {
//              if ((nQualifiers & QUAL_SHIFT) == 0 || !m_ListView->HasFlags(ListViewFlags::MultiSelect)) {
//                  m_BeginSel = -1;
//              }
//              ExpandSelect(m_EndSel - ((nChar == VK_UP_ARROW) ? 1 : nPageHeight),
//                  false, (nQualifiers & QUAL_SHIFT) == 0 || !m_ListView->HasFlags(ListViewFlags::MultiSelect));
//          }
//          if ((nQualifiers & QUAL_SHIFT) == 0) {
//              m_BeginSel = m_EndSel;
//          }
//          if (m_Rows[m_EndSel]->m_YPos < GetBounds().top) {
//              MakeVisible(m_EndSel, false);
//          }
//          Flush();
//          return(true);
//      }
//      case VK_PAGE_DOWN:
//      case VK_DOWN_ARROW:
//      {
//          int nPageHeight = 0;
//
//          if (m_EndSel == int(m_Rows.size()) - 1) {
//              return(true);
//          }
//
//          if (m_Rows.size() > 0) {
//              nPageHeight = int(GetBounds().Height() / (m_ContentHeight / float(m_Rows.size())));
//          }
//
//          if (m_EndSel == -1) {
//              m_EndSel = m_BeginSel;
//          }
//          if (m_BeginSel == -1) {
//              m_BeginSel = m_EndSel = 0;
//              ExpandSelect(m_EndSel, false, (nQualifiers & QUAL_SHIFT) == 0 || !m_ListView->HasFlags(ListViewFlags::MultiSelect));
//          } else {
//              if ((nQualifiers & QUAL_SHIFT) == 0 || !m_ListView->HasFlags(ListViewFlags::MultiSelect)) {
//                  m_BeginSel = -1;
//              }
//              ExpandSelect(m_EndSel + ((nChar == VK_DOWN_ARROW) ? 1 : nPageHeight),
//                  false, (nQualifiers & QUAL_SHIFT) == 0 || !m_ListView->HasFlags(ListView::MultiSelect));
//          }
//          if ((nQualifiers & QUAL_SHIFT) == 0) {
//              m_BeginSel = m_EndSel;
//          }
//          if (m_Rows[m_EndSel]->m_YPos + m_Rows[m_EndSel]->m_Height + m_VSpacing > GetBounds().bottom) {
//              MakeVisible(m_EndSel, false);
//          }
//          Flush();
//          return(true);
//      }
//      case '\n':
//          if (m_FirstSel >= 0) {
//              m_ListView->Invoked(m_FirstSel, m_LastSel);
//          }
//          return(true);
//      default:
//          return(false);
//  }
//}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::MakeVisible(size_t row, bool center)
{
    float   totRowHeight = m_Rows[row]->m_Height + m_VSpacing;
    float   y = m_Rows[row]->m_YPos;
    Rect    bounds = GetBounds();
    float   viewHeight = bounds.Height();

    if (m_ContentHeight <= viewHeight)
    {
        ScrollTo(0.0f, 0.0f);
    }
    else
    {
        if (center)
        {
            float offset = y - viewHeight * 0.5f + totRowHeight * 0.5f;
            if (offset < 0.0f) {
                offset = 0.0f;
            } else if (offset >= m_ContentHeight - 1.0f) {
                offset = m_ContentHeight - 1.0f;
            }
            if (offset < 0.0f) {
                offset = 0.0f;
            } else if (offset > m_ContentHeight - viewHeight) {
                offset = m_ContentHeight - viewHeight;
            }
            ScrollTo(0, -offset);
        }
        else
        {
            float offset;
            if (y + totRowHeight * 0.5f < bounds.top + viewHeight * 0.5f) {
                offset = y;
            } else {
                offset = -(viewHeight - (y + totRowHeight));
            }
            if (offset < 0.0f) {
                offset = 0.0f;
            } else if (offset > m_ContentHeight - viewHeight) {
                offset = m_ContentHeight - viewHeight;
            }
            ScrollTo(0.0f, -offset);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::LayoutColumns()
{
    float x = 0.0f;

    float height = std::max(Height(), m_ContentHeight);

    for (size_t i = 0; i < m_ColumnMap.size(); ++i)
    {
        Ptr<ListViewColumnView> columnView = GetColumn(i);
        Rect frame(x, 0.0f, x + columnView->Width(), height);
        columnView->SetFrame(frame);
        if (i == m_ColumnMap.size() - 1) {
            x += columnView->m_ContentWidth;
        } else {
            x += frame.Width();
        }
    }
    m_TotalWidth = x;

    m_InertialScroller.SetScrollVBounds(Height() - m_ContentHeight, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListViewScrolledView::InsertColumn(const char* title, float width, size_t index) noexcept
{
    try
    {
        Ptr<ListViewColumnView> columnView = ptr_new<ListViewColumnView>(ptr_tmp_cast(this), title);
        columnView->SetFrame(Rect(0.0f, 0.0f, width, COORD_MAX));

        if (index == INVALID_INDEX) {
            index = m_ColumnViews.size();
            m_ColumnViews.push_back(columnView);
        } else {
            m_ColumnViews.insert(m_ColumnViews.begin() + index, columnView);
        }
        m_ColumnMap.push_back(index);
        LayoutColumns();
        return index;
    }
    catch (const std::bad_alloc& error)
    {
        set_last_error(ENOMEM);
        return INVALID_INDEX;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t ListViewScrolledView::InsertRow(size_t index, Ptr<ListViewRow> row, bool update)
{
    if (index != INVALID_INDEX)
    {
        if (index > m_Rows.size()) {
            index = m_Rows.size();
        }
        m_Rows.insert(m_Rows.begin() + index, row);
    }
    else
    {
        if (!m_ListView->HasFlags(ListViewFlags::NoAutoSort) && m_SortColumn != INVALID_INDEX)
        {
            std::vector<Ptr<ListViewRow>>::iterator iterator;

            iterator = std::lower_bound(m_Rows.begin(), m_Rows.end(), row, [sortColumn = m_SortColumn](Ptr<const ListViewRow> lhs, Ptr<const ListViewRow> rhs) { return lhs->IsLessThan(rhs, sortColumn); });
            iterator = m_Rows.insert(iterator, row);
            index = iterator - m_Rows.begin();
        }
        else
        {
            index = m_Rows.size();
            m_Rows.push_back(row);
        }
    }
    for (size_t i = 0; i < m_ColumnViews.size(); ++i) {
        row->AttachToView(m_ColumnViews[i], i);
    }
    for (size_t i = 0; i < m_ColumnViews.size(); ++i)
    {
        float width = row->GetWidth(m_ColumnViews[i], i);
        if (width > m_ColumnViews[i]->m_ContentWidth) {
            m_ColumnViews[i]->m_ContentWidth = width;
        }
    }
    row->m_Height = row->GetHeight(ptr_tmp_cast(this));

    m_ContentHeight += row->m_Height + m_VSpacing;

    for (size_t i = index; i < m_Rows.size(); ++i)
    {
        float y = 0.0f;
        if (i > 0) {
            y = m_Rows[i - 1]->m_YPos + m_Rows[i - 1]->m_Height + m_VSpacing;
        }
        m_Rows[i]->m_YPos = y;
    }
    LayoutColumns();
    if (update)
    {
        Rect bounds = GetBounds();

        if (row->m_YPos + row->m_Height + m_VSpacing <= bounds.bottom)
        {
            if (m_ListView->HasFlags(ListViewFlags::DontScroll))
            {
                bounds.top = row->m_YPos;
                for (size_t i = 0; i < m_ColumnViews.size(); ++i) {
                    m_ColumnViews[i]->Invalidate(bounds);
                }
            }
            else
            {
                bounds.top = row->m_YPos + row->m_Height + m_VSpacing;
                bounds.bottom += row->m_Height + m_VSpacing;
                for (size_t i = 0; i < m_ColumnViews.size(); ++i)
                {
                    m_ColumnViews[i]->CopyRect(bounds - Point(0.0f, row->m_Height + m_VSpacing), bounds.TopLeft());
                    m_ColumnViews[i]->Invalidate(Rect(bounds.left, row->m_YPos, bounds.right, bounds.top));
                }
            }
        }
    }
    return index;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<ListViewRow> ListViewScrolledView::RemoveRow(size_t index, bool update)
{
    Ptr<ListViewRow> row = m_Rows[index];
    m_Rows.erase(m_Rows.begin() + index);

    m_ContentHeight -= row->m_Height + m_VSpacing;

    for (size_t i = index; i < m_Rows.size(); ++i)
    {
        float y = 0.0f;
        if (i > 0) {
            y = m_Rows[i - 1]->m_YPos + m_Rows[i - 1]->m_Height + m_VSpacing;
        }
        m_Rows[i]->m_YPos = y;
    }

    for (size_t i = 0; i < m_ColumnViews.size(); ++i)
    {
        m_ColumnViews[i]->m_ContentWidth = 0.0f;
        for (size_t j = 0; j < m_Rows.size(); ++j)
        {
            const float width = m_Rows[j]->GetWidth(m_ColumnViews[i], i);
            if (width > m_ColumnViews[i]->m_ContentWidth) {
                m_ColumnViews[i]->m_ContentWidth = width;
            }
        }
    }

    LayoutColumns();
    if (update)
    {
        Rect bounds = GetBounds();

        const float rowHeight = row->m_Height + m_VSpacing;
        const float y = row->m_YPos;

        if (y + rowHeight <= bounds.bottom)
        {
            if (m_ListView->HasFlags(ListViewFlags::DontScroll))
            {
                bounds.top = y;
                for (size_t i = 0; i < m_ColumnViews.size(); ++i) {
                    m_ColumnViews[i]->Invalidate(bounds);
                }
            }
            else
            {
                bounds.top = y + rowHeight;
                bounds.bottom += rowHeight;
                for (size_t i = 0; i < m_ColumnViews.size(); ++i) {
                    m_ColumnViews[i]->CopyRect(bounds, bounds.TopLeft() - Point(0.0f, rowHeight));
                }
            }
        }
    }
    bool selectionChanged = row->m_Selected;
    if (m_FirstSel != INVALID_INDEX)
    {
        if (m_FirstSel == m_LastSel && index == m_FirstSel)
        {
            m_FirstSel = m_LastSel = -1;
            selectionChanged = true;
        }
        else
        {
            if (index < m_FirstSel)
            {
                m_FirstSel--;
                m_LastSel--;
                selectionChanged = true;
            }
            else if (index == m_FirstSel)
            {
                m_LastSel--;
                for (int i = index; i <= m_LastSel; ++i)
                {
                    if (m_Rows[i]->m_Selected)
                    {
                        m_FirstSel = i;
                        selectionChanged = true;
                        break;
                    }
                }
            }
            else if (index < m_LastSel)
            {
                m_LastSel--;
                selectionChanged = true;
            }
            else if (index == m_LastSel)
            {
                for (int i = m_LastSel - 1; i >= m_FirstSel; --i)
                {
                    if (m_Rows[i]->m_Selected)
                    {
                        m_LastSel = i;
                        selectionChanged = true;
                        break;
                    }
                }
            }
        }
    }
    if (m_BeginSel != -1 && m_BeginSel >= int(m_Rows.size())) {
        m_BeginSel = m_Rows.size() - 1;
    }
    if (m_EndSel != -1 && m_EndSel >= int(m_Rows.size())) {
        m_EndSel = m_Rows.size() - 1;
    }
    if (selectionChanged) {
        m_ListView->SelectionChanged(m_FirstSel, m_LastSel);
    }
    return row;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::Clear()
{
    if (m_IsSelecting)
    {
        SetDrawingMode(DrawingMode::Invert);
        DrawFrame(m_SelectRect, FRAME_TRANSPARENT | FRAME_THIN);
        SetDrawingMode(DrawingMode::Copy);
        m_IsSelecting = false;
        Flush();
    }
    m_MouseMoved = true;
    m_DragIfMoved = false;

    m_Rows.clear();
    m_ContentHeight = 0.0f;

    const Rect bounds = GetBounds();
    for (size_t i = 0; i < m_ColumnMap.size(); ++i) {
        GetColumn(i)->Invalidate(bounds);
    }
    for (size_t i = 0; i < m_ColumnViews.size(); ++i) {
        m_ColumnViews[i]->m_ContentWidth = 0.0f;
    }
    m_BeginSel         = INVALID_INDEX;
    m_EndSel           = INVALID_INDEX;
    m_FirstSel         = INVALID_INDEX;
    m_LastSel          = INVALID_INDEX;

    m_MouseDownTime    = TimeValMicros::zero;
    m_LastHitRow       = INVALID_INDEX;
    ScrollTo(0.0f, 0.0f);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::Paint(const Rect& updateRect)
{
    Rect frame = GetBounds();
    frame.top    = 0.0f;
    frame.bottom = m_ContentHeight;

    if (!m_ColumnMap.empty())
    {
        const Rect lastCol = GetColumn(m_ColumnMap.size() - 1)->GetFrame();
        frame.left = lastCol.right;
    }
    SetFgColor(255, 255, 255);
    FillRect(frame);

    if (m_InertialScroller.GetState() == InertialScroller::State::Dragging) {
        SetFgColor(StandardColorID::DefaultBackground);
    }
    frame = GetBounds();

    if (frame.top < 0.0f) {
        FillRect(Rect(frame.left, frame.top, frame.right, 0.0f));
    }
    if (frame.bottom > m_ContentHeight) {
        FillRect(Rect(frame.left, m_ContentHeight, frame.right, frame.bottom));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::InvalidateRow(size_t index, uint32_t flags, bool imidiate)
{
    const float totRowHeight = m_Rows[index]->m_Height + m_VSpacing;
    const float y = m_Rows[index]->m_YPos;
    const Rect frame(0.0f, y, COORD_MAX, y + totRowHeight);

    if (!GetBounds().DoIntersect(frame)) {
        return;
    }
    if (imidiate)
    {
        const bool hasFocus = false; // m_ListView->HasFocus();

        for (size_t i = 0; i < m_ColumnMap.size(); ++i)
        {
            Rect columnFrame(frame);
            columnFrame.right = GetColumn(i)->GetFrame().Width();
            m_Rows[index]->Paint(columnFrame, GetColumn(i), i, m_Rows[index]->m_Selected, m_Rows[index]->m_Highlighted, hasFocus);
        }
    }
    else
    {
        for (size_t i = 0; i < m_ColumnMap.size(); ++i)
        {
            Rect columnFrame(frame);
            columnFrame.right = GetColumn(i)->GetFrame().Width();
            GetColumn(i)->Invalidate(columnFrame);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::InvertRange(size_t start, size_t end)
{
    for (size_t i = start; i <= end; ++i)
    {
        if (m_Rows[i]->m_IsSelectable)
        {
            m_Rows[i]->m_Selected = !m_Rows[i]->m_Selected;
            InvalidateRow(i, ListView::INV_VISUAL);
        }
    }
    if (m_FirstSel == INVALID_INDEX)
    {
        m_FirstSel = start;
        m_LastSel = end;
    }
    else
    {
        if (end < m_FirstSel)
        {
            for (size_t i = start; i <= end; ++i)
            {
                if (m_Rows[i]->m_Selected) {
                    m_FirstSel = i;
                    break;
                }
            }
        }
        else if (start > m_LastSel)
        {
            for (size_t i = end; i >= start; --i)
            {
                if (m_Rows[i]->m_Selected)
                {
                    m_LastSel = i;
                    break;
                }
            }
        }
        else
        {
            if (start <= m_FirstSel)
            {
                m_FirstSel = m_LastSel + 1;
                for (size_t i = start; i <= m_LastSel; ++i)
                {
                    if (m_Rows[i]->m_Selected) {
                        m_FirstSel = i;
                        break;
                    }
                }
            }
            if (end >= m_LastSel)
            {
                m_LastSel = m_FirstSel - 1;
                if (m_FirstSel < m_Rows.size())
                {
                    for (int i = end; i >= int(m_FirstSel); --i)
                    {
                        if (m_Rows[i]->m_Selected) {
                            m_LastSel = i;
                            break;
                        }
                    }
                }
            }
        }
    }
    if (m_LastSel < m_FirstSel)
    {
        m_FirstSel = INVALID_INDEX;
        m_LastSel = INVALID_INDEX;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::SelectRange(size_t start, size_t end, bool select)
{
    bool changed = false;

    for (size_t i = start; i <= end; ++i)
    {
        if (m_Rows[i]->m_IsSelectable && m_Rows[i]->m_Selected != select)
        {
            m_Rows[i]->m_Selected = select;
            InvalidateRow(i, ListView::INV_VISUAL);
            changed = true;
        }
    }
    if (changed)
    {
        if (m_FirstSel == INVALID_INDEX)
        {
            if (select)
            {
                m_FirstSel = start;
                m_LastSel  = end;
            }
        }
        else
        {
            if (select)
            {
                if (start < m_FirstSel) {
                    m_FirstSel = start;
                }
                if (end > m_LastSel) {
                    m_LastSel = end;
                }
            }
            else
            {
                if (end >= m_FirstSel)
                {
                    size_t i = m_FirstSel;
                    m_FirstSel = m_LastSel + 1;
                    for (; i <= m_LastSel; ++i)
                    {
                        if (m_Rows[i]->m_Selected) {
                            m_FirstSel = i;
                            break;
                        }
                    }
                }
                if (start <= m_LastSel && m_LastSel < m_Rows.size())
                {
                    int i = m_LastSel;
                    m_LastSel = m_FirstSel - 1;
                    for (; i >= int(m_FirstSel) - 1; --i)
                    {
                        if (m_Rows[i]->m_Selected) {
                            m_LastSel = i;
                            break;
                        }
                    }
                }
            }
            if (m_LastSel < m_FirstSel)
            {
                m_FirstSel = INVALID_INDEX;
                m_LastSel  = INVALID_INDEX;
            }
        }
    }
    return changed;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewScrolledView::ExpandSelect(size_t row, bool invert, bool clear)
{
    if (m_Rows.empty()) {
        return;
    }
    if (row == INVALID_INDEX) {
        return;
    }
    if (row >= m_Rows.size()) {
        row = m_Rows.size() - 1;
    }

    if (clear)
    {
        if (m_FirstSel != INVALID_INDEX)
        {
            for (size_t i = m_FirstSel; i <= m_LastSel; ++i)
            {
                if (m_Rows[i]->m_Selected)
                {
                    m_Rows[i]->m_Selected = false;
                    InvalidateRow(i, ListView::INV_VISUAL, true);
                }
            }
        }
        m_FirstSel = INVALID_INDEX;
        m_LastSel  = INVALID_INDEX;
        if (m_BeginSel == INVALID_INDEX) {
            m_BeginSel = row;
        }
        m_EndSel = row;
        bool changed;
        if (m_BeginSel < row) {
            changed = SelectRange(m_BeginSel, row, true);
        } else {
            changed = SelectRange(row, m_BeginSel, true);
        }
        if (changed) {
            m_ListView->SelectionChanged(m_FirstSel, m_LastSel);
        }
        return;
    }
    if (m_BeginSel == INVALID_INDEX || m_EndSel == INVALID_INDEX)
    {
        m_BeginSel = row;
        m_EndSel = row;
        bool selectionChanged;
        if (invert) {
            InvertRange(row, row);
            selectionChanged = true;
        } else {
            selectionChanged = SelectRange(row, row, true);
        }
        if (selectionChanged) {
            m_ListView->SelectionChanged(m_FirstSel, m_LastSel);
        }
        return;
    }
    if (row == m_EndSel)
    {
        bool selectionChanged;
        if (invert) {
            InvertRange(row, row);
            selectionChanged = true;
        } else {
            selectionChanged = SelectRange(row, row, true);
        }
        if (selectionChanged) {
            m_ListView->SelectionChanged(m_FirstSel, m_LastSel);
        }
        m_EndSel = row;
        return;
    }
    bool selectionChanged = false;
    if (m_BeginSel <= m_EndSel)
    {
        if (row < m_EndSel)
        {
            if (invert)
            {
                if (row < m_BeginSel) {
                    InvertRange(m_BeginSel + 1, m_EndSel);
                    InvertRange(row, m_BeginSel - 1);
                } else {
                    InvertRange(row + 1, m_EndSel);
                }
                selectionChanged = true;
            }
            else
            {
                if (row < m_BeginSel) {
                    selectionChanged = SelectRange(m_BeginSel + 1, m_EndSel, false);
                    selectionChanged = SelectRange(row, m_BeginSel - 1, true) || selectionChanged;
                } else {
                    selectionChanged = SelectRange(row + 1, m_EndSel, false);
                }
            }
        }
        else
        {
            if (invert) {
                InvertRange(m_EndSel + 1, row);
                selectionChanged = true;
            } else {
                selectionChanged = SelectRange(m_EndSel + 1, row, true);
            }
        }
    }
    else
    {
        if (row < m_EndSel)
        {
            if (invert) {
                InvertRange(row, m_EndSel - 1);
                selectionChanged = true;
            } else {
                selectionChanged = SelectRange(row, m_EndSel - 1, true);
            }
        }
        else
        {
            if (invert)
            {
                if (row > m_BeginSel) {
                    InvertRange(m_EndSel, m_BeginSel - 1);
                    InvertRange(m_BeginSel + 1, row);
                } else {
                    InvertRange(m_EndSel, row - 1);
                }
                selectionChanged = true;
            }
            else
            {
                if (row > m_BeginSel) {
                    selectionChanged = SelectRange(m_EndSel, m_BeginSel - 1, false);
                    selectionChanged = SelectRange(m_BeginSel + 1, row, true) || selectionChanged;
                } else {
                    selectionChanged = SelectRange(m_EndSel, row - 1, false);
                }
            }
        }
    }
    if (selectionChanged) {
        m_ListView->SelectionChanged(m_FirstSel, m_LastSel);
    }
    m_EndSel = row;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewScrolledView::ClearSelection()
{
    bool changed = false;

    if (m_FirstSel != INVALID_INDEX)
    {
        for (size_t i = m_FirstSel; i <= m_LastSel; ++i)
        {
            if (m_Rows[i]->m_Selected)
            {
                changed = true;
                m_Rows[i]->m_Selected = false;
                InvalidateRow(i, ListView::INV_VISUAL, true);
            }
        }
    }
    m_FirstSel = INVALID_INDEX;
    m_LastSel = INVALID_INDEX;
    return changed;
}
