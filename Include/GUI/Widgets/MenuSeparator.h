// This file is part of PadOS.
//
// Copyright (c) 1999-2020 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Widgets/MenuItem.h>

/** Menu separator item.
 * \ingroup gui
 * \par Description:
 *  A os::MenuSeparator can be inserted to a menu to categorize other
 *  items. The separator will draw an etched line in the menu.
 * \sa os::MenuItem, os::Menu
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class PMenuSeparator : public PMenuItem
{
public:
    PMenuSeparator();
    ~PMenuSeparator();

    virtual PPoint GetContentSize() override;
    virtual void  Draw(Ptr<PView> targetView) override;
    virtual void  DrawContent(Ptr<PView> targetView) override;
    virtual void  Highlight(bool highlight) override;
private:
};
