// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <array>
#include <string>
#include <gtest/gtest.h>
#include <ApplicationServer/Fonts/FontManager.h>
#include <ApplicationServer/Fonts/ServerFontGlyphTable.h>
#include <System/ExceptionHandling.h>
#include <Utils/UTF8Utils.h>

#include <ApplicationServer/Fonts/FontGlyphCache.h>

class FontGlyphCacheTest : public ::testing::Test
{
protected:
    Ptr<PServerFontGlyph> MakeGlyph(PServerFontGlyphTable& owner, FT_UInt glyphIndex, bool empty = false)
    {
        FT_GlyphSlotRec slot{};
        slot.glyph_index = glyphIndex;
        slot.advance.x = 256;
        slot.advance.y = -64;
        slot.metrics.horiAdvance = 192;
        slot.bitmap_left = -1;
        slot.bitmap_top = 2;
        if (!empty)
        {
            slot.bitmap.width = 2;
            slot.bitmap.rows = 2;
            slot.bitmap.pitch = 3;
            slot.bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
            slot.bitmap.num_grays = 256;
            slot.bitmap.buffer = m_Raster.data();
        }
        return PServerFontGlyph::Create(owner, &slot, false);
    }

    static size_t GetMemoryBudget(size_t glyphCount, size_t fontCount = 1, size_t rasterSize = 4)
    {
        const size_t buckets = PServerFontGlyphTable::INITIAL_BUCKET_COUNT * sizeof(Ptr<PServerFontGlyph>);
        return fontCount * (sizeof(PServerFontGlyphTable) + PFontGlyphCache::GetAllocationMemorySize(buckets))
            + glyphCount * PFontGlyphCache::GetAllocationMemorySize(sizeof(PServerFontGlyph) + rasterSize);
    }

    static std::vector<uint32_t> FindBucketCharacters(size_t count, size_t bucketCount, size_t index)
    {
        std::vector<uint32_t> characters;
        for (uint32_t character = 128; characters.size() < count; ++character)
        {
            if (PDisplayFontCache::GetBucketIndex(character, bucketCount) == index) {
                characters.push_back(character);
            }
        }
        return characters;
    }

    std::array<uint8_t, 6> m_Raster = {0, 128, 17, 255, 64, 19};
};

TEST_F(FontGlyphCacheTest, CopiesPackedCoverageIntoTheSharedDisplayRecord)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    PServerFontGlyphTable table(cache);
    Ptr<PServerFontGlyph> glyph = MakeGlyph(table, 2);
    m_Raster.fill(42);
    ASSERT_EQ(glyph->GetRasterSize(), 4);
    EXPECT_EQ(glyph->GetMemorySize(), PFontGlyphCache::GetAllocationMemorySize(sizeof(PServerFontGlyph) + 4));
    EXPECT_EQ(glyph->Bounds, PIRect(-1, -2, 1, 0));
    EXPECT_EQ(glyph->Advance, PPoint(4.0f, 1.0f));
    EXPECT_FLOAT_EQ(glyph->Width, 3.0f);
    EXPECT_EQ(glyph->GlyphIndex, 2);
    const uint8_t expected[] = {0, 128, 255, 64};
    for (size_t index = 0; index < std::size(expected); ++index) {
        EXPECT_EQ(glyph->Raster[index], expected[index]);
    }
    table.StoreGlyph(glyph);
    table.AddCharacter(*glyph, 'A');
    EXPECT_EQ(table.FindGlyph('A'), static_cast<const PDisplayFontGlyph*>(ptr_raw_pointer_cast(glyph)));
    EXPECT_EQ(table.FindGlyphIndex(2), glyph);
}

TEST_F(FontGlyphCacheTest, EmptyRenderedGlyphsKeepTheirAdvanceWithoutRasterStorage)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    PServerFontGlyphTable table(cache);
    Ptr<PServerFontGlyph> glyph = MakeGlyph(table, 3, true);
    table.StoreGlyph(glyph);
    table.AddCharacter(*glyph, ' ');
    ASSERT_NE(table.FindGlyph(' '), nullptr);
    EXPECT_EQ(glyph->Raster, nullptr);
    EXPECT_EQ(glyph->GetRasterSize(), 0);
    EXPECT_EQ(glyph->GetMemorySize(), PFontGlyphCache::GetAllocationMemorySize(sizeof(PServerFontGlyph)));
    EXPECT_FALSE(glyph->Bounds.IsValid());
    EXPECT_EQ(table.FindGlyph(' ')->Advance, PPoint(4.0f, 1.0f));
}

TEST_F(FontGlyphCacheTest, GlyphsWithoutRastersStillConsumeTheMemoryBudget)
{
    const size_t budget = GetMemoryBudget(1, 1, 0);
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(budget);
    PServerFontGlyphTable table(cache);
    Ptr<PServerFontGlyph> first = MakeGlyph(table, 1, true);
    Ptr<PServerFontGlyph> second = MakeGlyph(table, 2, true);
    table.StoreGlyph(first);
    table.AddCharacter(*first, ' ');
    EXPECT_EQ(cache->GetMemoryUsage(), budget);
    table.StoreGlyph(second);
    table.AddCharacter(*second, '\t');
    EXPECT_GT(cache->GetMemoryUsage(), budget);
    cache->Trim();
    EXPECT_EQ(table.FindCharacter(' '), nullptr);
    EXPECT_EQ(table.FindCharacter('\t'), second);
    EXPECT_EQ(cache->GetMemoryUsage(), budget);
}

