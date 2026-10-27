// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <Math/Rect.h>

inline constexpr float FONT_FLOAT_TO_F26DOT6 = 64.0f;
inline constexpr float FONT_F26DOT6_TO_FLOAT = 1.0f / FONT_FLOAT_TO_F26DOT6;

enum class PDisplayFontDirection : uint8_t
{
    Right,
    Down,
    Left,
    Up
};

struct PDisplayFontGlyph;

// Links point to glyphs, so moving an alias array does not invalidate a chain.
struct PDisplayFontCharacter
{
    uint32_t Character;
    const PDisplayFontGlyph* NextGlyph;
};

struct PDisplayFontGlyph
{
    static constexpr size_t INLINE_CHARACTER_COUNT = 1;

    union CharacterStorage
    {
        PDisplayFontCharacter Inline[INLINE_CHARACTER_COUNT]{};
        PDisplayFontCharacter* Heap;
    };

    std::span<PDisplayFontCharacter> GetCharacters() noexcept
    {
        PDisplayFontCharacter* entries = (CharacterCount > INLINE_CHARACTER_COUNT) ? Characters.Heap : Characters.Inline;
        return {entries, CharacterCount};
    }

    std::span<const PDisplayFontCharacter> GetCharacters() const noexcept
    {
        const PDisplayFontCharacter* entries = (CharacterCount > INLINE_CHARACTER_COUNT) ? Characters.Heap : Characters.Inline;
        return {entries, CharacterCount};
    }

    PIRect Bounds;
    PPoint Advance;
    float Width = 0.0f;
    uint32_t GlyphIndex = 0;
    const uint8_t* Raster = nullptr;
    CharacterStorage Characters;
    size_t CharacterCount = 0;
};

// The appserver owns this view. Pending submissions retain the font and pin
// every raster used by their strings. Tables may grow between submissions;
// the view and all referenced data remain unchanged during synchronous Submit.
// Non-ASCII buckets hold glyph chains.
// Glyph records and rasters are shared directly with the appserver's cache.
class PDisplayFontCache
{
public:
    static constexpr size_t ASCII_COUNT = 128;

    // Prepared text uses glyph zero on lookup misses; it has no character aliases.
    const PDisplayFontGlyph* GetMissingGlyph() const noexcept { return m_MissingGlyph; }
    const PDisplayFontGlyph* const* GetCharacters() const noexcept { return m_Characters; }
    size_t GetCharacterBucketCount() const noexcept { return m_CharacterBucketCount; }
    PPoint GetAscenderOffset() const noexcept { return m_AscenderOffset; }
    PPoint GetDescenderOffset() const noexcept { return m_DescenderOffset; }
    PDisplayFontDirection GetDirection() const noexcept { return m_Direction; }
    bool HasKerning() const noexcept { return m_HasKerning; }
    PPoint GetBaselineVector() const noexcept
    {
        return PPoint(float(m_KerningScaleX), float(m_KerningScaleY)) * (1.0f / 65536.0f);
    }
    // Dot a screen displacement with this vector to obtain its untransformed baseline coordinate.
    PPoint GetWidthProjection() const noexcept
    {
        const PPoint height = m_DescenderOffset - m_AscenderOffset;
        if (m_KerningScaleX == 65536 && m_KerningScaleY == 0 && height.x == 0.0f) {
            return PPoint(1.0f, 0.0f);
        }
        const PPoint baseline = GetBaselineVector();
        const float determinant = baseline.x * height.y - baseline.y * height.x;
        if (determinant == 0.0f) {
            return baseline;
        }
        return PPoint(height.y, -height.x) * (1.0f / determinant);
    }

    static size_t GetBucketIndex(uint32_t key, size_t bucketCount) noexcept
    {
        key ^= key >> 16;
        key *= 0x85ebca6bu;
        key ^= key >> 13;
        return size_t(key) & (bucketCount - 1);
    }

    const PDisplayFontGlyph* FindGlyph(uint32_t character) const noexcept
    {
        if (character < m_AsciiGlyphs.size()) {
            return m_AsciiGlyphs[character];
        }
        if (m_CharacterBucketCount != 0)
        {
            const size_t index = GetBucketIndex(character, m_CharacterBucketCount);
            const PDisplayFontGlyph* glyph = m_Characters[index];
            while (glyph != nullptr)
            {
                if (glyph->CharacterCount == 1)
                {
                    const PDisplayFontCharacter& entry = glyph->Characters.Inline[0];
                    if (entry.Character == character) {
                        return glyph;
                    }
                    glyph = entry.NextGlyph;
                }
                else
                {
                    const PDisplayFontCharacter* link = nullptr;
                    for (const PDisplayFontCharacter& entry : glyph->GetCharacters())
                    {
                        if (entry.Character == character) {
                            return glyph;
                        }
                        // Only the first non-ASCII alias in this bucket owns its link.
                        if (link == nullptr && entry.Character >= ASCII_COUNT
                            && GetBucketIndex(entry.Character, m_CharacterBucketCount) == index) {
                            link = &entry;
                        }
                    }
                    glyph = (link != nullptr) ? link->NextGlyph : nullptr;
                }
            }
        }
        return nullptr;
    }

    // Input is signed 26.6 pixels; the baseline coefficients are signed 16.16.
    // Match FreeType's rounding, including negative half-way values.
    PPoint TransformKerning(int32_t advance) const noexcept
    {
        if (advance == 0) {
            return PPoint();
        }
        if (m_KerningScaleX == 65536 && m_KerningScaleY == 0) {
            return PPoint(float(advance) * FONT_F26DOT6_TO_FLOAT, 0.0f);
        }
        const auto transform = [advance](int32_t scale)
        {
            const int64_t product = int64_t(advance) * scale;
            return float((product + 0x8000 + (product >> 63)) >> 16) * FONT_F26DOT6_TO_FLOAT;
        };
        return PPoint(transform(m_KerningScaleX), transform(m_KerningScaleY));
    }

protected:
    std::array<const PDisplayFontGlyph*, ASCII_COUNT> m_AsciiGlyphs{};
    const PDisplayFontGlyph* m_MissingGlyph = nullptr;
    const PDisplayFontGlyph* const* m_Characters = nullptr;
    size_t m_CharacterBucketCount = 0;
    PPoint m_AscenderOffset;
    PPoint m_DescenderOffset;
    int32_t m_KerningScaleX = 65536;
    int32_t m_KerningScaleY = 0;
    PDisplayFontDirection m_Direction = PDisplayFontDirection::Right;
    bool m_HasKerning = false;
};
