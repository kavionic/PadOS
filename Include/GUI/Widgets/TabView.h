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

#include <GUI/View.h>
#include <GUI/Font.h>

#include <vector>
#include <string>


class PTabView : public PView
{
public:
    PTabView(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = 0);
    PTabView(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);

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

    virtual void    Layout(const PPoint& delta);
    virtual void    OnFrameSized(const PPoint& delta) override;
    virtual bool    OnMouseDown(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseUp(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual bool    OnMouseMove(PMouseButton button, const PPoint& position, const PMotionEvent& event) override;
    virtual void    OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& event) override;

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void    OnPaint(const PRect& updateRect) override;

    Signal<void, size_t, Ptr<PView>, PTabView*> SignalSelectionChanged;//(size_t index, Ptr<View> tabView, TabView* source)
private:
    void Initialize();
    float GetAvailableTabsWidth() const;

    struct Tab
    {
        Tab(const PString& title, Ptr<PView> view) : m_Title(title) { m_View = view; }
        Ptr<PView>   m_View;
        PString     m_Title;
        float       m_Width;
    };

    class TopView : public PView
    {
    public:
        TopView(PTabView* parent) : PView("top_view", ptr_tmp_cast(parent), PViewFlags::WillDraw) {
            m_TabView = parent;
        }
    
        virtual void OnPaint(const PRect& updateRect) override;
    private:
        PTabView* m_TabView;
    };

    friend class PTabView::TopView;
  
    size_t              m_SelectedTab = INVALID_INDEX;
    float               m_ScrollOffset = 0.0f;
    PPoint               m_HitPos;
    PMouseButton       m_HitButton = PMouseButton::None;
    float               m_TabHeight;
    float               m_GlyphHeight;
    PFontHeight          m_FontHeight;
    float               m_TotalTabsWidth;
    std::vector<Tab>    m_TabList;
    Ptr<TopView>        m_TopView;
    Ptr<PView>           m_TopBarClientView;
};