TEST_F(FontGlyphCacheTest, MemoryBudgetAllowsMoreThan4096Glyphs)
{
    constexpr size_t MEMORY_BUDGET = 2 * 1024 * 1024;
    constexpr FT_UInt GLYPH_COUNT = 4097;
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(MEMORY_BUDGET);
    PServerFontGlyphTable table(cache);
    for (FT_UInt glyphIndex = 1; glyphIndex <= GLYPH_COUNT; ++glyphIndex) {
        table.StoreGlyph(MakeGlyph(table, glyphIndex, true));
    }
    ASSERT_LE(cache->GetMemoryUsage(), MEMORY_BUDGET);
    const size_t memoryUsage = cache->GetMemoryUsage();
    cache->Trim();
    EXPECT_EQ(cache->GetMemoryUsage(), memoryUsage);
    for (FT_UInt glyphIndex = 1; glyphIndex <= GLYPH_COUNT; ++glyphIndex) {
        EXPECT_NE(table.FindGlyphIndex(glyphIndex), nullptr);
    }
}

TEST_F(FontGlyphCacheTest, EmptyIndexesReleaseTheirAllocatedMemory)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    EXPECT_EQ(cache->GetMemoryUsage(), 0);
    {
        PServerFontGlyphTable table(cache);
        EXPECT_EQ(cache->GetMemoryUsage(), sizeof(PServerFontGlyphTable));
        Ptr<PServerFontGlyph> ascii = MakeGlyph(table, 1);
        table.StoreGlyph(ascii);
        table.AddCharacter(*ascii, 'A');
        const size_t asciiUsage = cache->GetMemoryUsage();
        Ptr<PServerFontGlyph> unicode = MakeGlyph(table, 2);
        table.StoreGlyph(unicode);
        table.AddCharacter(*unicode, 0x100);
        ASSERT_NE(table.GetCharacters(), nullptr);
        EXPECT_GT(cache->GetMemoryUsage(), asciiUsage + unicode->GetMemorySize());
        table.EraseGlyph(*unicode);
        EXPECT_EQ(table.GetCharacters(), nullptr);
        EXPECT_EQ(table.GetCharacterBucketCount(), 0);
        EXPECT_EQ(cache->GetMemoryUsage(), asciiUsage);
        EXPECT_EQ(table.FindCharacter('A'), ascii);
        table.EraseGlyph(*ascii);
        EXPECT_EQ(table.GetMemorySize(), sizeof(PServerFontGlyphTable));
        EXPECT_EQ(cache->GetMemoryUsage(), sizeof(PServerFontGlyphTable));
    }
    EXPECT_EQ(cache->GetMemoryUsage(), 0);
}

TEST_F(FontGlyphCacheTest, EvictionRemovesInlineAndOverflowAliasesIncludingAscii)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(GetMemoryBudget(1));
    PServerFontGlyphTable table(cache);
    Ptr<PServerFontGlyph> shared = MakeGlyph(table, 2);
    table.StoreGlyph(shared);
    const uint32_t aliases[] = {0, 'A', 0x100, 0x200, 0x300, 0x400, 0x500};
    for (uint32_t character : aliases)
    {
        table.AddCharacter(*shared, character);
        EXPECT_EQ(table.FindCharacter(character), shared);
    }
    ASSERT_EQ(shared->GetCharacters().size(), std::size(aliases));
    for (size_t index = 0; index < std::size(aliases); ++index) {
        EXPECT_EQ(shared->GetCharacters()[index].Character, aliases[index]);
    }
    Ptr<PServerFontGlyph> replacement = MakeGlyph(table, 1);
    table.StoreGlyph(replacement);
    table.AddCharacter(*replacement, 'B');
    cache->Trim();
    for (uint32_t character : aliases) {
        EXPECT_EQ(table.FindGlyph(character), nullptr);
    }
    EXPECT_EQ(table.FindGlyphIndex(2), nullptr);
    EXPECT_EQ(table.FindCharacter('B'), replacement);
    EXPECT_EQ(shared->GetNextCachedGlyph(), nullptr);
    EXPECT_EQ(shared->Raster[2], 255);
}

