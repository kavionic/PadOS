// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <string>

#include <GUI/FontDescription.h>
#include <Ptr/Ptr.h>


class PFont : public PtrTarget
{
public:
    // Standard font IDs select the closest bundled TrueType equivalent, also when passed to Set().
    explicit PFont(PFontID font);
    explicit PFont(const PFontDescription& description);
    ~PFont();

    // After modifying a font, call SetFont again on views using it.
    void Set(PFontID font);
    void Set(const PFontDescription& description);
    PFontID Get() const { return m_Font; } // e_FontCount for a FreeType font.
    bool IsFreeType() const { return m_FreeType != nullptr; }
    const PFontDescription& GetDescription() const;
    PFontHeight GetHeight() const;

    int		GetStringLength(const char* pzString, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const char* pzString, int nLength, float vWidth, bool bIncludeLast = false) const;
    int		GetStringLength(const std::string& cString, float vWidth, bool bIncludeLast = false) const;
    void	GetStringLengths(const char** stringArray, const int* lengthArray, int stringCount, float width, int* maxLengthArray, bool includeLast = false) const;

    float GetStringWidth(const char* string, size_t length) const;
    
private:
    static PFontDescription GetDefaultDescription(PFontID font);

    class FreeTypeState;
    Ptr<FreeTypeState> m_FreeType;
    PFontID m_Font;
};
