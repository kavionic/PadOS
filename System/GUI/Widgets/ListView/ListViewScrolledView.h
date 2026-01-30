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

#pragma once

#include "GUI/View.h"
#include "Threads/EventTimer.h"
#include "Utils/InertialScroller.h"

class PListView;
class PListViewRow;
class PListViewColumnView;

class PListViewScrolledView : public PView
{
public:
    friend class PListView;
    friend class PListViewHeaderView;
    friend class PListViewRow;
    friend class PListViewColumnView;

    enum { AUTOSCROLL_TIMER = 1 };

    PListViewScrolledView(Ptr<PListView> listView);

private:
    size_t              InsertColumn(const char* title, float width, size_t index = INVALID_INDEX) noexcept;
    size_t              InsertRow(size_t index, Ptr<PListViewRow> row, bool update);
    Ptr<PListViewRow>    RemoveRow(size_t index, bool update);
    void                InvalidateRow(size_t index, uint32_t flags, bool imidiate = false);
    void                Clear();
    size_t              GetRowIndex(float y) const;
    size_t              GetRowIndex(Ptr<const PListViewRow> row) const;
    void                MakeVisible(size_t row, bool center);
    void                StartScroll(PScrollDirection direction, bool select);
    void                StopScroll();

    virtual void    OnFrameSized(const PPoint& delta) override;

    virtual bool    OnTouchDown(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchUp(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnTouchMove(PMouseButton pointID, const PPoint& position, const PMotionEvent& event) override;

    virtual bool    OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    //    virtual void  WheelMoved(const Point& cDelta);
    virtual void    OnPaint(const PRect& updateRect) override;
    //    virtual void  TimerTick( int nID );
    virtual void    DetachedFromWindow();
    virtual bool    HasFocus(PMouseButton button) const override;

    //    bool      HandleKey( char nChar, uint32_t nQualifiers );
    void        LayoutColumns();
    Ptr<PListViewColumnView>    GetColumn(int columnIndex) const { return(m_ColumnViews[m_ColumnMap[columnIndex]]); }


    bool    ClearSelection();
    bool    SelectRange(size_t start, size_t end, bool select);
    void    InvertRange(size_t start, size_t end);
    void    ExpandSelect(size_t row, bool invert, bool clear);

    void SlotInertialScrollUpdate(const PPoint& position);

    std::vector<Ptr<PListViewColumnView>>    m_ColumnViews;
    std::vector<size_t>                     m_ColumnMap;
    std::vector<Ptr<PListViewRow>>           m_Rows;
    PListView*                               m_ListView;

    PMouseButton                           m_HitButton = PMouseButton::None;
    PPoint                                   m_HitPos;
    PInertialScroller                        m_InertialScroller;
    PRect                                    m_SelectRect;
    bool                                    m_IsSelecting       = false;
    bool                                    m_MouseMoved        = false;
    bool                                    m_DragIfMoved       = false;
    size_t                                  m_BeginSel          = INVALID_INDEX;
    size_t                                  m_EndSel            = INVALID_INDEX;
    TimeValNanos                            m_MouseDownTime;
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
