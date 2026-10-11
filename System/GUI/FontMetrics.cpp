// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <GUI/FontMetrics.h>
#include <GUI/Fonts/SansSerif_14.h>
#include <GUI/Fonts/SansSerif_20.h>
#include <GUI/Fonts/SansSerif_72.h>
#include <Utils/UTF8Utils.h>

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PFontMetrics::GetFontHeight(PFontID fontID)
{
    const FONT_INFO* font = GetFontDesc(fontID);
    return (font != nullptr) ? float(font->heightPages) : 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

float PFontMetrics::GetStringWidth(PFontID fontID, const char* string, size_t length)
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font != nullptr)
    {
        float width = 0.0f;

        while (length > 0)
        {
            int charLen = utf8_char_length(*string);
            if (charLen > length) {
                break;
            }
            uint32_t character = utf8_to_unicode(string);
            string += charLen;
            length -= charLen;

            if (character < font->startChar || character > font->endChar) continue;

            const FONT_CHAR_INFO* charInfo = font->charInfo + character - font->startChar;
            width += float(charInfo->widthBits);
            if (length != 0) {
                width += float(CHARACTER_SPACING);
            }
        }
        return width;
    }
    return 0.0f;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PFontMetrics::GetStringLength(PFontID fontID, const char* string, size_t length, float width, bool includeLast)
{
    const FONT_INFO* font = GetFontDesc(fontID);

    if (font == nullptr) {
        return 0;
    }
    size_t strLen = 0;

    while (length > 0)
    {
        int charLen = utf8_char_length(*string);
        if (charLen > length) {
            break;
        }
        uint32_t character = utf8_to_unicode(string);
        
        float advance = 0.0f;
        if (character >= font->startChar && character <= font->endChar)
        {
            const FONT_CHAR_INFO* charInfo = font->charInfo + character - font->startChar;
            advance = float(charInfo->widthBits + CHARACTER_SPACING);
        }
        if (width < advance)
        {
            if (includeLast) {
                strLen += charLen;
            }
            break;
        }
        string += charLen;
        length -= charLen;
        strLen += charLen;
        width -= advance;
    }
    return strLen;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const FONT_INFO* PFontMetrics::GetFontDesc(PFontID fontID)
{
    switch (fontID)
    {
        case PFontID::e_FontSmall:
        case PFontID::e_FontNormal:
            return &sansSerif_14ptFontInfo;
        case PFontID::e_FontLarge:
            return &sansSerif_20ptFontInfo;
        case PFontID::e_Font7Seg:
            return &sansSerif_72ptFontInfo;
        case PFontID::e_FontCount:
            return nullptr;
    }
    return nullptr;
}
