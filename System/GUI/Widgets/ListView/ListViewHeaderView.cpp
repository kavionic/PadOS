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

#include "ListViewHeaderView.h"
#include "ListViewScrolledView.h"
#include "ListViewColumnView.h"
#include <GUI/Widgets/ListView.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PListViewHeaderView::PListViewHeaderView(Ptr<PListView> parent) : PView("header_view", parent, PViewFlags::WillDraw)
{
    m_SizeColumn = -1;
    m_DragColumn = -1;
    m_ScrolledContainerView = ptr_new<PListViewScrolledView>(parent);
    AddChild(m_ScrolledContainerView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PListViewHeaderView::HasFocus(PMouseButton button) const
{
    if (PView::HasFocus(button)) {
        return true;
    }
    return m_ScrolledContainerView->HasFocus(button);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewHeaderView::DrawButton(const char* title, const PRect& frame, Ptr<PFont> font, PFontHeight* fontHeight)
{
    SetEraseColor(PStandardColorID::ListViewTab);
    DrawFrame(frame, FRAME_RAISED);

    SetFgColor(PStandardColorID::ListViewTabText);
    SetBgColor(PStandardColorID::ListViewTab);

    float vFontHeight = fontHeight->ascender + fontHeight->descender;

    int nStrLen = font->GetStringLength(title, frame.Width() - 9.0f);
    DrawString(PString(title, title + nStrLen), frame.TopLeft() + PPoint(5, (frame.Height() + 1.0f) / 2 - vFontHeight / 2 + fontHeight->ascender));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewHeaderView::OnPaint(const PRect& cUpdateRect)
{
    Ptr<PFont> pcFont = GetFont();
    if (pcFont == nullptr) {
        return;
    }

    PFontHeight sHeight = pcFont->GetHeight();

    PRect cFrame;
    for (size_t i = 0; i < m_ScrolledContainerView->m_ColumnMap.size(); ++i)
    {
        Ptr<PListViewColumnView> pcCol = m_ScrolledContainerView->GetColumn(i);
        cFrame = pcCol->GetFrame();
        cFrame.top = 0;
        cFrame.bottom = sHeight.ascender + sHeight.descender + 6 - 1;
        if (i == m_ScrolledContainerView->m_ColumnMap.size() - 1) {
            cFrame.right = COORD_MAX;
        }
        DrawButton(pcCol->m_Title.c_str(), cFrame, pcFont, &sHeight);
    }/*
    cFrame.left = cFrame.right + 1;
    cFrame.right = GetBounds().right;
    if ( cFrame.IsValid() ) {
    DrawButton( "", cFrame, pcFont, &sHeight );
    }*/
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PListViewHeaderView::OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    Ptr<PListView> listView = ptr_static_cast<PListView>(GetParent());
    assert(listView != nullptr);

    if (m_DragColumn != -1)
    {
        if (m_DragColumn < int(m_ScrolledContainerView->m_ColumnMap.size()))
        {
            //          Message cData(0);
            //          cData.AddInt32("column", m_pcMainView->m_ColumnMap[m_DragColumn]);
            //          Rect frame(m_pcMainView->GetColumn(m_DragColumn)->GetFrame());
            //          //      if ( m_DragColumn == int(m_pcMainView->m_ColumnMap.size()) - 1 ) {
            //          //      frame.right = frame.left + m_pcMainView->GetColumn( m_DragColumn )->m_vContentWidth;
            //          //      }
            //          m_pcMainView->ConvertToParent(&frame);
            //          ConvertFromParent(&frame);
            //          frame.top = 0.0f;
            //          frame.bottom = m_HeaderHeight - 1.0f;
            //
            //          BeginDrag(&cData, position - frame.LeftTop() - GetScrollOffset(), frame.Bounds(), this);
        }
        m_DragColumn = -1;
    }
    if (m_SizeColumn == -1) {
        return true;
    }
    float columnWidth = m_ScrolledContainerView->GetColumn(m_SizeColumn)->GetFrame().Width();
    float deltaSize = position.x - m_HitPos.x;
    if (columnWidth + deltaSize < 4.0f) {
        deltaSize = 4.0f - columnWidth;
    }
    m_ScrolledContainerView->GetColumn(m_SizeColumn)->ResizeBy(deltaSize, 0.0f);

    Ptr<PListViewColumnView> columnView = m_ScrolledContainerView->m_ColumnViews[m_ScrolledContainerView->m_ColumnMap[m_ScrolledContainerView->m_ColumnMap.size() - 1]];
    PRect frame = columnView->GetFrame();
    frame.right = listView->GetBounds().right - GetScrollOffset().x;
    columnView->SetFrame(frame);


    m_ScrolledContainerView->LayoutColumns();
    listView->AdjustScrollBars(false);
    OnPaint(GetBounds());
    Flush();
    m_HitPos = position;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewHeaderView::OnViewScrolled(const PPoint& delta)
{
    if (m_ScrolledContainerView->m_ColumnMap.empty()) {
        return;
    }
    Ptr<PListViewColumnView> columnView = m_ScrolledContainerView->m_ColumnViews[m_ScrolledContainerView->m_ColumnMap[m_ScrolledContainerView->m_ColumnMap.size() - 1]];
    PRect frame = columnView->GetFrame();
    frame.right = GetParent()->GetBounds().right - GetScrollOffset().x;
    columnView->SetFrame(frame);

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PListViewHeaderView::OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    if (m_ScrolledContainerView->m_ColumnMap.empty()) {
        return false;
    }
    if (position.y >= m_ScrolledContainerView->GetFrame().top) {
        return false;
    }
    PPoint containerViewPos = m_ScrolledContainerView->ConvertFromParent(position);
    for (size_t i = 0; i < m_ScrolledContainerView->m_ColumnMap.size(); ++i)
    {
        PRect frame(m_ScrolledContainerView->GetColumn(i)->GetFrame());

        if (containerViewPos.x >= frame.left && containerViewPos.x <= frame.right)
        {
            if (i > 0 && containerViewPos.x >= frame.left && containerViewPos.x <= frame.left + 5.0f)
            {
                m_SizeColumn = i - 1;
                m_HitPos = position;
                MakeFocus(button, true);
                break;
            }
            if (containerViewPos.x >= frame.right - 6.0f && containerViewPos.x < frame.right)
            {
                m_SizeColumn = i;
                m_HitPos = position;
                MakeFocus(button, true);
                break;
            }
            m_DragColumn = i;
            break;
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PListViewHeaderView::OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event)
{
    m_SizeColumn = INVALID_INDEX;

    if (m_DragColumn != INVALID_INDEX && m_DragColumn < m_ScrolledContainerView->m_ColumnMap.size())
    {
        m_ScrolledContainerView->m_SortColumn = m_ScrolledContainerView->m_ColumnMap[m_DragColumn];

        if (!m_ScrolledContainerView->m_ListView->HasFlags(PListViewFlags::NoAutoSort))
        {
            Ptr<PListView> listView = ptr_static_cast<PListView>(GetParent());
            assert(listView != nullptr);

            listView->Sort();
            PRect bounds = m_ScrolledContainerView->GetBounds();
            for (size_t j = 0; j < m_ScrolledContainerView->m_ColumnMap.size(); ++j)
            {
                if (m_ScrolledContainerView->GetColumn(j) != nullptr) {
                    m_ScrolledContainerView->GetColumn(j)->Invalidate(bounds);
                }
            }
            Flush();
        }
    }
    m_DragColumn = INVALID_INDEX;
    //  if (pcData != NULL)
    //  {
    //      int column;
    //      if (pcData->FindInt("column", &column) == 0) {
    //          Point cMVPos = m_pcMainView->ConvertFromParent(position);
    //          for (size_t i = 0; i < m_pcMainView->m_ColumnMap.size(); ++i)
    //          {
    //              Rect cFrame(m_pcMainView->GetColumn(i)->GetFrame());
    //
    //              if (cMVPos.x >= cFrame.left && cMVPos.x <= cFrame.right) {
    //                  int j;
    //                  if (cMVPos.x < cFrame.left + cFrame.Width() * 0.5f) {
    //                      j = i;
    //                  } else {
    //                      j = i + 1;
    //                  }
    //                  m_ScrolledContainerView->m_ColumnMap.insert(m_ScrolledContainerView->m_ColumnMap.begin() + j, column);
    //                  for (int k = 0; k < int(m_ScrolledContainerView->m_ColumnMap.size()); ++k) {
    //                      if (k != j && m_ScrolledContainerView->m_ColumnMap[k] == column) {
    //                          m_ScrolledContainerView->m_ColumnMap.erase(m_ScrolledContainerView->m_ColumnMap.begin() + k);
    //                          break;
    //                      }
    //                  }
    //                  m_ScrolledContainerView->LayoutColumns();
    //                  Paint(GetBounds());
    //                  Flush();
    //              }
    //          }
    //      } else {
    //          View::MouseUp(position, nButton, pcData);
    //      }
    //      return;
    //  }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewHeaderView::OnFrameSized(const PPoint& deltaSize)
{
    PRect bounds = GetBounds();
    bool needFlush = false;

    if (deltaSize.x != 0.0f)
    {
        PRect damage = bounds;

        damage.left = damage.right - std::max(1.0f, deltaSize.x);
        Invalidate(damage);
        needFlush = true;
    }
    if (deltaSize.y != 0.0f)
    {
        PRect damage = bounds;

        damage.top = damage.bottom - std::max(1.0f, deltaSize.y);
        Invalidate(damage);
        needFlush = true;
    }

    Layout();

    if (needFlush) {
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PListViewHeaderView::Layout()
{
    PFontHeight fontHeight = GetFontHeight();

    if (m_ScrolledContainerView->m_ListView->HasFlags(PListViewFlags::NoHeader)) {
        m_HeaderHeight = 0.0f;
    } else {
        m_HeaderHeight = fontHeight.ascender + fontHeight.descender + 6.0f;
    }

    PRect frame = GetFrame().Bounds();
    frame.Floor();
    frame.top += m_HeaderHeight;
    frame.right = COORD_MAX;
    m_ScrolledContainerView->SetFrame(frame);
}
