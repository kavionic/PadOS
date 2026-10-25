// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <ApplicationServer/Fonts/ServerFontGlyphTable.h>

#include <algorithm>
#include <cassert>
#include <utility>

#include <System/ExceptionHandling.h>

#include <ApplicationServer/Fonts/FontGlyphCache.h>

PServerFontGlyphTable::PServerFontGlyphTable(Ptr<PFontGlyphCache> cache)
    : m_Cache(std::move(cache))
{
    m_Cache->UpdateMemoryUsage(0, GetMemorySize());
}

PServerFontGlyphTable::~PServerFontGlyphTable()
{
    for (Ptr<PServerFontGlyph>& bucket : m_GlyphBuckets)
    {
        while (bucket != nullptr)
        {
            Ptr<PServerFontGlyph> glyph = std::move(bucket);
            bucket = std::move(glyph->GetNextCachedGlyph());
            m_Cache->RemoveGlyph(*glyph);
        }
    }
    m_Cache->UpdateMemoryUsage(GetMemorySize(), 0);
}

size_t PServerFontGlyphTable::GetMemorySize() const noexcept
{
    // The table itself includes the fixed ASCII index and is embedded in its font.
    return sizeof(PServerFontGlyphTable)
        + PFontGlyphCache::GetAllocationMemorySize(m_GlyphBuckets.capacity() * sizeof(Ptr<PServerFontGlyph>))
        + PFontGlyphCache::GetAllocationMemorySize(m_CharacterBuckets.capacity() * sizeof(const PDisplayFontGlyph*));
}

PServerFontGlyph* PServerFontGlyphTable::FindCharacter(uint32_t character) const noexcept
{
    return const_cast<PServerFontGlyph*>(static_cast<const PServerFontGlyph*>(FindGlyph(character)));
}

PServerFontGlyph* PServerFontGlyphTable::FindGlyphIndex(uint32_t glyphIndex) const noexcept
{
    if (!m_GlyphBuckets.empty())
    {
        PServerFontGlyph* glyph = ptr_raw_pointer_cast(m_GlyphBuckets[GetBucketIndex(glyphIndex, m_GlyphBuckets.size())]);
        while (glyph != nullptr)
        {
            if (glyph->GlyphIndex == glyphIndex) {
                return glyph;
            }
            glyph = ptr_raw_pointer_cast(glyph->GetNextCachedGlyph());
        }
    }
    return nullptr;
}

void PServerFontGlyphTable::StoreGlyph(Ptr<PServerFontGlyph> glyph)
{
    assert(glyph != nullptr && &glyph->GetOwner() == this);
    assert(!glyph->IsListMember() && !glyph->IsPinned());
    assert(glyph->GetNextCachedGlyph() == nullptr);
    assert(FindGlyphIndex(glyph->GlyphIndex) == nullptr);
    if (m_GlyphCount == m_GlyphBuckets.size()) {
        GrowGlyphIndex();
    }
    const size_t index = GetBucketIndex(glyph->GlyphIndex, m_GlyphBuckets.size());
    m_Cache->RegisterGlyph(*glyph);
    glyph->GetNextCachedGlyph() = std::move(m_GlyphBuckets[index]);
    m_GlyphBuckets[index] = std::move(glyph);
    ++m_GlyphCount;
}

void PServerFontGlyphTable::AddCharacter(PServerFontGlyph& glyph, uint32_t character)
{
    assert(&glyph.GetOwner() == this);
    assert(FindCharacter(character) == nullptr);
    if (glyph.GlyphIndex == 0)
    {
        m_MissingGlyph = &glyph;
        return;
    }
    const bool ascii = character < m_AsciiGlyphs.size();
    if (!ascii && m_CharacterCount == m_CharacterBuckets.size() / 2) {
        GrowCharacterIndex();
    }
    const size_t index = ascii ? 0 : GetBucketIndex(character, m_CharacterBuckets.size());
    const bool linked = !ascii && FindCharacterBucket(glyph, index) != nullptr;
    // Finish all allocations before publishing the alias in either index.
    PDisplayFontCharacter& entry = glyph.AddCharacter(character);
    if (ascii)
    {
        m_AsciiGlyphs[character] = &glyph;
    }
    else
    {
        if (!linked)
        {
            entry.NextGlyph = m_CharacterBuckets[index];
            m_CharacterBuckets[index] = &glyph;
        }
        ++m_CharacterCount;
    }
}

void PServerFontGlyphTable::EraseGlyph(PServerFontGlyph& glyph) noexcept
{
    assert(&glyph.GetOwner() == this);
    m_Cache->RemoveGlyph(glyph);
    if (m_MissingGlyph == &glyph) {
        m_MissingGlyph = nullptr;
    }
    for (PDisplayFontCharacter& entry : glyph.GetCharacters()) {
        RemoveCharacter(glyph, entry);
    }
    Ptr<PServerFontGlyph>* entry = &m_GlyphBuckets[GetBucketIndex(glyph.GlyphIndex, m_GlyphBuckets.size())];
    while (*entry != &glyph)
    {
        assert(*entry != nullptr);
        entry = &(*entry)->GetNextCachedGlyph();
    }
    // Break the owning hash chain before releasing the removed glyph.
    Ptr<PServerFontGlyph> removed = std::move(*entry);
    *entry = std::move(removed->GetNextCachedGlyph());
    --m_GlyphCount;
    ReleaseEmptyIndexes();
}

