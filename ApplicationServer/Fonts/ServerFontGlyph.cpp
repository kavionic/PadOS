// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/ServerFontGlyph.h>

#include <cassert>
#include <cstring>
#include <limits>
#include <new>

#include <ApplicationServer/Fonts/ServerFontGlyphTable.h>
#include <System/ExceptionHandling.h>

#include <ApplicationServer/Fonts/FontGlyphCache.h>

Ptr<PServerFontGlyph> PServerFontGlyph::Create(PServerFontGlyphTable& owner, FT_GlyphSlot glyph, bool monochrome)
{
    size_t rasterSize = 0;
    const FT_Bitmap& bitmap = glyph->bitmap;
    if (bitmap.width != 0 && bitmap.rows != 0)
    {
        if (bitmap.width > MAX_RASTER_BYTES / bitmap.rows || bitmap.buffer == nullptr) {
            PERROR_THROW_CODE(PErrorCode::FBIG);
        }
        rasterSize = size_t(bitmap.width) * bitmap.rows;
    }
    void* storage = ::operator new(sizeof(PServerFontGlyph) + rasterSize);
    PScopeFail releaseStorage([storage] { ::operator delete(storage); });
    return ptr_new_cast(::new(storage) PServerFontGlyph(owner, glyph, rasterSize, monochrome));
}

PServerFontGlyph::~PServerFontGlyph()
{
    if (m_CharacterCapacity > INLINE_CHARACTER_COUNT) {
        delete[] Characters.Heap;
    }
}

void PServerFontGlyph::operator delete(void* storage) noexcept
{
    ::operator delete(storage);
}

size_t PServerFontGlyph::GetMemorySize() const noexcept
{
    size_t size = PFontGlyphCache::GetAllocationMemorySize(sizeof(PServerFontGlyph) + m_RasterSize);
    if (m_CharacterCapacity > INLINE_CHARACTER_COUNT) {
        size += PFontGlyphCache::GetAllocationMemorySize(m_CharacterCapacity * sizeof(PDisplayFontCharacter));
    }
    return size;
}

PDisplayFontCharacter& PServerFontGlyph::AddCharacter(uint32_t character)
{
    assert(IsListMember() || IsPinned());
    if (CharacterCount == m_CharacterCapacity)
    {
        constexpr size_t MAX_CAPACITY = std::numeric_limits<size_t>::max() / sizeof(PDisplayFontCharacter);
        if (m_CharacterCapacity > MAX_CAPACITY / 2) {
            PERROR_THROW_CODE(PErrorCode::FBIG);
        }
        const size_t previousSize = GetMemorySize();
        const size_t capacity = m_CharacterCapacity * 2;
        PDisplayFontCharacter* entries = new PDisplayFontCharacter[capacity];
        std::memcpy(entries, GetCharacters().data(), CharacterCount * sizeof(PDisplayFontCharacter));
        if (m_CharacterCapacity > INLINE_CHARACTER_COUNT) {
            delete[] Characters.Heap;
        }
        Characters.Heap = entries;
        m_CharacterCapacity = capacity;
        m_Owner->GetCache().UpdateMemoryUsage(previousSize, GetMemorySize());
    }
    PDisplayFontCharacter* entries = (m_CharacterCapacity > INLINE_CHARACTER_COUNT) ? Characters.Heap : Characters.Inline;
    PDisplayFontCharacter& entry = entries[CharacterCount++];
    entry = {character, nullptr};
    return entry;
}

PServerFontGlyph::PServerFontGlyph(PServerFontGlyphTable& owner, FT_GlyphSlot glyph, size_t rasterSize, bool monochrome)
    : m_RasterSize(rasterSize)
    , m_Owner(&owner)
{
    GlyphIndex = glyph->glyph_index;
    Advance = PPoint(float(glyph->advance.x) * FONT_F26DOT6_TO_FLOAT, float(-glyph->advance.y) * FONT_F26DOT6_TO_FLOAT);
    Width = float(glyph->metrics.horiAdvance) * FONT_F26DOT6_TO_FLOAT;
    if (rasterSize != 0)
    {
        Bounds = PIRect(
            glyph->bitmap_left,
            -glyph->bitmap_top,
            glyph->bitmap_left + int(glyph->bitmap.width),
            -glyph->bitmap_top + int(glyph->bitmap.rows));
        Raster = reinterpret_cast<const uint8_t*>(this + 1);
        CopyRaster(glyph->bitmap, monochrome);
    }
}

void PServerFontGlyph::CopyRaster(const FT_Bitmap& bitmap, bool monochrome)
{
    uint8_t* raster = reinterpret_cast<uint8_t*>(this + 1);
    const uint8_t* firstRow = bitmap.buffer;
    if (bitmap.pitch < 0) {
        firstRow -= ptrdiff_t(bitmap.pitch) * (bitmap.rows - 1);
    }
    for (size_t row = 0; row < bitmap.rows; ++row)
    {
        const uint8_t* source = firstRow + ptrdiff_t(row) * bitmap.pitch;
        uint8_t* destination = raster + row * bitmap.width;
        switch (bitmap.pixel_mode)
        {
            case FT_PIXEL_MODE_GRAY:
                if (bitmap.num_grays == 256)
                {
                    memcpy(destination, source, bitmap.width);
                }
                else
                {
                    if (bitmap.num_grays < 2) {
                        PERROR_THROW_CODE(PErrorCode::INVAL);
                    }
                    for (size_t column = 0; column < bitmap.width; ++column) {
                        destination[column] = uint8_t(uint32_t(source[column]) * 255 / (bitmap.num_grays - 1));
                    }
                }
                break;
            case FT_PIXEL_MODE_MONO:
                for (size_t column = 0; column < bitmap.width; ++column) {
                    destination[column] = ((source[column / 8] & (0x80 >> (column % 8))) != 0) ? 255 : 0;
                }
                break;
            case FT_PIXEL_MODE_GRAY2:
                for (size_t column = 0; column < bitmap.width; ++column) {
                    destination[column] = uint8_t(((source[column / 4] >> (6 - 2 * (column % 4))) & 3) * 85);
                }
                break;
            case FT_PIXEL_MODE_GRAY4:
                for (size_t column = 0; column < bitmap.width; ++column) {
                    destination[column] = uint8_t(((source[column / 2] >> (4 - 4 * (column % 2))) & 15) * 17);
                }
                break;
            default:
                PERROR_THROW_CODE(PErrorCode::INVAL);
        }
        if (monochrome && bitmap.pixel_mode != FT_PIXEL_MODE_MONO)
        {
            for (size_t column = 0; column < bitmap.width; ++column) {
                destination[column] = (destination[column] >= 128) ? 255 : 0;
            }
        }
    }
}
