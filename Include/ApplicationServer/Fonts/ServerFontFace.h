// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <Ptr/Ptr.h>
#include <Utils/String.h>

class PServerFontLibrary;

class PServerFontFace : public PtrTarget
{
public:
    PServerFontFace(Ptr<PServerFontLibrary> library, const PString& path, FT_Long faceIndex);
    ~PServerFontFace();

    FT_Face GetHandle() const { return m_Face; }

private:
    Ptr<PServerFontLibrary> m_Library;
    FT_Face m_Face = nullptr;
};
