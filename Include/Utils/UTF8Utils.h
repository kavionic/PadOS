// This file is part of PadOS.
//
// Copyright (C) 2018-2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 18/05/18 11:36:18

#pragma once

#include <cstddef>
#include <cstdint>
#include <iterator>

inline constexpr uint32_t UNICODE_REPLACEMENT_CHARACTER = 0xfffd;
inline constexpr uint32_t UNICODE_MAX_CODE_POINT = 0x10ffff;
inline constexpr size_t UTF8_MAX_CHARACTER_LENGTH = 4;

inline constexpr bool is_first_utf8_byte(uint8_t byte)
{
    return (byte & 0xc0) != 0x80;
}

inline constexpr bool is_utf8_continuation_byte(uint8_t byte)
{
    return (byte & 0xc0) == 0x80;
}

inline constexpr bool is_utf16_high_surrogate(uint32_t character)
{
    return character >= 0xd800 && character <= 0xdbff;
}

inline constexpr bool is_utf16_low_surrogate(uint32_t character)
{
    return character >= 0xdc00 && character <= 0xdfff;
}

inline constexpr bool is_unicode_scalar_value(uint32_t character)
{
    return character <= UNICODE_MAX_CODE_POINT && !is_utf16_high_surrogate(character) && !is_utf16_low_surrogate(character);
}

inline constexpr int utf8_char_length(uint8_t firstByte)
{
    return int(((0xe5000000u >> ((firstByte >> 3) & 0x1e)) & 3) + 1);
}

// Preconditions: source points to a non-empty buffer and sourceLength is greater than zero.
// This function prevents truncated reads, but assumes that complete sequences contain valid UTF-8.
inline constexpr uint32_t utf8_to_unicode(const char* source, size_t sourceLength, size_t& outByteLength)
{
    const uint32_t firstByte = uint8_t(source[0]);
    if ((firstByte & 0x80) == 0)
    {
        outByteLength = 1;
        return firstByte;
    }
    else if ((firstByte & 0x20) == 0)
    {
        if (sourceLength < 2)
        {
            outByteLength = 1;
            return UNICODE_REPLACEMENT_CHARACTER;
        }
        const uint32_t secondByte = uint8_t(source[1]);
        outByteLength = 2;
        return ((firstByte & 0x1f) << 6) | (secondByte & 0x3f);
    }
    else if ((firstByte & 0x10) == 0)
    {
        if (sourceLength < 3)
        {
            outByteLength = 1;
            return UNICODE_REPLACEMENT_CHARACTER;
        }
        const uint32_t secondByte = uint8_t(source[1]);
        const uint32_t thirdByte = uint8_t(source[2]);
        outByteLength = 3;
        return ((firstByte & 0x0f) << 12) | ((secondByte & 0x3f) << 6) | (thirdByte & 0x3f);
    }
    else
    {
        if (sourceLength < 4)
        {
            outByteLength = 1;
            return UNICODE_REPLACEMENT_CHARACTER;
        }
        const uint32_t secondByte = uint8_t(source[1]);
        const uint32_t thirdByte = uint8_t(source[2]);
        const uint32_t fourthByte = uint8_t(source[3]);
        outByteLength = 4;
        return ((firstByte & 0x07) << 18) | ((secondByte & 0x3f) << 12) | ((thirdByte & 0x3f) << 6) | (fourthByte & 0x3f);
    }
}

inline constexpr uint32_t utf8_to_unicode(const char* source)
{
    size_t byteLength = 0;
    return utf8_to_unicode(source, UTF8_MAX_CHARACTER_LENGTH, byteLength);
}

inline constexpr size_t utf8_valid_sequence_length(const char* source, size_t sourceLength)
{
    const uint8_t firstByte = uint8_t(source[0]);
    if (firstByte < 0x80)
    {
        return 1;
    }
    else if (firstByte < 0xc2)
    {
        return 0;
    }
    else if (firstByte < 0xe0)
    {
        return (sourceLength >= 2 && is_utf8_continuation_byte(uint8_t(source[1]))) ? 2 : 0;
    }
    else if (firstByte < 0xf0)
    {
        const bool validSequence = sourceLength >= 3 && is_utf8_continuation_byte(uint8_t(source[1])) && is_utf8_continuation_byte(uint8_t(source[2])) &&
                                   (firstByte != 0xe0 || uint8_t(source[1]) >= 0xa0) && (firstByte != 0xed || uint8_t(source[1]) < 0xa0);
        return validSequence ? 3 : 0;
    }
    else if (firstByte <= 0xf4)
    {
        const bool validSequence = sourceLength >= 4 && is_utf8_continuation_byte(uint8_t(source[1])) && is_utf8_continuation_byte(uint8_t(source[2])) && is_utf8_continuation_byte(uint8_t(source[3])) &&
                                   (firstByte != 0xf0 || uint8_t(source[1]) >= 0x90) && (firstByte != 0xf4 || uint8_t(source[1]) < 0x90);
        return validSequence ? 4 : 0;
    }
    else
    {
        return 0;
    }
}

// Preconditions: destination has room for UTF8_MAX_CHARACTER_LENGTH bytes and character is a Unicode scalar value.
inline constexpr size_t unicode_to_utf8(char* destination, uint32_t character)
{
    if (character < 0x80)
    {
        destination[0] = char(character);
        return 1;
    }
    else if (character < 0x800)
    {
        destination[0] = char(0xc0 | (character >> 6));
        destination[1] = char(0x80 | (character & 0x3f));
        return 2;
    }
    else if (character < 0x10000)
    {
        destination[0] = char(0xe0 | (character >> 12));
        destination[1] = char(0x80 | ((character >> 6) & 0x3f));
        destination[2] = char(0x80 | (character & 0x3f));
        return 3;
    }
    else
    {
        destination[0] = char(0xf0 | (character >> 18));
        destination[1] = char(0x80 | ((character >> 12) & 0x3f));
        destination[2] = char(0x80 | ((character >> 6) & 0x3f));
        destination[3] = char(0x80 | (character & 0x3f));
        return 4;
    }
}

class PUTF8CodePointIterator
{
public:
    using iterator_concept = std::forward_iterator_tag;
    using iterator_category = std::forward_iterator_tag;
    using value_type = uint32_t;
    using difference_type = ptrdiff_t;
    using pointer = void;
    using reference = uint32_t;

    constexpr PUTF8CodePointIterator() noexcept = default;
    constexpr PUTF8CodePointIterator(const char* position, const char* end) noexcept : m_Position(position), m_End(end) {}

    constexpr uint32_t operator*() const noexcept
    {
        size_t byteLength = 0;
        return utf8_to_unicode(m_Position, size_t(m_End - m_Position), byteLength);
    }

    constexpr PUTF8CodePointIterator& operator++() noexcept
    {
        const size_t remainingLength = size_t(m_End - m_Position);
        const size_t characterLength = size_t(utf8_char_length(uint8_t(m_Position[0])));
        m_Position += (characterLength < remainingLength) ? characterLength : remainingLength;
        return *this;
    }

    constexpr PUTF8CodePointIterator operator++(int) noexcept
    {
        PUTF8CodePointIterator previous = *this;
        ++(*this);
        return previous;
    }

    constexpr bool operator==(const PUTF8CodePointIterator& rhs) const noexcept
    {
        return m_Position == rhs.m_Position;
    }

private:
    const char* m_Position = nullptr;
    const char* m_End = nullptr;
};
