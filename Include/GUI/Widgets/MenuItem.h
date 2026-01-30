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

class PView;
class PMenu;

/** 
 * \ingroup gui
 * \par Description:
 *
 * \sa
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PMenuItem : public PtrTarget
{
public:
    PMenuItem(const PString& label, int id = 0);
    PMenuItem(Ptr<PMenu> menu);
    ~PMenuItem();

    int             GetID() const { return m_ID; }
    Ptr<PMenu>       GetSubMenu() const;
    Ptr<PMenu>       GetSuperMenu() const;
    PRect            GetFrame() const;
    virtual PPoint   GetContentSize();
    PString         GetLabel() const;
    virtual void    Draw(Ptr<PView> targetView);
    virtual void    DrawContent(Ptr<PView> targetView);
    virtual void    Highlight(bool highlight);
    PPoint           GetContentLocation() const;
    
    Signal<void, Ptr<PMenuItem>> SignalClicked;
private:
    friend class PMenu;
    friend class PMenuRenderView;

    PMenu*       m_SuperMenu;
    Ptr<PMenu>   m_SubMenu;
    PRect        m_Frame;

    int         m_ID = 0;
    PString     m_Label;

    bool        m_IsHighlighted;
};
