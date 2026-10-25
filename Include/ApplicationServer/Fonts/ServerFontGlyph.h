// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <ft2build.h>
#include FT_FREETYPE_H

#include <DeviceControl/DisplayFont.h>
#include <Ptr/Ptr.h>
#include <Utils/IntrusiveList.h>

class PServerFontGlyphTable;

// One complete glyph record and its coverage share an allocation. The display
// base is the same record read by the kernel; no character-specific copy exists.
class PServerFontGlyph : public PtrTarget, public PIntrusiveListNode<PServerFontGlyph>, public PDisplayFontGlyph
{
public:
    static Ptr<PServerFontGlyph> Create(PServerFontGlyphTable& owner, FT_GlyphSlot glyph, bool monochrome);
    ~PServerFontGlyph();
    static void operator delete(void* storage) noexcept;

    PServerFontGlyphTable& GetOwner() const noexcept { return *m_Owner; }
    size_t GetRasterSize() const noexcept { return m_RasterSize; }
    size_t GetMemorySize() const noexcept;
    PDisplayFontCharacter& AddCharacter(uint32_t character);
    bool IsPinned() const noexcept { return m_Pinned; }
    void SetPinned(bool pinned) noexcept { m_Pinned = pinned; }
    Ptr<PServerFontGlyph>& GetNextCachedGlyph() noexcept { return m_NextCachedGlyph; }
    Ptr<PServerFontGlyph>& GetNextPinnedGlyph() noexcept { return m_NextPinnedGlyph; }

private:
    PServerFontGlyph(PServerFontGlyphTable& owner, FT_GlyphSlot glyph, size_t rasterSize, bool monochrome);
    void CopyRaster(const FT_Bitmap& bitmap, bool monochrome);

    static constexpr size_t MAX_RASTER_BYTES = 4 * 1024 * 1024;

    size_t m_CharacterCapacity = INLINE_CHARACTER_COUNT;
    size_t m_RasterSize;
    PServerFontGlyphTable* m_Owner;
    Ptr<PServerFontGlyph> m_NextCachedGlyph;
    Ptr<PServerFontGlyph> m_NextPinnedGlyph;
    bool m_Pinned = false;
};
