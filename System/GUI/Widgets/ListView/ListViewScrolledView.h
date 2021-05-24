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

#pragma once

#include "GUI/View.h"
#include "Threads/EventTimer.h"
#include "Utils/InertialScroller.h"

namespace os
{
class ListViewRow;

class ListView;
class ListViewColumnView;

class ListViewScrolledView : public View
{
public:
    friend class ListView;
    friend class ListViewHeaderView;
    friend class ListViewRow;
    friend class ListViewColumnView;

    enum { AUTOSCROLL_TIMER = 1 };

    ListViewScrolledView(Ptr<ListView> listView);

private:
    size_t              InsertColumn(const char* title, float width, size_t index = INVALID_INDEX) noexcept;
    size_t              InsertRow(size_t index, Ptr<ListViewRow> row, bool update);
    Ptr<ListViewRow>    RemoveRow(size_t index, bool update);
    void                InvalidateRow(size_t index, uint32_t flags, bool imidiate = false);
    void                Clear();
    size_t              GetRowIndex(float y) const;
    size_t              GetRowIndex(Ptr<const ListViewRow> row) const;
    void                MakeVisible(size_t row, bool center);
    void                StartScroll(ScrollDirection direction, bool select);
    void                StopScroll();

    virtual void    FrameSized(const Point& delta) override;

    virtual bool    OnTouchDown(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchUp(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;
    virtual bool    OnTouchMove(MouseButton_e pointID, const Point& position, const MotionEvent& event) override;

    virtual bool    OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    //    virtual void  WheelMoved(const Point& cDelta);
    virtual void    Paint(const Rect& updateRect) override;
    //    virtual void  TimerTick( int nID );
    virtual void    DetachedFromWindow();
    virtual bool    HasFocus(MouseButton_e button) const override;

    //    bool      HandleKey( char nChar, uint32_t nQualifiers );
    void        LayoutColumns();
    Ptr<ListViewColumnView>    GetColumn(int columnIndex) const { return(m_ColumnViews[m_ColumnMap[columnIndex]]); }


    bool    ClearSelection();
    bool    SelectRange(size_t start, size_t end, bool select);
    void    InvertRange(size_t start, size_t end);
    void    ExpandSelect(size_t row, bool invert, bool clear);

    void SlotInertialScrollUpdate(const Point& position);

    std::vector<Ptr<ListViewColumnView>>    m_ColumnViews;
    std::vector<size_t>                     m_ColumnMap;
    std::vector<Ptr<ListViewRow>>           m_Rows;
    ListView*                               m_ListView;

    MouseButton_e                           m_HitButton = MouseButton_e::None;
    Point                                   m_HitPos;
    InertialScroller                        m_InertialScroller;
    Rect                                    m_SelectRect;
    bool                                    m_IsSelecting       = false;
    bool                                    m_MouseMoved        = false;
    bool                                    m_DragIfMoved       = false;
    size_t                                  m_BeginSel          = INVALID_INDEX;
    size_t                                  m_EndSel            = INVALID_INDEX;
    TimeValMicros                           m_MouseDownTime;
    size_t                                  m_LastHitRow        = INVALID_INDEX;
    size_t                                  m_FirstSel          = INVALID_INDEX;
    size_t                                  m_LastSel           = INVALID_INDEX;
    float                                   m_VSpacing          = 3.0f;
    float                                   m_TotalWidth        = 0.0f; // Total with of columns
    float                                   m_ContentHeight     = 0.0f;
    size_t                                  m_SortColumn        = 0;
    bool                                    m_AutoScrollUp      = false;
    bool                                    m_AutoScrollDown    = false;
    bool                                    m_MousDownSeen      = false;
    bool                                    m_AutoScrollSelects = false;
};

} // namespace os