TEST_F(FontGlyphCacheTest, MissingCharactersDoNotGrowIndexesOrConsumeAdditionalMemory)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(GetMemoryBudget(2));
    PServerFontGlyphTable table(cache);
    Ptr<PServerFontGlyph> supported = MakeGlyph(table, 1);
    Ptr<PServerFontGlyph> missing = MakeGlyph(table, 0);
    table.StoreGlyph(supported);
    table.AddCharacter(*supported, 'A');
    table.StoreGlyph(missing);
    const size_t memoryUsage = cache->GetMemoryUsage();
    cache->PinGlyph(*missing);
    PScopeExit releasePin([cache, missing] { cache->UnpinGlyph(*missing); });
    table.AddCharacter(*missing, 0);
    table.AddCharacter(*missing, 127);
    for (uint32_t character = 0x1000; character < 0x3000; ++character) {
        table.AddCharacter(*missing, character);
    }
    cache->Trim();
    EXPECT_EQ(table.GetMissingGlyph(), &*missing);
    EXPECT_TRUE(missing->GetCharacters().empty());
    EXPECT_EQ(table.GetCharacterBucketCount(), 0);
    EXPECT_EQ(table.GetCharacters(), nullptr);
    EXPECT_EQ(table.FindGlyphIndex(0), missing);
    EXPECT_EQ(table.FindCharacter('A'), supported);
    EXPECT_EQ(table.FindCharacter('B'), nullptr);
    EXPECT_EQ(table.FindCharacter(0), nullptr);
    EXPECT_EQ(table.FindCharacter(0x1000), nullptr);
    EXPECT_EQ(cache->GetMemoryUsage(), memoryUsage);
}

TEST_F(FontGlyphCacheTest, MissingGlyphFallbackSurvivesPinningAndClearsOnEviction)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(GetMemoryBudget(1));
    PServerFontGlyphTable table(cache);
    EXPECT_EQ(table.GetMissingGlyph(), nullptr);
    Ptr<PServerFontGlyph> missing = MakeGlyph(table, 0);
    table.StoreGlyph(missing);
    table.AddCharacter(*missing, 0xfdd0);
    cache->PinGlyph(*missing);
    PScopeExit releasePin([cache, missing]
    {
        if (missing->IsPinned()) {
            cache->UnpinGlyph(*missing);
        }
    });
    Ptr<PServerFontGlyph> other = MakeGlyph(table, 1);
    table.StoreGlyph(other);
    table.AddCharacter(*other, 'A');
    cache->Trim();
    EXPECT_EQ(table.GetMissingGlyph(), &*missing);
    EXPECT_EQ(table.FindGlyphIndex(1), nullptr);
    cache->UnpinGlyph(*missing);
    Ptr<PServerFontGlyph> replacement = MakeGlyph(table, 2);
    table.StoreGlyph(replacement);
    table.AddCharacter(*replacement, 'B');
    cache->Trim();
    EXPECT_EQ(table.GetMissingGlyph(), nullptr);
    EXPECT_EQ(table.FindGlyphIndex(0), nullptr);
    EXPECT_EQ(table.FindCharacter('B'), replacement);
    Ptr<PServerFontGlyph> reloaded = MakeGlyph(table, 0);
    table.StoreGlyph(reloaded);
    table.AddCharacter(*reloaded, 0xfdd1);
    cache->Trim();
    EXPECT_EQ(table.GetMissingGlyph(), &*reloaded);
    EXPECT_TRUE(reloaded->GetCharacters().empty());
}

TEST_F(FontGlyphCacheTest, OneLruEvictsAcrossIndependentFontTables)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(GetMemoryBudget(2, 2));
    PServerFontGlyphTable firstFont(cache);
    PServerFontGlyphTable secondFont(cache);
    Ptr<PServerFontGlyph> first = MakeGlyph(firstFont, 7);
    Ptr<PServerFontGlyph> second = MakeGlyph(secondFont, 7);
    Ptr<PServerFontGlyph> third = MakeGlyph(firstFont, 8);
    firstFont.StoreGlyph(first);
    firstFont.AddCharacter(*first, 'A');
    secondFont.StoreGlyph(second);
    secondFont.AddCharacter(*second, 'A');
    EXPECT_EQ(firstFont.FindGlyphIndex(7), first);
    EXPECT_EQ(secondFont.FindGlyphIndex(7), second);
    cache->TouchGlyph(*first);
    firstFont.StoreGlyph(third);
    firstFont.AddCharacter(*third, 'B');
    cache->Trim();
    EXPECT_EQ(firstFont.FindCharacter('A'), first);
    EXPECT_EQ(firstFont.FindCharacter('B'), third);
    EXPECT_EQ(secondFont.FindCharacter('A'), nullptr);
    EXPECT_EQ(secondFont.FindGlyphIndex(7), nullptr);
}

TEST_F(FontGlyphCacheTest, RetainsPinnedGlyphsAndTheirAliasesUntilRelease)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(GetMemoryBudget(1));
    PServerFontGlyphTable table(cache);
    Ptr<PServerFontGlyph> first = MakeGlyph(table, 1);
    Ptr<PServerFontGlyph> second = MakeGlyph(table, 2);
    Ptr<PServerFontGlyph> third = MakeGlyph(table, 3);
    PScopeExit releasePins([cache, first, third]
    {
        if (first->IsPinned()) {
            cache->UnpinGlyph(*first);
        }
        if (third->IsPinned()) {
            cache->UnpinGlyph(*third);
        }
        cache->Trim();
    });
    table.StoreGlyph(first);
    table.AddCharacter(*first, 'A');
    cache->PinGlyph(*first);
    table.StoreGlyph(second);
    table.AddCharacter(*second, 'B');
    cache->Trim();
    EXPECT_EQ(table.FindCharacter('A'), first);
    EXPECT_EQ(table.FindCharacter('B'), nullptr);
    table.StoreGlyph(third);
    table.AddCharacter(*third, 'C');
    cache->PinGlyph(*third);
    cache->Trim();
    EXPECT_EQ(table.FindCharacter('A'), first);
    EXPECT_EQ(table.FindCharacter('C'), third);
    cache->UnpinGlyph(*first);
    cache->Trim();
    EXPECT_EQ(table.FindCharacter('A'), nullptr);
    EXPECT_EQ(first->Raster[2], 255);
    EXPECT_EQ(table.FindCharacter('C'), third);
}

