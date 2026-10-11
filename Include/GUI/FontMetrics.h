// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <GUI/Font.h>
#include <GUI/FontData.h>

class PFontMetrics
{
public:
    static constexpr int CHARACTER_SPACING = 3;
    static float GetFontHeight(PFontID fontID);
    static float GetStringWidth(PFontID fontID, const char* string, size_t length);
    static size_t GetStringLength(PFontID fontID, const char* string, size_t length, float width, bool includeLast);
    static const FONT_INFO* GetFontDesc(PFontID fontID);
};
