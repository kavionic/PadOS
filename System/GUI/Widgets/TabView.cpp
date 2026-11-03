// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////


#include <algorithm>
#include <cstring>

#include <GUI/Style/GUIStyle.h>
#include <GUI/Widgets/TabView.h>
#include <GUI/Widgets/ScrollView.h>
#include <GUI/ViewFactory.h>
#include <System/ExceptionHandling.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabView::PTabView(const PString& name, PViewFlags flags) : PStyledView(name, flags | PViewFlag::WillDraw)
{
    Initialize();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTabView::PTabView(PViewFactoryContext& context, const pugi::xml_node& xmlData) : PStyledView(context, xmlData)
{
    MergeFlags(PViewFlag::WillDraw);
    Initialize();

    for (pugi::xml_node childNode = xmlData.first_child(); childNode; childNode = childNode.next_sibling())
    {
        if (strcmp(childNode.name(), "_TabViewTab") == 0)
        {
            const char* tabName = childNode.attribute("name").value();
            Ptr<PView> tabContentView;
            if (childNode.first_child())
            {
                tabContentView = PViewFactory::Get().CreateView(context, nullptr, childNode);
                if (tabContentView != nullptr)
                {
                    if (tabContentView->GetLayoutNode() == nullptr) {
                        tabContentView->SetLayoutNode(ptr_new<PLayoutNode>());
                    }
                    tabContentView->SetName(tabName);
                    tabContentView->MergeFlags(PViewFlag::WillDraw);
                }
            }
            AppendTab(tabName, tabContentView);
        }
        else if (strcmp(childNode.name(), "_TopBarClientView") == 0)
        {
            const char* viewName = childNode.attribute("name").value();
            Ptr<PView> topBarClientView;
            if (childNode.first_child())
            {
                topBarClientView = PViewFactory::Get().CreateView(context, nullptr, childNode);
                if (topBarClientView != nullptr)
                {
                    if (topBarClientView->GetLayoutNode() == nullptr) {
                        topBarClientView->SetLayoutNode(ptr_new<PLayoutNode>());
                    }
                    topBarClientView->SetName(viewName);
                    topBarClientView->MergeFlags(PViewFlag::WillDraw);
                }
            }
            SetTopBarClientView(topBarClientView);
        }
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

int PTabView::AppendTab(const PString& title, Ptr<PView> view)
{
    return InsertTab(m_TabViews.size(), title, view);
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
/// \param title - The string that should appear inside the tab.
/// \param view  - The View to be associated with the tab, or nullptr.
///
/// \return The zero based index of the new tab (Same as index).
/// \sa AppendTab(), DeleteTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PTabView::InsertTab(size_t index, const PString& title, Ptr<PView> view)
{
    m_TabsView->CancelInteraction();
    index = std::min(index, m_TabViews.size());
    m_TabViewStyleData.Titles.insert(m_TabViewStyleData.Titles.begin() + index, title);
    PScopeFail restoreTitles([this, index] { m_TabViewStyleData.Titles.erase(m_TabViewStyleData.Titles.begin() + index); });
    m_TabViews.insert(m_TabViews.begin() + index, view);
    restoreTitles.release();
    if (m_TabViewStyleData.Selection != INVALID_INDEX && index <= m_TabViewStyleData.Selection) {
        ++m_TabViewStyleData.Selection;
    }
    const bool addView = view != nullptr && view->GetParent() != this;
    if (addView && m_TabViewStyleData.Selection == INVALID_INDEX) {
        m_TabViewStyleData.Selection = index;
    }
    NotifyStylePropertyChanged(PTabViewProperty::TabInserted, index);
    if (addView)
    {
        AddChild(view);
        view->SetFrame(GetClientFrame());
        if (index != m_TabViewStyleData.Selection) {
            view->Hide();
        }
    }
    Flush();
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


Ptr<PView> PTabView::DeleteTab(size_t index)
{
    if (index >= m_TabViews.size()) {
        return nullptr;
    }
    m_TabsView->CancelInteraction();
    const size_t oldSelection = m_TabViewStyleData.Selection;
    Ptr<PView> view = m_TabViews[index];
    Ptr<PView> previousView = (oldSelection < m_TabViews.size()) ? m_TabViews[oldSelection] : nullptr;
    m_TabViews.erase(m_TabViews.begin() + index);
    m_TabViewStyleData.Titles.erase(m_TabViewStyleData.Titles.begin() + index);
    if (oldSelection != INVALID_INDEX)
    {
        if (m_TabViews.empty()) {
            m_TabViewStyleData.Selection = INVALID_INDEX;
        } else if (index < oldSelection || oldSelection == m_TabViews.size()) {
            --m_TabViewStyleData.Selection;
        }
    }
    NotifyStylePropertyChanged(PTabViewProperty::TabDeleted, index);
    const bool stillMember = std::find(m_TabViews.begin(), m_TabViews.end(), view) != m_TabViews.end();
    if (view != nullptr && !stillMember)
    {
        RemoveChild(view);
        if (view != previousView) {
            view->Show(); // Leave the view as we found it.
        }
    }
    Ptr<PView> selectedView = (m_TabViewStyleData.Selection < m_TabViews.size()) ? m_TabViews[m_TabViewStyleData.Selection] : nullptr;
    if (previousView != selectedView)
    {
        if (previousView != nullptr && (previousView != view || stillMember)) {
            previousView->Hide();
        }
        if (selectedView != nullptr) {
            selectedView->Show();
        }
    }
    Flush();
    return view;
}

///////////////////////////////////////////////////////////////////////////////
/// Get the View associated with a given tab.
/// \param index - The zero based index of the tab.
/// \return Pointer to the View associated with the tab.
/// \sa GetTabTitle(), AppendTab(), InsertTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


Ptr<PView> PTabView::GetTabView(size_t index) const
{
    return m_TabViews[index];
}

///////////////////////////////////////////////////////////////////////////////
/// Get number of tabs currently added to the view.
/// \return Tab count
/// \sa AppendTab(), InsertTab(), DeleteTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PTabView::GetTabCount() const
{
    return m_TabViews.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PTabView::SetTabTitle(size_t index, const PString& title)
{
    if (m_TabViewStyleData.Titles[index] != title)
    {
        m_TabsView->CancelInteraction();
        m_TabViewStyleData.Titles[index] = title;
        NotifyStylePropertyChanged(PTabViewProperty::Title, index);
        Flush();
    }
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// Get the title of a given tab.
/// \param index - The zero based index of the tab.
/// \return const reference to a STL string containing the title.
/// \sa GetTabView(), AppendTab(), InsertTab()
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const std::string& PTabView::GetTabTitle(size_t index) const
{
    return m_TabViewStyleData.Titles[index];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PView> PTabView::SetTopBarClientView(Ptr<PView> view)
{
    if (view != m_TopBarClientView)
    {
        Ptr<PView> previousView = m_TopBarClientView;
        if (previousView != nullptr)
        {
            previousView->SignalPreferredSizeChanged.Disconnect(this, &PTabView::SlotTopBarPreferredSizeChanged);
            previousView->RemoveThis();
        }
        m_TopBarClientView = view;
        SlotTopBarPreferredSizeChanged();
        if (view != nullptr)
        {
            view->SignalPreferredSizeChanged.Connect(this, &PTabView::SlotTopBarPreferredSizeChanged);
            AddChild(view);
        }
        return previousView;
    }
    return m_TopBarClientView;
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

size_t PTabView::GetSelection()
{
    return m_TabViewStyleData.Selection;
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

void PTabView::SetSelection(size_t index, bool notify)
{
    if (index != m_TabViewStyleData.Selection && index < m_TabViews.size())
    {
        Ptr<PView> previousView = (m_TabViewStyleData.Selection < m_TabViews.size()) ? m_TabViews[m_TabViewStyleData.Selection] : nullptr;
        Ptr<PView> selectedView = m_TabViews[index];
        if (previousView != selectedView && previousView != nullptr) {
            previousView->Hide();
        }
        m_TabViewStyleData.Selection = index;
        NotifyStylePropertyChanged(PTabViewProperty::Selection);
        if (previousView != selectedView && selectedView != nullptr) {
            selectedView->Show();
        }
        Flush();
        if (notify) {
            SignalSelectionChanged(index, selectedView, this);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PRect PTabView::GetClientFrame() const
{
    const PGUIStyle* style = GetStyle();
    return (style != nullptr) ? style->TabViewGetFrames(*this, m_TabViewStyleData).ClientFrame : GetNormalizedBounds();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::OnFontChanged(Ptr<const PFont> newFont)
{
    m_TabsView->CancelInteraction();
    m_TopView->SetFont(newFont);
    m_TabsView->SetFont(newFont);
    NotifyStylePropertyChanged(PTabViewProperty::Font);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::OnStyleChanged()
{
    m_TabViewStyleData.StyleData.Reset();
    m_TabsView->CancelInteraction();
    PStyledView::OnStyleChanged();
    Layout(PPoint());
    m_TabsView->ContentSizeChanged();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::Layout(const PPoint& delta)
{
    const PGUIStyle* style = GetStyle();
    PTabViewFrames frames;
    if (style != nullptr) {
        frames = style->TabViewGetFrames(*this, m_TabViewStyleData);
    } else {
        frames.ClientFrame = GetNormalizedBounds();
    }
    if (m_TabViewStyleData.Selection < m_TabViews.size())
    {
        Ptr<PView> view = m_TabViews[m_TabViewStyleData.Selection];
        if (view != nullptr && view->GetFrame() != frames.ClientFrame) {
            view->SetFrame(frames.ClientFrame);
        }
    }
    if (m_TopView->GetFrame() != frames.TabsFrame) {
        m_TopView->SetFrame(frames.TabsFrame);
    }
    m_TopView->RefreshLayout();
    if (m_TopBarClientView != nullptr && m_TopBarClientView->GetFrame() != frames.TopBarClientFrame) {
        m_TopBarClientView->SetFrame(frames.TopBarClientFrame);
    }
    if (style != nullptr)
    {
        style->TabViewPropertyChanged(*this, *m_TabsView, m_TabViewStyleData, PTabViewProperty::Frame, INVALID_INDEX);
    }
    else
    {
        m_TabsView->Invalidate();
        Invalidate();
    }
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::OnFrameSized(const PPoint& delta)
{
    PStyledView::OnFrameSized(delta);
    Layout(delta);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& event)
{
    switch (event.KeySymbol)
    {
        case PKeySymbol::LEFT_ARROW:
            if (m_TabViewStyleData.Selection != INVALID_INDEX && m_TabViewStyleData.Selection > 0) {
                SetSelection(m_TabViewStyleData.Selection - 1);
            }
            break;
        case PKeySymbol::RIGHT_ARROW:
            if (!m_TabViews.empty() && m_TabViewStyleData.Selection < m_TabViews.size() - 1) {
                SetSelection(m_TabViewStyleData.Selection + 1);
            }
            break;
        default:
            PView::OnKeyDown(keyCode, text, event);
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

void PTabView::CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight)
{
    const PGUIStyle* style = GetStyle();
    if (style == nullptr)
    {
        PView::CalculatePreferredSize(minSize, maxSize, includeWidth, includeHeight);
    }
    else
    {
        *minSize = PPoint();
        *maxSize = PPoint();
    }
    for (const Ptr<PView>& view : m_TabViews)
    {
        if (view != nullptr)
        {
            const PPoint viewMinSize = view->GetPreferredSize(PPrefSizeType::Smallest);
            const PPoint viewMaxSize = view->GetPreferredSize(PPrefSizeType::Greatest);
            minSize->x = std::max(minSize->x, viewMinSize.x);
            minSize->y = std::max(minSize->y, viewMinSize.y);
            maxSize->x = std::max(maxSize->x, viewMaxSize.x);
            maxSize->y = std::max(maxSize->y, viewMaxSize.y);
        }
    }
    if (style != nullptr)
    {
        const PPoint contentMinSize = *minSize;
        const PPoint contentMaxSize = *maxSize;
        style->TabViewGetPreferredSize(*this, m_TabViewStyleData, contentMinSize, contentMaxSize, minSize, maxSize);
    }
}


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->TabViewPaint(*this, *m_TabsView, updateRect, m_TabViewStyleData);
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::Initialize()
{
    m_TopView = ptr_new<PScrollView>("top_view");
    m_TabsView = ptr_new<TabsView>(this);
    m_TopView->SetFont(GetFont());
    m_TabsView->SetFont(GetFont());
    m_TopView->SetMaxOverscroll(0.0f, 0.0f);
    m_TopView->SetScrolledView(m_TabsView);
    AddChild(m_TopView);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::NotifyStylePropertyChanged(PTabViewProperties properties, size_t tabIndex)
{
    const PGUIStyle* style = GetStyle();
    if (style != nullptr) {
        style->TabViewPropertyChanged(*this, *m_TabsView, m_TabViewStyleData, properties, tabIndex);
    } else {
        PreferredSizeChanged();
    }
    Layout(PPoint());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::SlotTopBarPreferredSizeChanged()
{
    m_TabViewStyleData.HasTopBarClient = m_TopBarClientView != nullptr;
    if (m_TopBarClientView != nullptr)
    {
        m_TabViewStyleData.TopBarMinSize = m_TopBarClientView->GetPreferredSize(PPrefSizeType::Smallest);
        m_TabViewStyleData.TopBarMaxSize = m_TopBarClientView->GetPreferredSize(PPrefSizeType::Greatest);
    }
    else
    {
        m_TabViewStyleData.TopBarMinSize = PPoint();
        m_TabViewStyleData.TopBarMaxSize = PPoint();
    }
    NotifyStylePropertyChanged(PTabViewProperty::TopBar);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPaint(const PRect& updateRect)
{
    const PGUIStyle* style = m_TabView->GetStyle();
    if (style != nullptr) {
        style->TabViewTabsPaint(*this, *m_TabView, updateRect, m_TabView->m_TabViewStyleData);
    } else {
        PView::OnPaint(updateRect);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PPoint PTabView::TabsView::CalculateContentSize() const
{
    const PGUIStyle* style = m_TabView->GetStyle();
    return (style != nullptr) ? style->TabViewGetTabsContentSize(*m_TabView, m_TabView->m_TabViewStyleData) : PPoint();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnViewScrolled(const PPoint& delta)
{
    m_IsHoverSuppressed = true;
    CancelInteraction();
    const PGUIStyle* style = m_TabView->GetStyle();
    if (style != nullptr) {
        style->TabViewPropertyChanged(*m_TabView, *this, m_TabView->m_TabViewStyleData, PTabViewProperty::Scroll, INVALID_INDEX);
    }
    Flush();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase != PEventPhase::Target || m_HitPointerID != PInvalidPointerID) {
        return;
    }
    const size_t index = GetTabFromPosition(position);
    if (index != INVALID_INDEX && SetPointerCapture(pointerID, PPointerCaptureMode::Preemptible))
    {
        m_HitPointerID = pointerID;
        m_PressedTab = index;
        m_IsHoverSuppressed = false;
        SetHighlightedTab(index);
        m_TabView->SetSelection(index);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Target && pointerID == m_HitPointerID)
    {
        const size_t index = GetTabFromPosition(position);
        m_HitPointerID = PInvalidPointerID;
        m_PressedTab = INVALID_INDEX;
        SetHighlightedTab(event.SupportsHover ? index : INVALID_INDEX);
        ReleasePointerCapture(pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase != PEventPhase::Target) {
        return;
    }
    if (pointerID == m_HitPointerID)
    {
        const size_t index = GetTabFromPosition(position);
        SetHighlightedTab((index == m_PressedTab) ? index : INVALID_INDEX);
    }
    else if (m_HitPointerID == PInvalidPointerID && event.SupportsHover && event.Buttons == PPointerButtonMaskNone && !IsScrolling(pointerID))
    {
        m_IsHoverSuppressed = false;
        SetHighlightedTab(GetTabFromPosition(position));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPointerOver(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Target && event.SupportsHover && m_HitPointerID == PInvalidPointerID
        && !m_IsHoverSuppressed && !IsScrolling(pointerID)) {
        SetHighlightedTab(GetTabFromPosition(position));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPointerOut(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Target && (m_HitPointerID == PInvalidPointerID || pointerID == m_HitPointerID)) {
        SetHighlightedTab(INVALID_INDEX);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPointerCancel(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase)
{
    if (phase == PEventPhase::Target && pointerID == m_HitPointerID) {
        CancelInteraction();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason)
{
    if (pointerID == m_HitPointerID) {
        CancelInteraction();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::CancelInteraction()
{
    const PPointerID pointerID = m_HitPointerID;
    m_HitPointerID = PInvalidPointerID;
    m_PressedTab = INVALID_INDEX;
    if (pointerID != PInvalidPointerID || m_TabView->m_TabViewStyleData.HighlightedTab != INVALID_INDEX) {
        m_IsHoverSuppressed = true;
    }
    SetHighlightedTab(INVALID_INDEX);
    if (pointerID != PInvalidPointerID) {
        ReleasePointerCapture(pointerID);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTabView::TabsView::GetTabFromPosition(const PPoint& position) const
{
    const PGUIStyle* style = m_TabView->GetStyle();
    return (style != nullptr) ? style->TabViewGetTabFromPosition(*m_TabView, *this, m_TabView->m_TabViewStyleData, position) : INVALID_INDEX;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PTabView::TabsView::IsScrolling(PPointerID pointerID) const
{
    const PInertialScroller::State state = m_TabView->m_TopView->GetInertialScrollerState();
    const bool isInertialScroll = state == PInertialScroller::State::Dragging || state == PInertialScroller::State::Coasting;
    return m_TabView->m_TopView->HasPointerCapture(pointerID) || isInertialScroll;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PTabView::TabsView::SetHighlightedTab(size_t index)
{
    PTabViewStyleData& data = m_TabView->m_TabViewStyleData;
    if (data.HighlightedTab != index)
    {
        data.HighlightedTab = index;
        const PGUIStyle* style = m_TabView->GetStyle();
        if (style != nullptr) {
            style->TabViewPropertyChanged(*m_TabView, *this, data, PTabViewProperty::Highlight, INVALID_INDEX);
        } else {
            Invalidate();
        }
        Flush();
    }
}