TEST_F(FontGlyphCacheTest, AliasCapacityCountsAgainstMemoryBudgetEvenForOneGlyph)
{
    const size_t characterBuckets = PServerFontGlyphTable::INITIAL_BUCKET_COUNT * sizeof(const PDisplayFontGlyph*);
    const size_t budget = GetMemoryBudget(1) + PFontGlyphCache::GetAllocationMemorySize(characterBuckets)
        + PFontGlyphCache::GetAllocationMemorySize(2 * sizeof(PDisplayFontCharacter));
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(budget);
    PServerFontGlyphTable table(cache);
    Ptr<PServerFontGlyph> shared = MakeGlyph(table, 2);
    table.StoreGlyph(shared);
    cache->PinGlyph(*shared);
    PScopeExit releasePin([cache, shared]
    {
        if (shared->IsPinned()) {
            cache->UnpinGlyph(*shared);
        }
    });
    for (uint32_t character = 0x100; character < 0x103; ++character) {
        table.AddCharacter(*shared, character);
    }
    // Three aliases reserve four slots; charge the unused slot as well.
    const size_t expectedSize = PFontGlyphCache::GetAllocationMemorySize(sizeof(PServerFontGlyph) + 4)
        + PFontGlyphCache::GetAllocationMemorySize(4 * sizeof(PDisplayFontCharacter));
    EXPECT_EQ(shared->GetMemorySize(), expectedSize);
    EXPECT_EQ(cache->GetMemoryUsage(), table.GetMemorySize() + expectedSize);
    EXPECT_GT(cache->GetMemoryUsage(), budget);
    cache->Trim();
    EXPECT_EQ(table.FindCharacter(0x100), shared);
    cache->UnpinGlyph(*shared);
    cache->Trim();
    for (uint32_t character = 0x100; character < 0x103; ++character) {
        EXPECT_EQ(table.FindCharacter(character), nullptr);
    }
    EXPECT_EQ(table.FindGlyphIndex(2), nullptr);
    Ptr<PServerFontGlyph> replacement = MakeGlyph(table, 1);
    table.StoreGlyph(replacement);
    for (uint32_t character = 0x200; character < 0x202; ++character) {
        table.AddCharacter(*replacement, character);
    }
    cache->Trim();
    EXPECT_EQ(table.FindCharacter(0x201), replacement);
    EXPECT_EQ(cache->GetMemoryUsage(), budget);
}

TEST_F(FontGlyphCacheTest, DeletionPreservesCollidingGlyphChains)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    PServerFontGlyphTable table(cache);
    std::array<uint32_t, 4> characters;
    size_t count = 0;
    for (uint32_t character = 128; count < characters.size(); ++character)
    {
        if (PDisplayFontCache::GetBucketIndex(character, 16) == 15) {
            characters[count++] = character;
        }
    }
    std::array<Ptr<PServerFontGlyph>, 4> glyphs;
    for (size_t index = 0; index < glyphs.size(); ++index)
    {
        glyphs[index] = MakeGlyph(table, characters[index]);
        table.StoreGlyph(glyphs[index]);
        table.AddCharacter(*glyphs[index], characters[index]);
    }
    ASSERT_EQ(table.GetCharacterBucketCount(), 16);
    table.EraseGlyph(*glyphs[1]);
    table.EraseGlyph(*glyphs[0]);
    EXPECT_EQ(table.FindCharacter(characters[0]), nullptr);
    EXPECT_EQ(table.FindCharacter(characters[1]), nullptr);
    for (size_t index = 2; index < glyphs.size(); ++index)
    {
        EXPECT_EQ(table.FindCharacter(characters[index]), glyphs[index]);
        EXPECT_EQ(table.FindGlyphIndex(characters[index]), glyphs[index]);
    }
    Ptr<PServerFontGlyph> replacement = MakeGlyph(table, characters[0]);
    table.StoreGlyph(replacement);
    table.AddCharacter(*replacement, characters[0]);
    table.EraseGlyph(*glyphs[2]);
    EXPECT_EQ(table.FindCharacter(characters[0]), replacement);
    EXPECT_EQ(table.FindCharacter(characters[3]), glyphs[3]);
}

