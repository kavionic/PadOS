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

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>

#include <Utils/UTF8Utils.h>

inline constexpr size_t UNICODE_CASE_FOLD_MAX_EXPANSION_LENGTH = 3;

size_t unicode_case_fold_non_ascii(uint32_t character, uint32_t* foldedCharacters) noexcept;

class PUnicodeCaseFoldIterator
{
public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;
    using value_type = uint32_t;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = uint32_t;

    PUnicodeCaseFoldIterator() noexcept = default;
    PUnicodeCaseFoldIterator(const char* position, const char* end) noexcept : m_Position(position), m_End(end)
    {
        if (m_Position != m_End) {
            LoadCurrentCharacter();
        }
    }

    uint32_t operator*() const noexcept
    {
        return m_FoldedCharacters[m_FoldedCharacterIndex];
    }

    PUnicodeCaseFoldIterator& operator++() noexcept
    {
        ++m_FoldedCharacterIndex;
        if (m_FoldedCharacterIndex == m_FoldedCharacterCount)
        {
            m_Position += m_SourceCharacterByteLength;
            m_FoldedCharacterIndex = 0;
            if (m_Position != m_End) {
                LoadCurrentCharacter();
            }
        }
        return *this;
    }

    PUnicodeCaseFoldIterator operator++(int) noexcept
    {
        PUnicodeCaseFoldIterator previous = *this;
        ++(*this);
        return previous;
    }

    bool operator==(const PUnicodeCaseFoldIterator& rhs) const noexcept
    {
        return m_Position == rhs.m_Position && m_FoldedCharacterIndex == rhs.m_FoldedCharacterIndex;
    }

private:
    void LoadCurrentCharacter() noexcept;

    const char* m_Position = nullptr;
    const char* m_End = nullptr;
    uint32_t m_FoldedCharacters[UNICODE_CASE_FOLD_MAX_EXPANSION_LENGTH] = {};
    size_t m_SourceCharacterByteLength = 0;
    size_t m_FoldedCharacterCount = 0;
    size_t m_FoldedCharacterIndex = 0;
};

int unicode_case_fold_compare(const char* lhsBegin, const char* lhsEnd, const char* rhsBegin, const char* rhsEnd) noexcept;
bool unicode_case_fold_starts_with(const char* sourceBegin, const char* sourceEnd, const char* tokenBegin, const char* tokenEnd) noexcept;
bool unicode_case_fold_ends_with(const char* sourceBegin, const char* sourceEnd, const char* tokenBegin, const char* tokenEnd) noexcept;
bool unicode_case_fold_contains(const char* sourceBegin, const char* sourceEnd, const char* tokenBegin, const char* tokenEnd) noexcept;
