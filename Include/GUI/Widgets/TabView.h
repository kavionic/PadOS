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

#include <GUI/View.h>
#include <GUI/Font.h>

#include <vector>
#include <string>

namespace os
{


class TabView : public View
{
public:
    TabView(const String& name = String::zero, Ptr<View> parent = nullptr, uint32_t flags = 0);
    TabView(ViewFactoryContext& context, Ptr<View> parent, const pugi::xml_node& xmlData);

    int         AppendTab( const String& title, Ptr<View> view = nullptr );
    int         InsertTab(size_t index, const String& title, Ptr<View> view = nullptr );
    Ptr<View>   DeleteTab(size_t index );
    Ptr<View>   GetTabView(size_t index ) const;
    int         GetTabCount() const;

    int         SetTabTitle(size_t index, const String& title );
    const std::string&  GetTabTitle(size_t index ) const;

    Ptr<View> SetTopBarClientView(Ptr<View> view);


    size_t      GetSelection();
    void        SetSelection(size_t index, bool notify = true );

    Rect GetClientFrame() const;

    virtual void    Layout(const Point& delta);
    virtual void    FrameSized(const Point& delta) override;
    virtual bool    OnMouseDown(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseUp(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual bool    OnMouseMove(MouseButton_e button, const Point& position, const MotionEvent& event) override;
    virtual void    OnKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& event) override;

    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void    Paint(const Rect& updateRect) override;

    Signal<void, size_t, Ptr<View>, TabView*> SignalSelectionChanged;//(size_t index, Ptr<View> tabView, TabView* source)
private:
    void Initialize();
    float GetAvailableTabsWidth() const;

    struct Tab
    {
        Tab(const String& title, Ptr<View> view) : m_Title(title) { m_View = view; }
        Ptr<View>   m_View;
        String      m_Title;
        float       m_Width;
    };

    class TopView : public View
    {
    public:
        TopView(TabView* parent) : View("top_view", ptr_tmp_cast(parent), ViewFlags::WillDraw) {
            m_TabView = parent;
        }
    
        virtual void Paint(const Rect& updateRect) override;
    private:
        TabView* m_TabView;
    };

    friend class TabView::TopView;
  
    size_t              m_SelectedTab = INVALID_INDEX;
    float               m_ScrollOffset = 0.0f;
    Point               m_HitPos;
    MouseButton_e       m_HitButton = MouseButton_e::None;
    float               m_TabHeight;
    float               m_GlyphHeight;
    FontHeight          m_FontHeight;
    float               m_TotalTabsWidth;
    std::vector<Tab>    m_TabList;
    Ptr<TopView>        m_TopView;
    Ptr<View>           m_TopBarClientView;
};


}