TEST_F(FontGlyphCacheTest, SameBucketAliasesKeepOneGlyphInTheChain)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    PServerFontGlyphTable table(cache);
    const std::vector<uint32_t> characters = FindBucketCharacters(6, 16, 15);
    Ptr<PServerFontGlyph> tail = MakeGlyph(table, 1);
    Ptr<PServerFontGlyph> shared = MakeGlyph(table, 2);
    Ptr<PServerFontGlyph> head = MakeGlyph(table, 3);
    table.StoreGlyph(tail);
    table.StoreGlyph(shared);
    table.StoreGlyph(head);
    table.AddCharacter(*tail, characters[0]);
    uint32_t ascii = 0;
    while (PDisplayFontCache::GetBucketIndex(ascii, 16) != 15) {
        ++ascii;
    }
    ASSERT_LT(ascii, PDisplayFontCache::ASCII_COUNT);
    table.AddCharacter(*shared, ascii);
    table.AddCharacter(*shared, characters[1]);
    table.AddCharacter(*head, characters[2]);
    table.AddCharacter(*shared, characters[3]);
    table.AddCharacter(*shared, characters[4]);

    ASSERT_EQ(table.GetCharacterBucketCount(), 16);
    ASSERT_EQ(table.GetCharacters()[15], &*head);
    ASSERT_EQ(head->GetCharacters()[0].NextGlyph, &*shared);
    ASSERT_EQ(shared->GetCharacters()[1].NextGlyph, &*tail);
    ASSERT_EQ(tail->GetCharacters()[0].NextGlyph, nullptr);
    EXPECT_EQ(shared->GetCharacters()[0].NextGlyph, nullptr);
    EXPECT_EQ(shared->GetCharacters()[2].NextGlyph, nullptr);
    EXPECT_EQ(shared->GetCharacters()[3].NextGlyph, nullptr);
    EXPECT_EQ(table.FindCharacter(ascii), shared);
    EXPECT_EQ(table.FindCharacter(characters[0]), tail);
    EXPECT_EQ(table.FindCharacter(characters[1]), shared);
    EXPECT_EQ(table.FindCharacter(characters[2]), head);
    EXPECT_EQ(table.FindCharacter(characters[3]), shared);
    EXPECT_EQ(table.FindCharacter(characters[4]), shared);
    EXPECT_EQ(table.FindCharacter(characters[5]), nullptr);

    table.EraseGlyph(*shared);
    ASSERT_EQ(head->GetCharacters()[0].NextGlyph, &*tail);
    EXPECT_EQ(table.FindCharacter(ascii), nullptr);
    EXPECT_EQ(table.FindCharacter(characters[1]), nullptr);
    EXPECT_EQ(table.FindCharacter(characters[3]), nullptr);
    EXPECT_EQ(table.FindCharacter(characters[4]), nullptr);
    EXPECT_EQ(table.FindCharacter(characters[0]), tail);
    table.EraseGlyph(*head);
    ASSERT_EQ(table.GetCharacters()[15], &*tail);
    table.EraseGlyph(*tail);
    EXPECT_EQ(table.GetCharacters(), nullptr);
    EXPECT_EQ(table.GetCharacterBucketCount(), 0);
}

TEST_F(FontGlyphCacheTest, AliasStorageGrowthPreservesLinksInDifferentBuckets)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    PServerFontGlyphTable table(cache);
    const std::vector<uint32_t> firstBucket = FindBucketCharacters(5, 16, 0);
    const std::vector<uint32_t> secondBucket = FindBucketCharacters(4, 16, 1);
    Ptr<PServerFontGlyph> firstTail = MakeGlyph(table, 1);
    Ptr<PServerFontGlyph> secondTail = MakeGlyph(table, 2);
    Ptr<PServerFontGlyph> shared = MakeGlyph(table, 3);
    Ptr<PServerFontGlyph> firstHead = MakeGlyph(table, 4);
    Ptr<PServerFontGlyph> secondHead = MakeGlyph(table, 5);
    table.StoreGlyph(firstTail);
    table.StoreGlyph(secondTail);
    table.StoreGlyph(shared);
    table.StoreGlyph(firstHead);
    table.StoreGlyph(secondHead);
    table.AddCharacter(*firstTail, firstBucket[0]);
    table.AddCharacter(*secondTail, secondBucket[0]);
    table.AddCharacter(*shared, firstBucket[1]);
    ASSERT_EQ(shared->GetCharacters().data(), shared->Characters.Inline);
    table.AddCharacter(*firstHead, firstBucket[2]);
    table.AddCharacter(*shared, secondBucket[1]);
    ASSERT_EQ(shared->GetCharacters().data(), shared->Characters.Heap);
    table.AddCharacter(*secondHead, secondBucket[2]);
    table.AddCharacter(*shared, firstBucket[3]);

    ASSERT_EQ(table.GetCharacterBucketCount(), 16);
    ASSERT_EQ(firstHead->GetCharacters()[0].NextGlyph, &*shared);
    ASSERT_EQ(secondHead->GetCharacters()[0].NextGlyph, &*shared);
    ASSERT_EQ(shared->GetCharacters()[0].NextGlyph, &*firstTail);
    ASSERT_EQ(shared->GetCharacters()[1].NextGlyph, &*secondTail);
    EXPECT_EQ(table.FindCharacter(firstBucket[0]), firstTail);
    EXPECT_EQ(table.FindCharacter(firstBucket[1]), shared);
    EXPECT_EQ(table.FindCharacter(firstBucket[2]), firstHead);
    EXPECT_EQ(table.FindCharacter(firstBucket[3]), shared);
    EXPECT_EQ(table.FindCharacter(secondBucket[0]), secondTail);
    EXPECT_EQ(table.FindCharacter(secondBucket[1]), shared);
    EXPECT_EQ(table.FindCharacter(secondBucket[2]), secondHead);
    EXPECT_EQ(table.FindCharacter(firstBucket[4]), nullptr);
    EXPECT_EQ(table.FindCharacter(secondBucket[3]), nullptr);

    table.EraseGlyph(*shared);
    EXPECT_EQ(firstHead->GetCharacters()[0].NextGlyph, &*firstTail);
    EXPECT_EQ(secondHead->GetCharacters()[0].NextGlyph, &*secondTail);
    EXPECT_EQ(table.FindCharacter(firstBucket[1]), nullptr);
    EXPECT_EQ(table.FindCharacter(firstBucket[3]), nullptr);
    EXPECT_EQ(table.FindCharacter(secondBucket[1]), nullptr);
}

