// This file is part of PadOS.
//
// Copyright (C) 2020-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 09.02.2020 21:20

#include <array>
#include <algorithm>

#include <GUI/NamedColors.h>
#include <Utils/Utils.h>
#include <GUI/Color.h>
#include <GUI/StandardColorsDefinition.h>
#include <GUI/View.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor PColor::FromColorID(PNamedColors colorID)
{
    auto i = std::lower_bound(PStandardColorsTable.begin(), PStandardColorsTable.end(), colorID, [](const PNamedColorNode& lhs, PNamedColors rhs) { return lhs.NameID < rhs; });
    if (i != PStandardColorsTable.end() && i->NameID == colorID) {
        return i->ColorValue;
    }
    return PColor(0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor::PColor(PNamedColors colorID)
{
    m_Color = FromColorID(colorID).m_Color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor::PColor(const PString& name)
{
    m_Color = FromColorName(name).m_Color;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PColor& PColor::operator*=(float rhs)
{
    *this = (*this) * rhs;
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDynamicColor::PDynamicColor(PStandardColorID colorID) : PColor(pget_standard_color(colorID))
{
}
