// This file is part of PadOS.
//
// Copyright (c) 1999-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/View.h>
#include <GUI/ViewScroller.h>


class PMenu;
class PMenuItem;
class PMenuRenderView;


namespace PMenuFlags
{
static constexpr uint32_t NoKeyboardFocus = 0x01 << PViewFlags::FirstUserBit;

extern const std::map<PString, uint32_t> FlagMap;
}


enum class PMenuLayout : uint8_t
{
    Vertical,
    Horizontal,
};

enum class PMenuLocation
{
    Auto,
    Center,
    TopLeft,
    BottomLeft
};


/**
 * \ingroup gui
 * \par Description:
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PMenu : public PView, public PViewScroller
{
public:
    static constexpr int    LEFT_ARROW_WIDTH = 10;
    static constexpr int    LEFT_ARROW_HEIGHT = 19;

    PMenu(const PString& title = PString::zero, PMenuLayout layout = PMenuLayout::Vertical, uint32_t flags = 0);
    ~PMenu();

//  From View:
//    void        WindowActivated( bool bIsActive );

    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;

    virtual void    OnKeyDown(PKeyCodes keyCode, const PString& text, const PKeyEvent& event) override;

    virtual void    OnFrameSized(const PPoint& delta) override;

    virtual void    OnPaint(const PRect& updateRect) override;

//  From Menu:
    PString         GetLabel() const;
    PMenuLayout      GetLayout() const;
    Ptr<PMenuItem>   AddStringItem(const PString& label, int id = 0);
    Ptr<PMenuItem>   AddItem(Ptr<PMenuItem> item);
    Ptr<PMenuItem>   AddItem(Ptr<PMenuItem> item, size_t index);
    Ptr<PMenuItem>   AddSubMenu(Ptr<PMenu> subMenu);
    Ptr<PMenuItem>   AddSubMenu(Ptr<PMenu> subMenu, size_t index);

    Ptr<PMenuItem>   RemoveItem(size_t index);
    bool            RemoveItem(Ptr<PMenuItem> item);
    bool            RemoveItem(Ptr<PMenu> subMenu);

    Ptr<PMenuItem>   GetItemAt(size_t index) const;
    Ptr<PMenuItem>   GetItemAt(PPoint position) const;
    Ptr<PMenu>       GetSubMenuAt(size_t index) const;
    size_t          GetItemCount() const;

    size_t          GetIndexOf(Ptr<PMenuItem> item) const;
    size_t          GetIndexOf(Ptr<PMenu> subMenu) const;

    Ptr<PMenuItem>   FindItem(int id) const;
    Ptr<PMenuItem>   FindItem(const PString& name) const;

    Ptr<PMenuItem>   FindMarked() const;

    Ptr<PMenu>       GetSuperMenu() const;
    Ptr<PMenuItem>   GetSuperItem();
    void            InvalidateLayout();

    void    Open(const PPoint& screenPosition, PMenuLocation relativeLocation);

    Signal<void, Ptr<PMenuItem>, Ptr<PMenu>> SignalItemSelected;
private:
    friend class PMenuItem;
    friend class PMenuRenderView;

    void        StartOpenTimer(TimeValNanos delay);
    void        OpenSelection();
    void        SelectItem(Ptr<PMenuItem> item);
    void        SelectPrev();
    void        SelectNext();
    void        Close(bool closeChilds, bool closeParent, Ptr<PMenuItem> selectedItem);
    void        InvalidateItem(PMenuItem* item);
    Ptr<PBitmap> GetArrowRightBitmap();

    std::vector<Ptr<PMenuItem>>  m_Items;
    PEventTimer                  m_OpenTimer;
    PMenuLayout                  m_Layout;
    PMenuItem*                   m_SuperItem = nullptr;
    PPoint                       m_ContentSize;

    PRect                        m_ItemBorders;

    Ptr<PMenuRenderView>         m_ContentView;
    Ptr<PBitmap>                 m_ArrowRightBitmap;

    PString                     m_Title;

    bool                        m_IsOpen            = false;
    bool                        m_HasOpenChildren   = false;
};
