// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