TEST_F(FontGlyphCacheTest, RehashSplitsAliasesIntoIndependentGlyphChains)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    PServerFontGlyphTable table(cache);
    const std::vector<uint32_t> firstBucket = FindBucketCharacters(6, 32, 15);
    const std::vector<uint32_t> secondBucket = FindBucketCharacters(6, 32, 31);
    Ptr<PServerFontGlyph> firstTail = MakeGlyph(table, 1);
    Ptr<PServerFontGlyph> secondTail = MakeGlyph(table, 2);
    Ptr<PServerFontGlyph> shared = MakeGlyph(table, 3);
    table.StoreGlyph(firstTail);
    table.StoreGlyph(secondTail);
    table.StoreGlyph(shared);
    table.AddCharacter(*firstTail, firstBucket[0]);
    table.AddCharacter(*secondTail, secondBucket[0]);
    for (size_t index = 1; index < 4; ++index)
    {
        table.AddCharacter(*shared, firstBucket[index]);
        table.AddCharacter(*shared, secondBucket[index]);
    }
    ASSERT_EQ(table.GetCharacterBucketCount(), 16);
    table.AddCharacter(*shared, firstBucket[4]);
    table.AddCharacter(*shared, secondBucket[4]);
    ASSERT_EQ(table.GetCharacterBucketCount(), 32);
    for (size_t index = 1; index < 5; ++index)
    {
        EXPECT_EQ(table.FindCharacter(firstBucket[index]), shared);
        EXPECT_EQ(table.FindCharacter(secondBucket[index]), shared);
    }
    EXPECT_EQ(table.FindCharacter(firstBucket[0]), firstTail);
    EXPECT_EQ(table.FindCharacter(secondBucket[0]), secondTail);
    EXPECT_EQ(table.FindCharacter(firstBucket[5]), nullptr);
    EXPECT_EQ(table.FindCharacter(secondBucket[5]), nullptr);
    table.EraseGlyph(*shared);
    EXPECT_EQ(table.FindCharacter(firstBucket[0]), firstTail);
    EXPECT_EQ(table.FindCharacter(secondBucket[0]), secondTail);
    for (size_t index = 1; index < 5; ++index)
    {
        EXPECT_EQ(table.FindCharacter(firstBucket[index]), nullptr);
        EXPECT_EQ(table.FindCharacter(secondBucket[index]), nullptr);
    }
}

TEST_F(FontGlyphCacheTest, GrowingBothIndexesPreservesPinnedGlyphAddresses)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>();
    PServerFontGlyphTable table(cache);
    const PDisplayFontCache* view = &table;
    Ptr<PServerFontGlyph> pinned = MakeGlyph(table, 1);
    table.StoreGlyph(pinned);
    table.AddCharacter(*pinned, 'A');
    table.AddCharacter(*pinned, 0x100);
    cache->PinGlyph(*pinned);
    PScopeExit releasePin([cache, pinned] { cache->UnpinGlyph(*pinned); });
    const PDisplayFontGlyph* before = view->FindGlyph(0x100);
    for (FT_UInt index = 2; index <= 96; ++index)
    {
        Ptr<PServerFontGlyph> glyph = MakeGlyph(table, index);
        table.StoreGlyph(glyph);
        table.AddCharacter(*glyph, 0xff + index);
    }
    size_t glyphBytes = 0;
    for (FT_UInt index = 1; index <= 96; ++index)
    {
        const PServerFontGlyph* cached = table.FindGlyphIndex(index);
        ASSERT_NE(cached, nullptr);
        glyphBytes += cached->GetMemorySize();
        const PDisplayFontGlyph* glyph = view->FindGlyph(0xff + index);
        ASSERT_NE(glyph, nullptr);
        EXPECT_EQ(glyph->GlyphIndex, index);
        EXPECT_EQ(glyph->Raster[1], 128);
        EXPECT_EQ(table.FindGlyphIndex(index), glyph);
    }
    EXPECT_EQ(view->FindGlyph(0x100), before);
    EXPECT_EQ(view->FindGlyph('A'), before);
    EXPECT_TRUE(pinned->IsPinned());
    EXPECT_EQ(table.GetCharacterBucketCount(), 256);
    const size_t tableBytes = sizeof(PServerFontGlyphTable)
        + PFontGlyphCache::GetAllocationMemorySize(128 * sizeof(Ptr<PServerFontGlyph>))
        + PFontGlyphCache::GetAllocationMemorySize(256 * sizeof(const PDisplayFontGlyph*));
    EXPECT_EQ(table.GetMemorySize(), tableBytes);
    EXPECT_EQ(cache->GetMemoryUsage(), tableBytes + glyphBytes);
}

