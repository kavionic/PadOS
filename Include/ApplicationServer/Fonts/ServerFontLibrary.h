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

// All FreeType operations and cache access belong to the appserver looper.
// Faces retain their library, and instances retain their face and cache.
class PServerFontLibrary : public PtrTarget
{
public:
    PServerFontLibrary();
    ~PServerFontLibrary();

    FT_Library GetHandle() const { return m_Library; }
    static void CheckError(FT_Error error);

private:
    FT_Library m_Library = nullptr;
};
