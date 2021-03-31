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

#include <Ptr/PtrTarget.h>
#include <Utils/String.h>
#include <Math/Rect.h>
#include <Signals/Signal.h>

namespace os
{
class View;
class Menu;

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class MenuItem : public PtrTarget
{
public:
    MenuItem(const String& label, int id = 0);
    MenuItem(Ptr<Menu> menu);
    ~MenuItem();

    int             GetID() const { return m_ID; }
    Ptr<Menu>       GetSubMenu() const;
    Ptr<Menu>       GetSuperMenu() const;
    Rect            GetFrame() const;
    virtual Point   GetContentSize();
    String          GetLabel() const;
    virtual void    Draw(Ptr<View> targetView);
    virtual void    DrawContent(Ptr<View> targetView);
    virtual void    Highlight(bool highlight);
    Point           GetContentLocation() const;
    
    Signal<void, Ptr<MenuItem>> SignalClicked;
private:
    friend class Menu;
    friend class MenuRenderView;

    Menu*       m_SuperMenu;
    Ptr<Menu>   m_SubMenu;
    Rect        m_Frame;

    int         m_ID = 0;
    String      m_Label;

    bool        m_IsHighlighted;
};

} // namespace os
