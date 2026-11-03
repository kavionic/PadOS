// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Style/TabViewStyleData.h>
#include <GUI/StyledView.h>

#include <vector>
#include <string>

class PScrollView;

class PTabView : public PStyledView
{
public:
    PTabView(const PString& name = PString::zero, PViewFlags flags = PViewFlags::zero);
    PTabView(PViewFactoryContext& context, const pugi::xml_node& xmlData);

    int         AppendTab( const PString& title, Ptr<PView> view = nullptr );
    int         InsertTab(size_t index, const PString& title, Ptr<PView> view = nullptr );
    Ptr<PView>   DeleteTab(size_t index );
    Ptr<PView>   GetTabView(size_t index ) const;
    int         GetTabCount() const;

    int         SetTabTitle(size_t index, const PString& title );
    const std::string&  GetTabTitle(size_t index ) const;

    Ptr<PView> SetTopBarClientView(Ptr<PView> view);


    size_t      GetSelection();
    void        SetSelection(size_t index, bool notify = true );

    PRect GetClientFrame() const;

    virtual void    OnFontChanged(Ptr<const PFont> newFont) override;
    virtual void    OnStyleChanged() override;
    virtual void    Layout(const PPoint& delta);
    virtual void    OnFrameSized(const PPoint& delta) override;
    virtual void    OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& event) override;

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void    OnPaint(const PRect& updateRect) override;

    Signal<void, size_t, Ptr<PView>, PTabView*> SignalSelectionChanged;//(size_t index, Ptr<View> tabView, TabView* source)
private:
    void Initialize();
    void NotifyStylePropertyChanged(PTabViewProperties properties, size_t tabIndex = INVALID_INDEX);
    void SlotTopBarPreferredSizeChanged();

    class TabsView : public PView
    {
    public:
        TabsView(PTabView* tabView) : PView("tabs_view", PViewFlag::WillDraw), m_TabView(tabView) {}

        virtual void OnPaint(const PRect& updateRect) override;
        virtual PPoint CalculateContentSize() const override;
        virtual void OnViewScrolled(const PPoint& delta) override;
        virtual void OnPointerDown(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
        virtual void OnPointerUp(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
        virtual void OnPointerMove(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
        virtual void OnPointerOver(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
        virtual void OnPointerOut(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
        virtual void OnPointerCancel(PPointerID pointerID, const PPoint& position, const PPointerEvent& event, PEventPhase phase) override;
        virtual void OnPointerCaptureLost(PPointerID pointerID, PPointerCaptureLostReason reason) override;

        void CancelInteraction();

    private:
        size_t GetTabFromPosition(const PPoint& position) const;
        bool IsScrolling(PPointerID pointerID) const;
        void SetHighlightedTab(size_t index);

        PTabView* m_TabView;
        size_t m_PressedTab = INVALID_INDEX;
        PPointerID m_HitPointerID = PInvalidPointerID;
        bool m_IsHoverSuppressed = false;
    };

    PTabViewStyleData        m_TabViewStyleData;
    std::vector<Ptr<PView>>  m_TabViews;
    Ptr<PScrollView>        m_TopView;
    Ptr<TabsView>           m_TabsView;
    Ptr<PView>              m_TopBarClientView;
};
