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

#include <GUI/MenuItem.h>

namespace os
{

/** Menu separator item.
 * \ingroup gui
 * \par Description:
 *  A os::MenuSeparator can be inserted to a menu to categorize other
 *  items. The separator will draw an etched line in the menu.
 * \sa os::MenuItem, os::Menu
 * \author Kurt Skauen (kurt@atheos.cx)
 *****************************************************************************/

class MenuSeparator : public MenuItem
{
public:
    MenuSeparator();
    ~MenuSeparator();

    virtual Point GetContentSize() override;
    virtual void  Draw(Ptr<View> targetView) override;
    virtual void  DrawContent(Ptr<View> targetView) override;
    virtual void  Highlight(bool highlight) override;
private:
};

} // namespace os