void PServerFontGlyphTable::ReleaseEmptyIndexes() noexcept
{
    if (m_GlyphCount == 0)
    {
        const size_t size = PFontGlyphCache::GetAllocationMemorySize(m_GlyphBuckets.capacity() * sizeof(Ptr<PServerFontGlyph>));
        std::vector<Ptr<PServerFontGlyph>>().swap(m_GlyphBuckets);
        m_Cache->UpdateMemoryUsage(size, 0);
    }
    if (m_CharacterCount == 0)
    {
        const size_t size = PFontGlyphCache::GetAllocationMemorySize(
            m_CharacterBuckets.capacity() * sizeof(const PDisplayFontGlyph*));
        std::vector<const PDisplayFontGlyph*>().swap(m_CharacterBuckets);
        m_Characters = nullptr;
        m_CharacterBucketCount = 0;
        m_Cache->UpdateMemoryUsage(size, 0);
    }
}

void PServerFontGlyphTable::GrowGlyphIndex()
{
    if (m_GlyphBuckets.size() > m_GlyphBuckets.max_size() / 2) {
        PERROR_THROW_CODE(PErrorCode::FBIG);
    }
    const size_t previousSize = GetMemorySize();
    std::vector<Ptr<PServerFontGlyph>> buckets(std::max(INITIAL_BUCKET_COUNT, m_GlyphBuckets.size() * 2));
    for (Ptr<PServerFontGlyph>& entry : m_GlyphBuckets)
    {
        while (entry != nullptr)
        {
            Ptr<PServerFontGlyph> glyph = std::move(entry);
            entry = std::move(glyph->GetNextCachedGlyph());
            const size_t index = GetBucketIndex(glyph->GlyphIndex, buckets.size());
            glyph->GetNextCachedGlyph() = std::move(buckets[index]);
            buckets[index] = std::move(glyph);
        }
    }
    m_GlyphBuckets.swap(buckets);
    m_Cache->UpdateMemoryUsage(previousSize, GetMemorySize());
}

void PServerFontGlyphTable::GrowCharacterIndex()
{
    if (m_CharacterBuckets.size() > m_CharacterBuckets.max_size() / 2) {
        PERROR_THROW_CODE(PErrorCode::FBIG);
    }
    const size_t previousSize = GetMemorySize();
    std::vector<const PDisplayFontGlyph*> buckets(std::max(INITIAL_BUCKET_COUNT, m_CharacterBuckets.size() * 2));
    for (const Ptr<PServerFontGlyph>& bucket : m_GlyphBuckets)
    {
        PServerFontGlyph* glyph = ptr_raw_pointer_cast(bucket);
        while (glyph != nullptr)
        {
            for (PDisplayFontCharacter& entry : glyph->GetCharacters())
            {
                entry.NextGlyph = nullptr;
                if (entry.Character >= ASCII_COUNT)
                {
                    const size_t index = GetBucketIndex(entry.Character, buckets.size());
                    // Process each glyph's aliases together so repeated buckets have this head.
                    if (buckets[index] != glyph)
                    {
                        entry.NextGlyph = buckets[index];
                        buckets[index] = glyph;
                    }
                }
            }
            glyph = ptr_raw_pointer_cast(glyph->GetNextCachedGlyph());
        }
    }
    m_CharacterBuckets.swap(buckets);
    m_Characters = m_CharacterBuckets.data();
    m_CharacterBucketCount = m_CharacterBuckets.size();
    m_Cache->UpdateMemoryUsage(previousSize, GetMemorySize());
}

PDisplayFontCharacter* PServerFontGlyphTable::FindCharacterBucket(PDisplayFontGlyph& glyph, size_t index) const noexcept
{
    for (PDisplayFontCharacter& entry : glyph.GetCharacters())
    {
        if (entry.Character >= ASCII_COUNT && GetBucketIndex(entry.Character, m_CharacterBuckets.size()) == index) {
            return &entry;
        }
    }
    return nullptr;
}

void PServerFontGlyphTable::RemoveCharacter(PServerFontGlyph& glyph, PDisplayFontCharacter& entry) noexcept
{
    if (entry.Character < m_AsciiGlyphs.size())
    {
        m_AsciiGlyphs[entry.Character] = nullptr;
    }
    else
    {
        const size_t index = GetBucketIndex(entry.Character, m_CharacterBuckets.size());
        const PDisplayFontGlyph** link = &m_CharacterBuckets[index];
        while (*link != nullptr && *link != &glyph)
        {
            PDisplayFontCharacter* previous = FindCharacterBucket(const_cast<PDisplayFontGlyph&>(**link), index);
            assert(previous != nullptr);
            link = &previous->NextGlyph;
        }
        // The first alias unlinks this glyph; later aliases in the same bucket find it absent.
        if (*link != nullptr) {
            *link = entry.NextGlyph;
        }
        --m_CharacterCount;
    }
    entry.NextGlyph = nullptr;
}
