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

#include "ListViewHeaderView.h"
#include "ListViewScrolledView.h"
#include "ListViewColumnView.h"
#include "GUI/ListView.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ListViewHeaderView::ListViewHeaderView(Ptr<ListView> parent) : View("header_view", parent, ViewFlags::WillDraw)
{
    m_SizeColumn = -1;
    m_DragColumn = -1;
    m_ScrolledContainerView = ptr_new<ListViewScrolledView>(parent);
    AddChild(m_ScrolledContainerView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewHeaderView::HasFocus(MouseButton_e button) const
{
    if (View::HasFocus(button)) {
        return true;
    }
    return m_ScrolledContainerView->HasFocus(button);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewHeaderView::DrawButton(const char* title, const Rect& frame, Ptr<Font> font, FontHeight* fontHeight)
{
    SetEraseColor(StandardColorID::LISTVIEW_TAB);
    DrawFrame(frame, FRAME_RAISED);

    SetFgColor(StandardColorID::LISTVIEW_TAB_TEXT);
    SetBgColor(StandardColorID::LISTVIEW_TAB);

    float vFontHeight = fontHeight->ascender + fontHeight->descender;

    int nStrLen = font->GetStringLength(title, frame.Width() - 9.0f);
    DrawString(String(title, title + nStrLen), frame.TopLeft() + Point(5, (frame.Height() + 1.0f) / 2 - vFontHeight / 2 + fontHeight->ascender));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewHeaderView::Paint(const Rect& cUpdateRect)
{
    Ptr<Font> pcFont = GetFont();
    if (pcFont == nullptr) {
        return;
    }

    FontHeight sHeight = pcFont->GetHeight();

    Rect cFrame;
    for (uint i = 0; i < m_ScrolledContainerView->m_ColumnMap.size(); ++i)
    {
        Ptr<ListViewColumnView> pcCol = m_ScrolledContainerView->GetColumn(i);
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

bool ListViewHeaderView::OnMouseMove(MouseButton_e button, const Point& position)
{
    Ptr<ListView> listView = ptr_static_cast<ListView>(GetParent());
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

    Ptr<ListViewColumnView> columnView = m_ScrolledContainerView->m_ColumnViews[m_ScrolledContainerView->m_ColumnMap[m_ScrolledContainerView->m_ColumnMap.size() - 1]];
    Rect frame = columnView->GetFrame();
    frame.right = listView->GetBounds().right - GetScrollOffset().x;
    columnView->SetFrame(frame);


    m_ScrolledContainerView->LayoutColumns();
    listView->AdjustScrollBars(false);
    Paint(GetBounds());
    Flush();
    m_HitPos = position;

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ListViewHeaderView::ViewScrolled(const Point& delta)
{
    if (m_ScrolledContainerView->m_ColumnMap.empty()) {
        return;
    }
    Ptr<ListViewColumnView> columnView = m_ScrolledContainerView->m_ColumnViews[m_ScrolledContainerView->m_ColumnMap[m_ScrolledContainerView->m_ColumnMap.size() - 1]];
    Rect frame = columnView->GetFrame();
    frame.right = GetParent()->GetBounds().right - GetScrollOffset().x;
    columnView->SetFrame(frame);

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool ListViewHeaderView::OnMouseDown(MouseButton_e button, const Point& position)
{
    if (m_ScrolledContainerView->m_ColumnMap.empty()) {
        return false;
    }
    if (position.y >= m_ScrolledContainerView->GetFrame().top) {
        return false;
    }
    Point containerViewPos = m_ScrolledContainerView->ConvertFromParent(position);
    for (size_t i = 0; i < m_ScrolledContainerView->m_ColumnMap.size(); ++i)
    {
        Rect frame(m_ScrolledContainerView->GetColumn(i)->GetFrame());

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

bool ListViewHeaderView::OnMouseUp(MouseButton_e button, const Point& position)
{
    m_SizeColumn = INVALID_INDEX;

    if (m_DragColumn != INVALID_INDEX && m_DragColumn < m_ScrolledContainerView->m_ColumnMap.size())
    {
        m_ScrolledContainerView->m_SortColumn = m_ScrolledContainerView->m_ColumnMap[m_DragColumn];

        if (!m_ScrolledContainerView->m_ListView->HasFlags(ListViewFlags::NoAutoSort))
        {
            Ptr<ListView> listView = ptr_static_cast<ListView>(GetParent());
            assert(listView != nullptr);

            listView->Sort();
            Rect bounds = m_ScrolledContainerView->GetBounds();
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
    //          for (uint i = 0; i < m_pcMainView->m_ColumnMap.size(); ++i)
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

void ListViewHeaderView::FrameSized(const Point& deltaSize)
{
    Rect bounds = GetBounds();
    bool needFlush = false;

    if (deltaSize.x != 0.0f)
    {
        Rect damage = bounds;

        damage.left = damage.right - std::max(1.0f, deltaSize.x);
        Invalidate(damage);
        needFlush = true;
    }
    if (deltaSize.y != 0.0f)
    {
        Rect damage = bounds;

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

void ListViewHeaderView::Layout()
{
    FontHeight fontHeight = GetFontHeight();

    if (m_ScrolledContainerView->m_ListView->HasFlags(ListViewFlags::NoHeader)) {
        m_HeaderHeight = 0.0f;
    } else {
        m_HeaderHeight = fontHeight.ascender + fontHeight.descender + 6.0f;
    }

    Rect frame = GetFrame().Bounds();
    frame.Floor();
    frame.top += m_HeaderHeight;
    frame.right = COORD_MAX;
    m_ScrolledContainerView->SetFrame(frame);
}
