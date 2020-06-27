// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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

#include "System/Platform.h"

#include "GUI/Font.h"

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FontHeight Font::GetHeight() const
{
    FontHeight height;
    height.ascender = 0.0f;
    height.descender = kernel::GfxDriver::Instance.GetFontHeight(m_Font);
    height.line_gap = ceil((height.descender - height.ascender) / 10.0f);
    return height;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float Font::GetStringWidth(const char* string, size_t length) const
{
    return kernel::GfxDriver::Instance.GetStringWidth(m_Font, string, length);
}
