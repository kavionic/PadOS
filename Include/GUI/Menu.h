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
#include <GUI/ViewScroller.h>

namespace os
{
class Menu;
class MenuItem;
class MenuRenderView;

enum class MenuLayout : uint8_t
{
    Vertical,
    Horizontal,
};

enum class MenuLocation
{
    Auto,
    TopLeft,
    BottomLeft
};


/**
 * \ingroup gui
 * \par Description:
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class Menu : public View, public ViewScroller
{
public:
    static constexpr int    LEFT_ARROW_WIDTH = 10;
    static constexpr int    LEFT_ARROW_HEIGHT = 19;

    Menu(const String& title = String::zero, MenuLayout layout = MenuLayout::Vertical, uint32_t flags = 0);
    ~Menu();

//  From View:
//    void        WindowActivated( bool bIsActive );

    virtual void    CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;

    virtual void    OnKeyDown(KeyCodes keyCode, const String& text, const KeyEvent& event) override;

    virtual void    FrameSized(const Point& delta) override;

    virtual void    Paint(const Rect& updateRect) override;

//  From Menu:
    String          GetLabel() const;
    MenuLayout      GetLayout() const;
    Ptr<MenuItem>   AddStringItem(const String& label, int id = 0);
    Ptr<MenuItem>   AddItem(Ptr<MenuItem> item);
    Ptr<MenuItem>   AddItem(Ptr<MenuItem> item, size_t index);
    Ptr<MenuItem>   AddSubMenu(Ptr<Menu> subMenu);
    Ptr<MenuItem>   AddSubMenu(Ptr<Menu> subMenu, size_t index);

    Ptr<MenuItem>   RemoveItem(size_t index);
    bool            RemoveItem(Ptr<MenuItem> item);
    bool            RemoveItem(Ptr<Menu> subMenu);

    Ptr<MenuItem>   GetItemAt(size_t index) const;
    Ptr<MenuItem>   GetItemAt(Point position) const;
    Ptr<Menu>       GetSubMenuAt(size_t index) const;
    size_t          GetItemCount() const;

    size_t          GetIndexOf(Ptr<MenuItem> item) const;
    size_t          GetIndexOf(Ptr<Menu> subMenu) const;

    Ptr<MenuItem>   FindItem(int id) const;
    Ptr<MenuItem>   FindItem(const String& name) const;

    Ptr<MenuItem>   FindMarked() const;

    Ptr<Menu>       GetSuperMenu() const;
    Ptr<MenuItem>   GetSuperItem();
    void            InvalidateLayout();

    void    Open(const Point& screenPosition, MenuLocation relativeLocation);

    Signal<void, Ptr<MenuItem>, Ptr<Menu>> SignalItemSelected;
private:
    friend class MenuItem;
    friend class MenuRenderView;

    void        StartOpenTimer(TimeValMicros delay);
    void        OpenSelection();
    void        SelectItem(Ptr<MenuItem> item);
    void        SelectPrev();
    void        SelectNext();
    void        Close(bool closeChilds, bool closeParent, Ptr<MenuItem> selectedItem);
    void        InvalidateItem(MenuItem* item);
    Ptr<Bitmap> GetArrowRightBitmap();

    std::vector<Ptr<MenuItem>>  m_Items;
    EventTimer                  m_OpenTimer;
    MenuLayout                  m_Layout;
    MenuItem*                   m_SuperItem = nullptr;
    Point                       m_ContentSize;

    Rect                        m_ItemBorders;

    Ptr<MenuRenderView>         m_ContentView;
    Ptr<Bitmap>                 m_ArrowRightBitmap;

    String                      m_Title;

    bool                        m_IsOpen            = false;
    bool                        m_HasOpenChildren   = false;
};

} // namespace os
