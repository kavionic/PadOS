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

#include "GUI/ButtonBase.h"

namespace os
{

///////////////////////////////////////////////////////////////////////////////
/// 2-state check box.
/// \ingroup gui
/// \par Description:
///
/// \sa
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class CheckBox : public ButtonBase
{
public:
    CheckBox(const String& name, Ptr<View> parent = nullptr, uint32_t flags = ViewFlags::WillDraw | ViewFlags::ClearBackground);
    CheckBox(ViewFactoryContext* context, Ptr<View> parent, const pugi::xml_node& xmlData);
    ~CheckBox();

      // From View:
    virtual void CalculatePreferredSize(Point* minSize, Point* maxSize, bool includeWidth, bool includeHeight) const override;
    virtual void Paint(const Rect& updateRect) override;

    // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) override;

private:
};

}
