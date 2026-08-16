// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include <algorithm>
#include <iterator>

#include <Utils/UnicodeCaseFolding.h>

#include "UnicodeCaseFoldingData.h"

inline constexpr uint32_t unicode_case_fold_ascii_character(uint8_t character) noexcept
{
    return (character >= 'A' && character <= 'Z') ? character + ('a' - 'A') : character;
}

static size_t unicode_case_fold_length(const char* begin, const char* end) noexcept
{
    size_t length = 0;
    const PUnicodeCaseFoldIterator iteratorEnd(end, end);
    for (PUnicodeCaseFoldIterator iterator(begin, end); iterator != iteratorEnd; ++iterator) {
        ++length;
    }
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t unicode_case_fold_non_ascii(uint32_t character, uint32_t* foldedCharacters) noexcept
{
    size_t rangeBegin = 0;
    size_t rangeEnd = std::size(g_UnicodeCaseFoldRanges);
    while (rangeBegin < rangeEnd)
    {
        const size_t rangeIndex = rangeBegin + (rangeEnd - rangeBegin) / 2;
        const UnicodeCaseFoldRange& range = g_UnicodeCaseFoldRanges[rangeIndex];
        const uint32_t rangeLast = range.First + uint32_t(range.Count - 1) * range.Stride;
        if (character < range.First)
        {
            rangeEnd = rangeIndex;
        }
        else if (character > rangeLast)
        {
            rangeBegin = rangeIndex + 1;
        }
        else
        {
            const uint32_t rangeOffset = character - range.First;
            if (range.Stride == 1 || (rangeOffset & 1) == 0) {
                foldedCharacters[0] = uint32_t(int32_t(character) + range.Delta);
                return 1;
            }
            break;
        }
    }

    size_t mappingBegin = 0;
    size_t mappingEnd = std::size(g_UnicodeCaseFoldMappings);
    while (mappingBegin < mappingEnd)
    {
        const size_t mappingIndex = mappingBegin + (mappingEnd - mappingBegin) / 2;
        const UnicodeCaseFoldMapping& mapping = g_UnicodeCaseFoldMappings[mappingIndex];
        if (character < mapping.Source)
        {
            mappingEnd = mappingIndex;
        }
        else if (character > mapping.Source)
        {
            mappingBegin = mappingIndex + 1;
        }
        else
        {
            if ((mapping.Mapping & UNICODE_CASE_FOLD_EXPANSION_FLAG) == 0)
            {
                foldedCharacters[0] = mapping.Mapping;
                return 1;
            }

            const size_t expansionLength = mapping.Mapping & UNICODE_CASE_FOLD_EXPANSION_LENGTH_MASK;
            const size_t expansionOffset = (mapping.Mapping & ~(UNICODE_CASE_FOLD_EXPANSION_FLAG | UNICODE_CASE_FOLD_EXPANSION_LENGTH_MASK)) >> 2;
            for (size_t expansionIndex = 0; expansionIndex < expansionLength; ++expansionIndex) {
                foldedCharacters[expansionIndex] = g_UnicodeCaseFoldExpansionCharacters[expansionOffset + expansionIndex];
            }
            return expansionLength;
        }
    }

    foldedCharacters[0] = character;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PUnicodeCaseFoldIterator::LoadCurrentCharacter() noexcept
{
    const uint8_t firstByte = uint8_t(m_Position[0]);
    if (firstByte < 0x80)
    {
        m_SourceCharacterByteLength = 1;
        m_FoldedCharacters[0] = unicode_case_fold_ascii_character(firstByte);
        m_FoldedCharacterCount = 1;
    }
    else
    {
        const uint32_t sourceCharacter = utf8_to_unicode(m_Position, size_t(m_End - m_Position), m_SourceCharacterByteLength);
        m_FoldedCharacterCount = unicode_case_fold_non_ascii(sourceCharacter, m_FoldedCharacters);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int unicode_case_fold_compare(const char* lhsBegin, const char* lhsEnd, const char* rhsBegin, const char* rhsEnd) noexcept
{
    const char* lhsPosition = lhsBegin;
    const char* rhsPosition = rhsBegin;
    while (lhsPosition != lhsEnd && rhsPosition != rhsEnd)
    {
        const uint8_t lhsCharacter = uint8_t(lhsPosition[0]);
        const uint8_t rhsCharacter = uint8_t(rhsPosition[0]);
        if ((lhsCharacter | rhsCharacter) >= 0x80) {
            break;
        }

        const uint32_t foldedLhsCharacter = unicode_case_fold_ascii_character(lhsCharacter);
        const uint32_t foldedRhsCharacter = unicode_case_fold_ascii_character(rhsCharacter);
        if (foldedLhsCharacter != foldedRhsCharacter) {
            return (foldedLhsCharacter < foldedRhsCharacter) ? -1 : 1;
        }
        ++lhsPosition;
        ++rhsPosition;
    }

    if (lhsPosition == lhsEnd) {
        return (rhsPosition == rhsEnd) ? 0 : -1;
    }
    if (rhsPosition == rhsEnd) {
        return 1;
    }

    PUnicodeCaseFoldIterator lhsIterator(lhsPosition, lhsEnd);
    const PUnicodeCaseFoldIterator lhsIteratorEnd(lhsEnd, lhsEnd);
    PUnicodeCaseFoldIterator rhsIterator(rhsPosition, rhsEnd);
    const PUnicodeCaseFoldIterator rhsIteratorEnd(rhsEnd, rhsEnd);
    while (lhsIterator != lhsIteratorEnd && rhsIterator != rhsIteratorEnd)
    {
        const uint32_t lhsCharacter = *lhsIterator;
        const uint32_t rhsCharacter = *rhsIterator;
        if (lhsCharacter != rhsCharacter) {
            return (lhsCharacter < rhsCharacter) ? -1 : 1;
        }
        ++lhsIterator;
        ++rhsIterator;
    }

    if (lhsIterator == lhsIteratorEnd) {
        return (rhsIterator == rhsIteratorEnd) ? 0 : -1;
    }
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool unicode_case_fold_starts_with(const char* sourceBegin, const char* sourceEnd, const char* tokenBegin, const char* tokenEnd) noexcept
{
    const char* sourcePosition = sourceBegin;
    const char* tokenPosition = tokenBegin;
    while (sourcePosition != sourceEnd && tokenPosition != tokenEnd)
    {
        const uint8_t sourceCharacter = uint8_t(sourcePosition[0]);
        const uint8_t tokenCharacter = uint8_t(tokenPosition[0]);
        if ((sourceCharacter | tokenCharacter) >= 0x80) {
            break;
        }
        if (unicode_case_fold_ascii_character(sourceCharacter) != unicode_case_fold_ascii_character(tokenCharacter)) {
            return false;
        }
        ++sourcePosition;
        ++tokenPosition;
    }

    if (tokenPosition == tokenEnd) {
        return true;
    }
    if (sourcePosition == sourceEnd) {
        return false;
    }

    PUnicodeCaseFoldIterator sourceIterator(sourcePosition, sourceEnd);
    const PUnicodeCaseFoldIterator sourceIteratorEnd(sourceEnd, sourceEnd);
    PUnicodeCaseFoldIterator tokenIterator(tokenPosition, tokenEnd);
    const PUnicodeCaseFoldIterator tokenIteratorEnd(tokenEnd, tokenEnd);
    while (tokenIterator != tokenIteratorEnd)
    {
        if (sourceIterator == sourceIteratorEnd) {
            return false;
        }
        if (*sourceIterator != *tokenIterator) {
            return false;
        }
        ++sourceIterator;
        ++tokenIterator;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool unicode_case_fold_ends_with(const char* sourceBegin, const char* sourceEnd, const char* tokenBegin, const char* tokenEnd) noexcept
{
    const char* sourcePosition = sourceEnd;
    const char* tokenPosition = tokenEnd;
    while (sourcePosition != sourceBegin && tokenPosition != tokenBegin)
    {
        const uint8_t sourceCharacter = uint8_t(sourcePosition[-1]);
        const uint8_t tokenCharacter = uint8_t(tokenPosition[-1]);
        if ((sourceCharacter | tokenCharacter) >= 0x80) {
            break;
        }
        if (unicode_case_fold_ascii_character(sourceCharacter) != unicode_case_fold_ascii_character(tokenCharacter)) {
            return false;
        }
        --sourcePosition;
        --tokenPosition;
    }

    if (tokenPosition == tokenBegin) {
        return true;
    }
    if (sourcePosition == sourceBegin) {
        return false;
    }

    const size_t sourceLength = unicode_case_fold_length(sourceBegin, sourcePosition);
    const size_t tokenLength = unicode_case_fold_length(tokenBegin, tokenPosition);
    if (sourceLength < tokenLength) {
        return false;
    }

    PUnicodeCaseFoldIterator sourceIterator(sourceBegin, sourcePosition);
    const PUnicodeCaseFoldIterator sourceIteratorEnd(sourcePosition, sourcePosition);
    for (size_t skipCount = sourceLength - tokenLength; skipCount != 0; --skipCount) {
        ++sourceIterator;
    }

    PUnicodeCaseFoldIterator tokenIterator(tokenBegin, tokenPosition);
    const PUnicodeCaseFoldIterator tokenIteratorEnd(tokenPosition, tokenPosition);
    while (tokenIterator != tokenIteratorEnd)
    {
        if (sourceIterator == sourceIteratorEnd || *sourceIterator != *tokenIterator) {
            return false;
        }
        ++sourceIterator;
        ++tokenIterator;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool unicode_case_fold_contains(const char* sourceBegin, const char* sourceEnd, const char* tokenBegin, const char* tokenEnd) noexcept
{
    bool sourceIsASCII = true;
    for (const char* position = sourceBegin; position != sourceEnd; ++position)
    {
        if (uint8_t(position[0]) >= 0x80)
        {
            sourceIsASCII = false;
            break;
        }
    }

    bool tokenIsASCII = true;
    for (const char* position = tokenBegin; position != tokenEnd; ++position)
    {
        if (uint8_t(position[0]) >= 0x80)
        {
            tokenIsASCII = false;
            break;
        }
    }

    if (sourceIsASCII && tokenIsASCII)
    {
        const auto compareCharacters = [](char lhs, char rhs)
        {
            return unicode_case_fold_ascii_character(uint8_t(lhs)) == unicode_case_fold_ascii_character(uint8_t(rhs));
        };
        return std::search(sourceBegin, sourceEnd, tokenBegin, tokenEnd, compareCharacters) != sourceEnd;
    }

    const PUnicodeCaseFoldIterator sourceIteratorBegin(sourceBegin, sourceEnd);
    const PUnicodeCaseFoldIterator sourceIteratorEnd(sourceEnd, sourceEnd);
    const PUnicodeCaseFoldIterator tokenIteratorBegin(tokenBegin, tokenEnd);
    const PUnicodeCaseFoldIterator tokenIteratorEnd(tokenEnd, tokenEnd);
    return std::search(sourceIteratorBegin, sourceIteratorEnd, tokenIteratorBegin, tokenIteratorEnd) != sourceIteratorEnd;
}