TEST_F(FontGlyphCacheTest, DestroyingAFontUnregistersItsGlyphsFromTheGlobalLru)
{
    Ptr<PFontGlyphCache> cache = ptr_new<PFontGlyphCache>(GetMemoryBudget(1));
    PServerFontGlyphTable survivor(cache);
    Ptr<PServerFontGlyph> glyph = MakeGlyph(survivor, 1);
    survivor.StoreGlyph(glyph);
    survivor.AddCharacter(*glyph, 'A');
    const size_t memoryUsage = cache->GetMemoryUsage();
    {
        PServerFontGlyphTable temporary(cache);
        Ptr<PServerFontGlyph> other = MakeGlyph(temporary, 1);
        temporary.StoreGlyph(other);
        temporary.AddCharacter(*other, 'A');
    }
    cache->Trim();
    EXPECT_EQ(survivor.FindCharacter('A'), glyph);
    EXPECT_EQ(cache->GetMemoryUsage(), memoryUsage);
}

// This integration test requires the bundled fonts on the mounted filesystem.
TEST(ServerFontStorage, MeasurementRendersGlyphsAndDrawingKeepsTheirAddresses)
{
    PFontManager manager;
    manager.ScanFonts();
    Ptr<PServerFontInstance> font = manager.GetFont(PFontDescription());
    const PFontTextMetrics expected = font->MeasureText("A\xc3\x85 ");
    const PDisplayFontCache* view = &font->GetDisplayCache();
    const PDisplayFontGlyph* ascii = view->FindGlyph('A');
    const PDisplayFontGlyph* unicode = view->FindGlyph(0xc5);
    ASSERT_NE(ascii, nullptr);
    ASSERT_NE(unicode, nullptr);
    ASSERT_NE(ascii->Raster, nullptr);
    ASSERT_NE(unicode->Raster, nullptr);
    ASSERT_NE(view->FindGlyph(' '), nullptr);
    EXPECT_EQ(view->FindGlyph(' ')->Raster, nullptr);
    PServerPreparedText first = font->PrepareText("A\xc3\x85 ");
    EXPECT_EQ(first.Cache->GetDisplayCache(), view);
    EXPECT_EQ(view->FindGlyph('A'), ascii);
    EXPECT_EQ(view->FindGlyph(0xc5), unicode);
    std::string text;
    // Noncharacters use glyph zero without adding aliases to either lookup table.
    for (uint32_t character = 0xfdd0; character <= 0xfdef; ++character)
    {
        char encoded[4];
        const size_t length = unicode_to_utf8(encoded, character);
        text.append(encoded, length);
    }
    // Warm previously unseen supported characters after the missing-glyph fallback.
    for (uint32_t character = 32; character < 127; ++character) {
        text.push_back(char(character));
    }
    PServerPreparedText grown = font->PrepareText(text);
    EXPECT_EQ(grown.Cache, first.Cache);
    EXPECT_EQ(grown.Cache->GetDisplayCache(), view);
    EXPECT_EQ(view->FindGlyph('A'), ascii);
    EXPECT_EQ(view->FindGlyph(0xc5), unicode);
    const PDisplayFontGlyph* missing = view->GetMissingGlyph();
    ASSERT_NE(missing, nullptr);
    EXPECT_EQ(missing->GlyphIndex, 0);
    EXPECT_TRUE(missing->GetCharacters().empty());
    for (uint32_t character = 0xfdd0; character <= 0xfdef; ++character) {
        EXPECT_EQ(view->FindGlyph(character), nullptr);
    }
    ASSERT_NE(view->FindGlyph('Z'), nullptr);
    EXPECT_NE(view->FindGlyph('Z')->GlyphIndex, 0);
    first.Cache = nullptr;
    grown.Cache = nullptr;
    PServerPreparedText next = font->PrepareText("A\xc3\x85 ");
    EXPECT_EQ(next.Cache->GetDisplayCache(), view);
    EXPECT_EQ(next.Metrics.Advance, expected.Advance);
    EXPECT_FLOAT_EQ(next.Metrics.Width, expected.Width);
}

