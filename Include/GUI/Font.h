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

#include <string>

#include <Ptr/PtrTarget.h>


enum class PFontID : uint8_t { e_FontSmall, e_FontNormal, e_FontLarge, e_Font7Seg, e_FontCount };

struct PFontHeight
{
    float ascender;   // Pixels from baseline to top of glyph (positive)
    float descender;  // Pixels from baseline to bottom of glyph (negative)
    float line_gap;   // Space between lines (positive)    
};

class PFont : public PtrTarget
{
public:
    PFont(PFontID font) : m_Font(font) {}

    void Set(PFontID font) { m_Font = font; }
    PFontID Get() const { return m_Font; }
    PFontHeight GetHeight() const;        

    int		GetStringLength(const char* pzString, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const char* pzString, int nLength, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const std::string& cString, float vWidth, bool bIncludeLast = false) const;
    void	GetStringLengths(const char** stringArray, const int* lengthArray, int stringCount, float width, int* maxLengthArray, bool includeLast = false) const;

    float GetStringWidth(const char* string, size_t length) const;
    
private:
    PFontID m_Font;
};
