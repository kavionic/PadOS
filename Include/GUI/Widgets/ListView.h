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

#include <GUI/Widgets/Control.h>

#include <vector>
#include <string>
#include <functional>

class PScrollBar;

class PListViewHeaderView;
class PListViewScrolledView;
class PListViewRow;

namespace PListViewFlags
{
static constexpr uint32_t MultiSelect   = 0x0001 << PViewFlags::FirstUserBit;
static constexpr uint32_t NoAutoSort    = 0x0002 << PViewFlags::FirstUserBit;
static constexpr uint32_t RenderBorder  = 0x0004 << PViewFlags::FirstUserBit;
static constexpr uint32_t DontScroll    = 0x0008 << PViewFlags::FirstUserBit;
static constexpr uint32_t NoHeader      = 0x0010 << PViewFlags::FirstUserBit;
static constexpr uint32_t NoColumnRemap = 0x0020 << PViewFlags::FirstUserBit;

extern const std::map<PString, uint32_t> FlagMap;
}


///////////////////////////////////////////////////////////////////////////////
/// Flexible multicolumn list view
/// \ingroup gui
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PListView : public PControl
{
public:
    static constexpr float AUTOSCROLL_BORDER = 20.0f;

    enum {
        INV_HEIGHT = 0x01,
        INV_WIDTH = 0x02,
        INV_VISUAL = 0x04
    };

    using const_iterator = std::vector<Ptr<PListViewRow>>::const_iterator;
    using ColumnMap = std::vector<size_t>;

    PListView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = PListViewFlags::MultiSelect | PListViewFlags::RenderBorder);
    PListView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

    ~PListView();

    virtual void    SelectionChanged(size_t firstRow, size_t lastRow);
    virtual bool    DragSelection(const PPoint& pos);

    void            StartScroll(PScrollDirection direction, bool select);
    void            StopScroll();

    bool            IsMultiSelect() const;
    void            SetMultiSelect(bool multi);

    bool            IsAutoSort() const;
    void            SetAutoSort(bool autoSort);

    bool            HasBorder() const;
    void            SetRenderBorder(bool render);

    bool            HasColumnHeader() const;
    void            SetHasColumnHeader(bool value);

    void            MakeVisible(size_t row, bool center = true);
    size_t          InsertColumn(const char* title, float width, size_t index = INVALID_INDEX);

    const ColumnMap&    GetColumnMapping() const;
    void                SetColumnMapping(const ColumnMap& map);

    void                InsertRow(size_t index, Ptr<PListViewRow> row, bool update = true);
    void                InsertRow(Ptr<PListViewRow> row, bool update = true);
    Ptr<PListViewRow>    RemoveRow(size_t index, bool update = true);
    void                InvalidateRow(size_t row, uint32_t flags);
    size_t              GetRowCount() const;
    Ptr<PListViewRow>    GetRow(const PPoint& pos) const;
    Ptr<PListViewRow>    GetRow(size_t index) const;
    size_t              GetRowIndex(Ptr<PListViewRow> row) const;
    size_t              HitTest(const PPoint& pos) const;
    float               GetRowPos(size_t row);
    void                Clear();
    bool                IsSelected(size_t row) const;
    void                Select(size_t first, size_t last, bool replace = true, bool select = true);
    void                Select(size_t row, bool replace = true, bool select = true);
    void                ClearSelection();

    void                Highlight(size_t first, size_t last, bool replace = true, bool highlight = true);
    void                Highlight(size_t row, bool replace, bool highlight = true);

    void                Sort();
    size_t              GetFirstSelected() const;
    size_t              GetLastSelected() const;
    virtual void        OnPaint(const PRect& updateRect) override;
    virtual void        OnFrameSized(const PPoint& delta) override;
    //    virtual void      KeyDown( const char* pzString, const char* pzRawString, uint32_t nQualifiers );
    //    virtual void      AllAttachedToScreen() override;
    virtual bool        HasFocus(PMouseButton button) const override;

    // STL iterator interface to the rows.
    const_iterator begin() const;
    const_iterator end() const;

    Signal<void, size_t, size_t, Ptr<PListView>> SignalSelectionChanged;//(size_t firstRow, size_t lastRow, Ptr<ListView> source)
    Signal<void, size_t, size_t, Ptr<PListView>> SignalItemClicked;

private:
    friend class PListViewScrolledView;
    friend class PListViewHeaderView;
    void    Construct();
    void    Layout();
    void    AdjustScrollBars(bool okToHScroll = true);

    Ptr<PListViewScrolledView>   m_ScrolledContainerView;
    Ptr<PListViewHeaderView>     m_HeaderView;
    Ptr<PScrollBar>              m_VScrollBar;
    Ptr<PScrollBar>              m_HScrollBar;
};