TEST(ServerFontStorage, BoundedPreparationKeepsTheIncludedAdvanceAndUtf8Prefix)
{
    PFontManager manager;
    manager.ScanFonts();
    Ptr<PServerFontInstance> font = manager.GetFont(PFontDescription());
    ASSERT_TRUE(font->GetDisplayCache().HasKerning());
    const std::string_view text = "A\xc3\x85V";
    const PFontTextMetrics first = font->MeasureText(text.substr(0, 1));
    const PFontTextMetrics second = font->MeasureText(text.substr(0, 3));
    const float width = (first.Width + second.Width) * 0.5f;
    std::array<int32_t, 4> kernings;
    kernings.fill(0x123456);
    const PServerPreparedText partial = font->PrepareText(text, kernings.data(), width, true);
    EXPECT_EQ(partial.Metrics.ByteCount, 3);
    EXPECT_EQ(partial.Metrics.Advance, second.Advance);
    EXPECT_FLOAT_EQ(partial.Metrics.Width, second.Width);
    EXPECT_EQ(partial.KerningCount, 2);
    EXPECT_EQ(kernings[2], 0x123456);
    kernings.fill(0x123456);
    const PServerPreparedText complete = font->PrepareText(text, kernings.data(), width, false);
    EXPECT_EQ(complete.Metrics.ByteCount, 1);
    EXPECT_EQ(complete.Metrics.Advance, first.Advance);
    EXPECT_FLOAT_EQ(complete.Metrics.Width, first.Width);
    EXPECT_EQ(complete.KerningCount, 1);
    EXPECT_EQ(kernings[1], 0x123456);
}

TEST(ServerFontStorage, BoundedPreparationPreservesKerningAndMissingGlyphs)
{
    PFontManager manager;
    manager.ScanFonts();
    Ptr<PServerFontInstance> font = manager.GetFont(PFontDescription());
    const std::string_view text = "AV\xef\xb7\x90";
    std::array<int32_t, 3> expectedKernings{};
    const PServerPreparedText expected = font->PrepareText(text, expectedKernings.data());
    std::array<int32_t, 3> kernings{};
    const PServerPreparedText actual = font->PrepareText(text, kernings.data(), 100.0f, true);
    EXPECT_EQ(actual.Metrics.ByteCount, text.size());
    EXPECT_EQ(actual.Metrics.Advance, expected.Metrics.Advance);
    EXPECT_EQ(actual.KerningCount, 3);
    EXPECT_EQ(kernings, expectedKernings);
    EXPECT_LT(kernings[1], 0);
    ASSERT_NE(actual.Cache->GetDisplayCache()->GetMissingGlyph(), nullptr);
}

TEST(ServerFontStorage, EmptyTextAndPaddingDoNotAddToThePenAdvance)
{
    PFontManager manager;
    manager.ScanFonts();
    Ptr<PServerFontInstance> font = manager.GetFont(PFontDescription());
    const PFontTextMetrics space = font->MeasureText(" ");
    const PServerPreparedText padded = font->PrepareText(" ", nullptr, 100.0f, true);
    EXPECT_EQ(padded.Metrics.Advance, space.Advance);
    const PServerPreparedText exact = font->PrepareText("  ", nullptr, space.Width, true);
    EXPECT_EQ(exact.Metrics.ByteCount, 1);
    EXPECT_EQ(exact.Metrics.Advance, space.Advance);
    const PServerPreparedText empty = font->PrepareText("", nullptr, 100.0f, true);
    EXPECT_EQ(empty.Metrics.Advance, PPoint());
    EXPECT_EQ(empty.Metrics.ByteCount, 0);
    EXPECT_EQ(empty.KerningCount, 0);
    const PServerPreparedText zeroWidth = font->PrepareText("A", nullptr, 0.0f, true);
    EXPECT_EQ(zeroWidth.Metrics.ByteCount, 0);
    EXPECT_EQ(zeroWidth.Metrics.Advance, PPoint());
}

TEST(ServerFontStorage, WholeCharactersRequireTheirOverhangToFit)
{
    PFontManager manager;
    manager.ScanFonts();
    PFontDescription description;
    description.Style = "Oblique";
    Ptr<PServerFontInstance> font = manager.GetFont(description);
    const PFontTextMetrics metrics = font->MeasureText("f");
    const PDisplayFontGlyph* glyph = font->GetDisplayCache().FindGlyph('f');
    ASSERT_NE(glyph, nullptr);
    ASSERT_GT(float(glyph->Bounds.right), metrics.Width);
    const PServerPreparedText omitted = font->PrepareText("f", nullptr, metrics.Width, false);
    EXPECT_EQ(omitted.Metrics.ByteCount, 0);
    EXPECT_EQ(omitted.Metrics.Advance, PPoint());
    const PServerPreparedText partial = font->PrepareText("f", nullptr, metrics.Width, true);
    EXPECT_EQ(partial.Metrics.ByteCount, 1);
    EXPECT_EQ(partial.Metrics.Advance, metrics.Advance);
    const PServerPreparedText fits = font->PrepareText("f", nullptr, float(glyph->Bounds.right), false);
    EXPECT_EQ(fits.Metrics.ByteCount, 1);
    EXPECT_EQ(fits.Metrics.Advance, metrics.Advance);
}
