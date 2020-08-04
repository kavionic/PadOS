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

#pragma once

#include "Ptr/PtrTarget.h"
#include "Kernel/Drivers/RA8875Driver/GfxDriver.h"

namespace os
{

struct FontHeight
{
    float ascender;   // Pixels from baseline to top of glyph (positive)
    float descender;  // Pixels from baseline to bottom of glyph (negative)
    float line_gap;   // Space between lines (positive)    
};

class Font : public PtrTarget
{
public:
    Font(kernel::GfxDriver::Font_e font) : m_Font(font) {}

    void Set(kernel::GfxDriver::Font_e font) { m_Font = font; }
    kernel::GfxDriver::Font_e Get() const { return m_Font; }
    FontHeight GetHeight() const;        

    int		GetStringLength(const char* pzString, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const char* pzString, int nLength, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const std::string& cString, float vWidth, bool bIncludeLast = false) const;
    void	GetStringLengths(const char** stringArray, const int* lengthArray, int stringCount, float width, int* maxLengthArray, bool includeLast = false) const;

    float GetStringWidth(const char* string, size_t length) const;
    
private:
    kernel::GfxDriver::Font_e m_Font;
};

} // namespace
