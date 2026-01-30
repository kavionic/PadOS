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

#include <GUI/Widgets/ButtonBase.h>


///////////////////////////////////////////////////////////////////////////////
/// 2-state check box.
/// \ingroup gui
/// \par Description:
///
/// \sa
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

class PRadioButton : public PButtonBase
{
public:
    PRadioButton(const PString& name = PString::zero, Ptr<PView> parent = nullptr, uint32_t flags = PViewFlags::WillDraw | PViewFlags::ClearBackground);
    PRadioButton(PViewFactoryContext& context, Ptr<PView> parent, const pugi::xml_node& xmlData);
    ~PRadioButton();

      // From View:
    virtual void CalculatePreferredSize(PPoint* minSize, PPoint* maxSize, bool includeWidth, bool includeHeight) override;
    virtual void OnPaint(const PRect& updateRect) override;

    // From Control:
    virtual void OnEnableStatusChanged(bool isEnabled) override;

private:
};
