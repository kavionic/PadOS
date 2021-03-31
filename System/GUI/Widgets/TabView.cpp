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


#include <stdio.h>

#include <GUI/Widgets/Tabview.h>


using namespace os;

static constexpr float CORNER_SIZE = 10.0f;
static constexpr float ARROW_SPACE = 16.0f;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

TabView::TabView(const String& name, Ptr<View> parent, uint32_t flags) : View(name, parent, flags | ViewFlags::WillDraw)
{
    m_FontHeight    = GetFontHeight();
    m_GlyphHeight   = m_FontHeight.ascender + m_FontHeight.descender + m_FontHeight.line_gap;
    m_TabHeight     = round(m_GlyphHeight * 1.2f + 6.0f);
    m_TotalWidth    = 4.0f;

    m_TopView = ptr_new<TopView>(this);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TabView::FrameSized(const Point& delta)
{
    const Rect bounds = GetBounds();
    bool needFlush = false;

    if (m_SelectedTab != INVALID_INDEX && m_SelectedTab < m_TabList.size())
    {
        Ptr<View> view = m_TabList[m_SelectedTab].m_View;
        if (view != nullptr)
        {
            Rect clientBounds = bounds;
            clientBounds.Resize(2.0f, m_TabHeight, -2.0f, -2.0f);
            view->SetFrame(clientBounds);
        }
    }
    if (delta.x != 0.0f)
    {
        Rect damage = bounds;

        damage.left = damage.right - std::max(3.0f, delta.x + 9.0f);
        Invalidate(damage);
        needFlush = true;
    }
    if (delta.y != 0.0f)
    {
        Rect damage = bounds;

        damage.top = damage.bottom - std::max(3.0f, delta.y + 2.0f);
        Invalidate(damage);
        needFlush = true;
    }

    float width = bounds.Width();

    const float oldOffset = m_ScrollOffset;
    if (width < m_TotalWidth)
    {
        m_TopView->SetFrame(Rect(bounds.left + ARROW_SPACE, bounds.top, bounds.right - ARROW_SPACE, m_TabHeight - 2.0f));
        width -= ARROW_SPACE * 2.0f;
        if (m_ScrollOffset > 0.0f) {
            m_ScrollOffset = 0.0f;
        }
        if (m_TotalWidth + m_ScrollOffset < width) {
            m_ScrollOffset = width - m_TotalWidth;
        }
        if (width + ARROW_SPACE * 2.0f - delta.x >= m_TotalWidth)
        {
            m_TopView->Invalidate();
            Rect damage(bounds);
            damage.bottom = damage.top + m_TabHeight;
            Invalidate(damage);
            needFlush = true;
        }
    }
    else
    {
        m_TopView->SetFrame(Rect(bounds.left + 2.0f, bounds.top, bounds.right - 2.0f, m_TabHeight - 2.0f));
        if (m_ScrollOffset > width - m_TotalWidth) {
            m_ScrollOffset = width - m_TotalWidth;
        }
        if (m_ScrollOffset < 0.0f) {
            m_ScrollOffset = 0.0f;
        }
        if (width - delta.x < m_TotalWidth)
        {
            m_TopView->Invalidate();
            Rect damage(bounds);
            damage.bottom = damage.top + m_TabHeight;
            Invalidate(damage);
            needFlush = true;
        }
    }
    if (m_ScrollOffset != oldOffset)
    {
        m_TopView->ScrollBy(m_ScrollOffset - oldOffset, 0.0f);
        m_TopView->Invalidate();
        Rect damage(bounds);
        damage.bottom = damage.top + m_TabHeight;
        Invalidate(damage);
        needFlush = true;
    }
    if (needFlush) {
        Flush();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// Add a tab at the end of the list.
/// \par Description:
///     Add a new tab to the TabView. The new tab will be appended to the list
///     and will appear rightmost. Each tab is associated with title that is
///     printed inside it, and a View that will be made visible when the tab
///     is selected or hidden when the tab is inactive. If you want to use
///     the TabView to something else than flipping between views, you can
///     pass a nullptr, or you can pass the same ViewPointer for all tabs.
///     Then you can associate a message with the TabView that will be sent
///     to it's target every time the selection change.
/// \par Note:
///     Views associated with tabs should *NOT* be attached to any other
///     view when or after the tab is created. It will automatically be a
///     child of the TabView when added to a tab.
///
///     When a view is associated with a tab, the view will be resized to
///     fit the interior of the TabView.
///
/// \param title - The string that should appear inside the tab.
/// \param view  - The View to be associated with the tab, or nullptr.
///
/// \return The zero based index of the new tab.
/// \sa InsertTab(), DeleteTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int TabView::AppendTab(const String& title, Ptr<View> view)
{
    return InsertTab(m_TabList.size(), title, view);
}

///////////////////////////////////////////////////////////////////////////////
/// Insert tabs at a given position
/// \par Description:
///     Same as AppendTab() except that InsertTab() accept a zero based index
///     at which the tab will be inserted. Look at AppendTab() to get the full
///     truth.
/// \par Note:
///     If the rightmost tab is selected, the selection will be moved to
///     the previous tab, and a notification message (if any) is sent.
/// \par Warning:
///     The index *MUST* be between 0 and the current number of tabs!
///
/// \param index  - The zero based position where the tab is inserted.
/// \param pcTitle - The string that should appear inside the tab.
/// \param view  - The View to be associated with the tab, or nullptr.
///
/// \return The zero based index of the new tab (Same as index).
/// \sa AppendTab(), DeleteTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int TabView::InsertTab(size_t index, const String& title, Ptr<View> view)
{
    if (index > m_TabList.size()) index = m_TabList.size();
    if (m_SelectedTab != INVALID_INDEX && index <= m_SelectedTab) {
        m_SelectedTab++;
    }
    m_TabList.insert(m_TabList.begin() + index, Tab(title, view));
    if (view != nullptr && view->GetParent() != this)
    {
        AddChild(view);
        Rect bounds = GetNormalizedBounds();
        bounds.Resize(2, m_TabHeight, -2, -2);
        view->SetFrame(bounds);
        if (m_SelectedTab == INVALID_INDEX) {
            m_SelectedTab = index;
        } else {
            view->Show(false);
        }
    }
    m_TabList[index].m_Width = round(GetStringWidth(title) * 1.1f) + 4.0f;
    m_TotalWidth += m_TabList[index].m_Width;
    m_TopView->Invalidate();
    m_TopView->Flush();
    return index;
}

///////////////////////////////////////////////////////////////////////////////
/// Delete a given tab
/// \par Description:
///     The tab at position index is deleted, and the associated view is
///     removed from the child list unless it is still associated with any
///     of the remaining tabs. The view associated with the deleted tab
///     is returned, and must be deleted by the caller if it is no longer
///     needed.
/// \param index - The zero based index of the tab to delete.
/// \return Pointer to the view associated with the deleted tab.
/// \sa AppendTab(), InsertTab(), GetTabView(), GetTabTitle()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


Ptr<View> TabView::DeleteTab(size_t index)
{
    if (index >= m_TabList.size()) {
        return nullptr;
    }

    const size_t  oldSelection = m_SelectedTab;
    Ptr<View> view = m_TabList[index].m_View;
    Ptr<View> prevSelection;

    if (m_SelectedTab != INVALID_INDEX && m_SelectedTab < m_TabList.size() && m_TabList[m_SelectedTab].m_View != nullptr) {
        prevSelection = m_TabList[m_SelectedTab].m_View;
    }

    if (index < m_SelectedTab || m_SelectedTab == (m_TabList.size() - 1)) {
        m_SelectedTab--;
    }
    m_TotalWidth -= m_TabList[index].m_Width;

    m_TabList.erase(m_TabList.begin() + index);

    bool stillMember = false;
    for (size_t i = 0; i < m_TabList.size(); ++i)
    {
        if (m_TabList[i].m_View == view)
        {
            stillMember = true;
            break;
        }
    }
    if (!stillMember)
    {
        RemoveChild(view);
        if (index != oldSelection) {
            view->Show(); // Leave the view as we found it.
        }
    }
    if (!m_TabList.empty())
    {
        if (prevSelection != m_TabList[m_SelectedTab].m_View)
        {
            if (prevSelection != nullptr && prevSelection != view) {
                prevSelection->Hide();
            }
            if (m_TabList[m_SelectedTab].m_View != nullptr)
            {
                Ptr<View> newView = m_TabList[m_SelectedTab].m_View;
                Rect clientBounds = GetNormalizedBounds();
                clientBounds.Resize(2.0f, m_TabHeight, -2.0f, -2.0f);
                newView->SetFrame(clientBounds);
                newView->Show(true);
            }
        }
    }
    m_TopView->Invalidate();
    Invalidate();
    m_TopView->Flush();
    return view;
}

///////////////////////////////////////////////////////////////////////////////
/// Get the View associated with a given tab.
/// \param index - The zero based index of the tab.
/// \return Pointer to the View associated with the tab.
/// \sa GetTabTitle(), AppendTab(), InsertTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


Ptr<View> TabView::GetTabView(size_t index) const
{
    return m_TabList[index].m_View;
}

///////////////////////////////////////////////////////////////////////////////
/// Get the title of a given tab.
/// \param index - The zero based index of the tab.
/// \return const reference to a STL string containing the title.
/// \sa GetTabView(), AppendTab(), InsertTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const std::string& TabView::GetTabTitle(size_t index) const
{
    return m_TabList[index].m_Title;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int TabView::SetTabTitle(size_t index, const String& title)
{
    m_TabList[index].m_Title = title;

    float vOldWidth = m_TabList[index].m_Width;
    m_TabList[index].m_Width = round(GetStringWidth(title) * 1.1f) + 4.0f;
    m_TotalWidth += m_TabList[index].m_Width - vOldWidth;

    Invalidate();
    m_TopView->Invalidate();
    m_TopView->Flush();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// Get number of tabs currently added to the view.
/// \return Tab count
/// \sa AppendTab(), InsertTab(), DeleteTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int TabView::GetTabCount() const
{
    return m_TabList.size();
}

///////////////////////////////////////////////////////////////////////////////
/// Get the current selection
/// \par Description:
///     Returns the zero based index of the selected tab.
/// \par Note:
///     This index is also added to the notification sent when the selection
///     change. The selection is then added under the name "selection".
/// \return The zero based index of the selected tab.
/// \sa SetSelection(), Invoker::SetMessage(), Invoker::SetTarget()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t TabView::GetSelection()
{
    return m_SelectedTab;
}

///////////////////////////////////////////////////////////////////////////////
/// Select a tab, and optionally notify the target.
/// \par Description:
///     Selects the given tab. If notify is true, and a messages has been
///     assigned through Invoker::SetMessage() the message will be sendt
///     to the target.
/// \param index - The zero based index of the tab to select.
/// \param notify - Set to true if Invoker::Invoke() should be called.
/// \sa GetSelection()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TabView::SetSelection(size_t index, bool notify)
{
    if (index != m_SelectedTab && index < m_TabList.size())
    {
        if (m_SelectedTab < m_TabList.size() && m_TabList[m_SelectedTab].m_View != nullptr) {
            m_TabList[m_SelectedTab].m_View->Show(false);
        }
        m_SelectedTab = index;
        if (m_TabList[m_SelectedTab].m_View != nullptr)
        {
            Ptr<View> view = m_TabList[m_SelectedTab].m_View;
            Rect clientBounds = GetNormalizedBounds();
            clientBounds.Resize(2.0f, m_TabHeight, -2.0f, -2.0f);
            view->SetFrame(clientBounds);
            view->Show(true);
//            view->MakeFocus(true);
        }
        Invalidate();
        m_TopView->Invalidate();
        m_TopView->Flush();

        SignalSelectionChanged(m_SelectedTab, m_TabList[m_SelectedTab].m_View, this);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TabView::OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (m_HitButton != MouseButton_e::None || m_TabList.empty() || position.y >= m_TabHeight) {
        return false;
    }
    m_HitButton = button;
    m_HitPos    = position;

    float x = m_ScrollOffset;

    for (size_t i = 0; i < m_TabList.size(); ++i)
    {
        float width = m_TabList[i].m_Width;
        if (position.x > x && position.x < x + width) {
            SetSelection(i);
            break;
        }
        x += width;
    }
    MakeFocus(button, true);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TabView::OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (button != m_HitButton) {
        return false;
    }
    MakeFocus(button, false);
    m_HitButton = MouseButton_e::None;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool TabView::OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event)
{
    if (button == m_HitButton)
    {
        float oldOffset = m_ScrollOffset;

        float width = Width();

        if (m_TotalWidth <= width /*&& (GetQualifiers() & QUAL_SHIFT) == 0*/) {
            return false;
        }

        m_ScrollOffset += position.x - m_HitPos.x;

        if (m_TotalWidth <= width)
        {
            if (m_ScrollOffset > width - m_TotalWidth) {
                m_ScrollOffset = width - m_TotalWidth;
            }
            if (m_ScrollOffset < 0.0f) {
                m_ScrollOffset = 0.0f;
            }
        }
        else
        {
            width -= ARROW_SPACE * 2.0f;
            if (m_ScrollOffset > 0.0f) {
                m_ScrollOffset = 0.0f;
            }
            if (m_TotalWidth + m_ScrollOffset < width) {
                m_ScrollOffset = width - m_TotalWidth;
            }
        }
        if (m_ScrollOffset != oldOffset)
        {
            m_HitPos = position;
            if (m_TotalWidth <= width)
            {
                Invalidate();
            }
            else
            {
                Rect cBounds = GetBounds();
                Invalidate(Rect(cBounds.left, m_TabHeight - 2.0f, cBounds.right, m_TabHeight));
            }
            m_TopView->ScrollBy(m_ScrollOffset - oldOffset, 0.0f);
            Flush();
        }
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TabView::OnKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& event)
{
    switch (keyCode)
    {
        case KeyCodes::CURSOR_LEFT:
            if (m_SelectedTab > 0) {
                SetSelection(m_SelectedTab - 1);
            }
            break;
        case KeyCodes::CURSOR_RIGHT:
            if (m_SelectedTab < m_TabList.size() - 1) {
                SetSelection(m_SelectedTab + 1);
            }
            break;
        default:
            View::OnKeyDown(keyCode, text, event);
            break;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \par Description:
///     TabView overloads GetPreferredSize() and return the largest size returned
///     by any of the views associated with the different tabs.
/// \return The largest preffered size returned by it's childs.
/// \sa View::GetPreferredSize()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TabView::CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const
{
    minSize->x = m_TotalWidth;
    minSize->y = 0.0f;
    *maxSize = *minSize;

    for (size_t i = 0; i < m_TabList.size(); ++i)
    {
        if (m_TabList[i].m_View != nullptr)
        {
            const Point viewMinSize = m_TabList[i].m_View->GetPreferredSize(PrefSizeType::Smallest);
            if (viewMinSize.x > minSize->x) minSize->x = viewMinSize.x;
            if (viewMinSize.y > minSize->y) minSize->y = viewMinSize.y;
            const Point viewMaxSize = m_TabList[i].m_View->GetPreferredSize(PrefSizeType::Greatest);
            if (viewMaxSize.x > maxSize->x) maxSize->x = viewMaxSize.x;
            if (viewMaxSize.y > maxSize->y) maxSize->y = viewMaxSize.y;
        }
    }
    minSize->y += m_TabHeight + 3.0f;
    minSize->x += 5.0f;

    maxSize->y += m_TabHeight + 3.0f;
    maxSize->x += 5.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static Color Tint(const Color& color, float tint)
{
    return Color(uint8_t((float(color.GetRed()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                 uint8_t((float(color.GetGreen()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                 uint8_t((float(color.GetBlue()) * tint + 127.0f * (1.0f - tint)) * 0.5f),
                 color.GetAlpha()
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TabView::TopView::Paint(const Rect& updateRect)
{
    const Rect bounds = GetBounds();

    const DynamicColor fgColor(StandardColorID::Shine);
    const DynamicColor bgColor(StandardColorID::Shadow);
    const Color fgShadowColor = Tint(fgColor, 0.9f);
    const Color bgShadowColor = Tint(bgColor, 0.8f);

    float x;

    if (m_TabView->m_TotalWidth > m_TabView->Width()) {
        x = 0.0f;
    } else {
        x = -2.0f;
    }

    const float       tabHeight = m_TabView->m_TabHeight;
    const FontHeight  fontHeight = m_TabView->m_FontHeight;
    const float       glyphHeight = m_TabView->m_GlyphHeight;

    FillRect(bounds, get_standard_color(StandardColorID::DefaultBackground));
    for (size_t i = 0; i < m_TabView->m_TabList.size(); ++i)
    {
        float width = m_TabView->m_TabList[i].m_Width;
        Rect tabFrame(x + 2.0f, 2.0f, x + width + 2.0f, m_TabView->m_TabHeight);

        if (i == m_TabView->m_SelectedTab) {
            tabFrame.Resize(-2.0f, -2.0f, 2.0f, 0.0f);
        }
        SetFgColor(fgShadowColor);
        if (i != m_TabView->m_SelectedTab + 1)
        {
            DrawLine(Point(tabFrame.left, tabFrame.bottom - 3.0f), Point(tabFrame.left, tabFrame.top + CORNER_SIZE));
            DrawLine(Point(tabFrame.left + CORNER_SIZE, tabFrame.top));
            if (i == m_TabView->m_SelectedTab - 1) {
                DrawLine(Point(tabFrame.right + CORNER_SIZE - 3.0f, tabFrame.top));
            } else {
                DrawLine(Point(tabFrame.right - 1.0f, tabFrame.top));
            }

            SetFgColor(fgColor);

            DrawLine(Point(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f), Point(tabFrame.left + 1.0f, tabFrame.top + 1.0f + CORNER_SIZE));
            DrawLine(Point(tabFrame.left + 1.0f + CORNER_SIZE, tabFrame.top + 1.0f));
            if (i == m_TabView->m_SelectedTab - 1) {
                DrawLine(Point(tabFrame.right + CORNER_SIZE - 4.0f, tabFrame.top + 1.0f));
            } else {
                DrawLine(Point(tabFrame.right - 2.0f, tabFrame.top + 1.0f));
            }
        }
        else
        {
            DrawLine(Point(tabFrame.left + 2.0f, tabFrame.top), Point(tabFrame.right - 1.0f, tabFrame.top));
            SetFgColor(fgColor);
            DrawLine(Point(tabFrame.left + 2.0f, tabFrame.top + 1.0f), Point(tabFrame.right - 2.0f, tabFrame.top + 1.0f));
        }

        if (i != m_TabView->m_SelectedTab - 1)
        {
            SetFgColor(bgShadowColor);
            DrawLine(Point(tabFrame.right - 1.0f, tabFrame.top + 2.0f), Point(tabFrame.right - 1.0f, tabFrame.bottom - 3.0f));

            SetFgColor(bgColor);
            DrawLine(Point(tabFrame.right - 2.0f, tabFrame.top + 2.0f), Point(tabFrame.right - 2.0f, tabFrame.bottom - 2.0f));
        }
        SetFgColor(0, 0, 0);

        MovePenTo(round(x + (width - 4.0f) * 5.0f / 100.0f + 4.0f), round(tabFrame.top + tabHeight * 0.5f + fontHeight.ascender - fontHeight.line_gap * 0.5f - glyphHeight * 0.5f + 2.0f));

        DrawString(m_TabView->m_TabList[i].m_Title.c_str());
        x += width;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void TabView::Paint(const Rect& updateRect)
{
    Rect bounds = GetBounds();

    const float viewWidth = bounds.Width();

    Rect tabRect = bounds;
    tabRect.bottom = m_TabHeight - 2.0f;

    bounds.top += m_TabHeight - 2.0f;

    const DynamicColor fgColor(StandardColorID::Shine);
    const DynamicColor bgColor(StandardColorID::Shadow);

    const Color fgShadowColor = Tint(fgColor, 0.9f);
    const Color bgShadowColor = Tint(bgColor, 0.8f);

    SetFgColor(bgShadowColor);
    DrawLine(Point(bounds.right - 1.0f, bounds.top + 2.0f), Point(bounds.right - 1.0f, bounds.bottom - 1.0f));  // top/right -> bottom/right
    DrawLine(Point(bounds.left, bounds.bottom - 1.0f));                                                         // bottom/left

    SetFgColor(bgColor);
    DrawLine(Point(bounds.right - 2.0f, bounds.top + 2.0f), Point(bounds.right - 2.0f, bounds.bottom - 2.0f));  // top/right -> bottom/right
    DrawLine(Point(bounds.left + 1.0f, bounds.bottom - 2.0f));                                                  // bottom/left

    SetFgColor(fgShadowColor);
    DrawLine(Point(bounds.left, bounds.top - 1.0f), Point(bounds.left, bounds.bottom - 3.0f));                  // top/left -> bottom/left

    SetFgColor(fgColor);
    DrawLine(Point(bounds.left + 1.0f, bounds.top + 1.0f), Point(bounds.left + 1.0f, bounds.bottom - 3.0f));    // top/left -> bottom/left

    float x = m_ScrollOffset;

    if (m_TotalWidth > viewWidth) {
        x += ARROW_SPACE;
    }
    if (m_TotalWidth <= viewWidth)
    {
        SetFgColor(StandardColorID::DefaultBackground);
        FillRect(Rect(0.0f, 0.0f, 2.0f, m_TabHeight - 2.0f));
        FillRect(Rect(tabRect.right - 2.0f, 0.0f, tabRect.right, m_TabHeight - 2.0f));
    }

    for (size_t i = 0; i < m_TabList.size(); ++i)
    {
        float width = m_TabList[i].m_Width;
        Rect tabFrame(x + 2.0f, 2.0f, x + width + 2.0f, m_TabHeight);

        if (i == m_SelectedTab) {
            tabFrame.Resize(-2.0f, -2.0f, 2.0f, 0.0f);
        }

        if (m_TotalWidth <= viewWidth)
        {
            if (i != m_SelectedTab + 1)
            {
                SetFgColor(fgShadowColor);
                DrawLine(Point(tabFrame.left, tabFrame.bottom - 3.0f), Point(tabFrame.left, tabFrame.top + 2.0f + CORNER_SIZE));
                DrawLine(Point(tabFrame.left + 3.0f, tabFrame.top));
                SetFgColor(fgColor);
                DrawLine(Point(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f), Point(tabFrame.left + 1.0f, tabFrame.top + 2.0f + CORNER_SIZE));
                DrawLine(Point(tabFrame.left + 1.0f + CORNER_SIZE, tabFrame.top + 1.0f));
            }
        }
        else
        {
            if (i == m_SelectedTab) {
                DrawLine(Point(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f), Point(tabFrame.left + 1.0f, tabFrame.bottom - 2.0f));
            }
        }
        if (i == m_SelectedTab)
        {
            SetFgColor(StandardColorID::DefaultBackground);
            float x1 = x + 2.0f;
            float x2 = x + width + 2.0f;

            if (m_TotalWidth > viewWidth)
            {
                if (x1 < ARROW_SPACE) {
                    x1 = ARROW_SPACE;
                }
                if (x2 > bounds.right - 2.0f - ARROW_SPACE) {
                    x2 = bounds.right - 2.0f - ARROW_SPACE;
                }
            }
            DrawLine(Point(x1, tabFrame.bottom - 2.0f), Point(x2, tabFrame.bottom - 2.0f));
            DrawLine(Point(x1, tabFrame.bottom - 1.0f), Point(x2, tabFrame.bottom - 1.0f));

            SetFgColor(fgShadowColor);

            if (x >= 0.0f) {
                DrawLine(Point(0, tabFrame.bottom - 2.0f), Point(x, tabFrame.bottom - 2.0f));
            }
            DrawLine(Point(x + width + 2, tabFrame.bottom - 2.0f), Point(bounds.right - 1.0f, tabFrame.bottom - 2.0f));

            SetFgColor(fgColor);
            if (x >= 1) {
                DrawLine(Point(1, tabFrame.bottom - 1.0f), Point(x + 1.0f, tabFrame.bottom - 1.0f));
            }
            DrawLine(Point(x + width + 2.0f, tabFrame.bottom - 1.0f), Point(bounds.right - 1.0f, tabFrame.bottom - 1.0f));
        }
        x += width;
    }

    if (m_TotalWidth > viewWidth)
    {
        SetFgColor(StandardColorID::DefaultBackground);
        FillRect(Rect(0.0f, 0.0f, ARROW_SPACE, m_TabHeight - 2.0f));
        FillRect(Rect(tabRect.right - ARROW_SPACE, 0.0f, tabRect.right, m_TabHeight - 2.0f));
        SetFgColor(bgColor);
        for (float x = 2.0f; x < ARROW_SPACE - 2.0f; x += 1.0f)
        {
            const float y1 = m_TabHeight * 0.5f - (x - 2.0f);
            const float y2 = m_TabHeight * 0.5f + (x - 2.0f);
            DrawLine(Point(x, y1), Point(x, y2));
            DrawLine(Point(bounds.right - x - 1.0f, y1), Point(bounds.right - x - 1.0f, y2));
        }

        SetFgColor(fgShadowColor);
        DrawLine(Point(0, m_TabHeight - 2.0f), Point(ARROW_SPACE, m_TabHeight - 2.0f));
        DrawLine(Point(bounds.right - 1.0f - ARROW_SPACE, m_TabHeight - 2.0f), Point(bounds.right - 1.0f, m_TabHeight - 2.0f));
        SetFgColor(fgColor);
        DrawLine(Point(1.0f, m_TabHeight - 1.0f), Point(ARROW_SPACE, m_TabHeight - 1.0f));
        DrawLine(Point(bounds.right - 1.0f - ARROW_SPACE, m_TabHeight - 1.0f), Point(bounds.right - 1.0f, m_TabHeight - 1.0f));
    }
}
