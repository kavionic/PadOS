// This file is part of PadOS.
//
// Copyright (c) 2018-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 15.04.2018 16:58:13

#include <algorithm>
#include <string.h>
#include <sys/stat.h>

#include "Math/Misc.h"
#include "Utils/String.h"
#include "Utils/UnicodeCaseFolding.h"
#include "Utils/UTF8Utils.h"
#include "Utils/Utils.h"

#include "UnicodeCaseMappingData.h"

PString PString::zero;

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <cmath>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString::PString(const wchar16_t* utf16, size_t length)
{
    assign_utf16(utf16, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::assign_utf16(const wchar16_t* source, size_t length)
{
    clear();

    if (source == nullptr) {
        return *this;
    }

    if (length == npos)
    {
        length = 0;
        while (source[length] != 0) {
            ++length;
        }
    }

    bool isASCII = true;
    for (size_t sourceIndex = 0; sourceIndex < length; ++sourceIndex)
    {
        if (source[sourceIndex] >= 0x80)
        {
            isASCII = false;
            break;
        }
    }

    if (isASCII)
    {
        resize_and_overwrite(length, [source, length](char* destination, size_t)
        {
            for (size_t sourceIndex = 0; sourceIndex < length; ++sourceIndex) {
                destination[sourceIndex] = char(source[sourceIndex]);
            }
            return length;
        });
        return *this;
    }

    size_t utf8Length = 0;
    for (size_t sourceIndex = 0; sourceIndex < length;)
    {
        const uint32_t character = decode_utf16_character(source, length, sourceIndex);
        const size_t characterLength = get_utf8_encoded_length(character);
        if (characterLength > max_size() - utf8Length) {
            throw std::length_error("UTF-16 string is too long");
        }
        utf8Length += characterLength;
    }

    resize_and_overwrite(utf8Length, [source, length](char* destination, size_t)
    {
        size_t destinationIndex = 0;
        for (size_t sourceIndex = 0; sourceIndex < length;)
        {
            const uint32_t character = decode_utf16_character(source, length, sourceIndex);
            destinationIndex += unicode_to_utf8(destination + destinationIndex, character);
        }
        return destinationIndex;
    });
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PString::copy_utf16(wchar16_t* dst, size_t length, size_t pos) const
{
    size_t outputPosition = 0;
    size_t inputPosition = pos;

    while (dst != nullptr && inputPosition < size() && outputPosition < length)
    {
        size_t byteLength = 0;
        const uint32_t character = utf8_to_unicode(data() + inputPosition, size() - inputPosition, byteLength);

        if (character < 0x10000)
        {
            dst[outputPosition++] = wchar16_t(character);
        }
        else
        {
            if (length - outputPosition < 2) {
                break;
            }
            const uint32_t supplementaryCharacter = character - 0x10000;
            dst[outputPosition++] = wchar16_t(0xd800 + (supplementaryCharacter >> 10));
            dst[outputPosition++] = wchar16_t(0xdc00 + (supplementaryCharacter & 0x3ff));
        }
        inputPosition += byteLength;
    }
    return outputPosition;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::append_utf32_char(uint32_t unicode)
{
    char utf8Buffer[UTF8_MAX_CHARACTER_LENGTH];
    size_t utf8Length = unicode_to_utf8(utf8Buffer, unicode);
    insert(end(), utf8Buffer, utf8Buffer + utf8Length);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString::utf32_iterator PString::utf32_begin() const noexcept
{
    return utf32_iterator(data(), data() + size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString::utf32_iterator PString::utf32_end() const noexcept
{
    return utf32_iterator(data() + size(), data() + size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::is_valid_utf8() const noexcept
{
    size_t byteOffset = 0;
    while (byteOffset < size())
    {
        const size_t characterLength = utf8_valid_sequence_length(data() + byteOffset, size() - byteOffset);
        if (characterLength == 0) {
            return false;
        }
        byteOffset += characterLength;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::sanitize_utf8()
{
    size_t byteOffset = 0;
    while (byteOffset < size())
    {
        const size_t characterLength = utf8_valid_sequence_length(data() + byteOffset, size() - byteOffset);
        if (characterLength == 0) {
            break;
        }
        byteOffset += characterLength;
    }

    if (byteOffset != size())
    {
        PString sanitizedString;
        sanitizedString.reserve(size());
        sanitizedString.append(data(), byteOffset);

        while (byteOffset < size())
        {
            const size_t characterLength = utf8_valid_sequence_length(data() + byteOffset, size() - byteOffset);
            if (characterLength != 0)
            {
                sanitizedString.append(data() + byteOffset, characterLength);
                byteOffset += characterLength;
            }
            else
            {
                sanitizedString.append_utf32_char(UNICODE_REPLACEMENT_CHARACTER);
                ++byteOffset;
            }
        }
        swap(sanitizedString);
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PString::compare_nocase(const std::string& rhs) const
{
    return unicode_case_fold_compare(data(), data() + size(), rhs.data(), rhs.data() + rhs.size());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PString::compare_nocase(const char* rhs) const
{
    return unicode_case_fold_compare(data(), data() + size(), rhs, rhs + strlen(rhs));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::starts_with(const char* token, size_t length /*= INVALID_INDEX*/) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    if (length <= size()) {
        return compare(0, length, token, length) == 0;
    } else {
        return false;
    }

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::starts_with_nocase(const char* token, size_t length /*= INVALID_INDEX*/) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    return unicode_case_fold_starts_with(data(), data() + size(), token, token + length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::ends_with(const char* token, size_t length) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    if (length <= size()) {
        return compare(size() - length, length, token, length) == 0;
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::ends_with_nocase(const char* token, size_t length) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    return unicode_case_fold_ends_with(data(), data() + size(), token, token + length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::containes(const char* token, size_t length) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    return find(token, 0, length) != npos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::containes_nocase(const char* token, size_t length) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    return unicode_case_fold_contains(data(), data() + size(), token, token + length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PString::count_chars() const
{
    size_t numChars = 0;
    for (size_t byteOffset = 0; byteOffset < size(); byteOffset += size_t(utf8_char_length(uint8_t((*this)[byteOffset])))) {
        ++numChars;
    }
    return numChars;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::strip()
{
    rstrip();
    lstrip();
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::lstrip()
{
    std::string::iterator i;

    for (i = begin(); i != end(); ++i)
    {
        if (!isspace(*i)) {
            break;
        }
    }
    erase(begin(), i);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::rstrip()
{
    size_t newLength = size();
    while (newLength > 0 && isspace((*this)[newLength - 1])) {
        --newLength;
    }
    resize(newLength);
    return *this;
}

/**
 * @brief Convert this UTF-8 string to lowercase in place.
 *
 * Applies the full Unicode 17.0 default lowercase mappings, independently of
 * the current locale. Mappings can produce multiple code points; for example,
 * U+0130 becomes U+0069 followed by U+0307.
 *
 * Greek capital sigma (U+03A3) becomes final sigma (U+03C2) if the nearest
 * preceding character is cased and the nearest following character is absent or
 * uncased, after skipping characters with the Unicode Case_Ignorable property.
 * Otherwise, it becomes small sigma (U+03C3).
 *
 * Processes the entire string, including text after embedded null bytes.
 * Characters without a mapping are preserved. The UTF-8 byte length and number
 * of code points may change. Reallocation may invalidate pointers, references,
 * and iterators.
 *
 * @pre The string contains valid UTF-8.
 * @return A reference to this string (@c *this).
 * @see upper(), is_valid_utf8(), sanitize_utf8()
 *
 * \author Kurt Skauen
 *****************************************************************************/

PString& PString::lower()
{
    ConvertStringCase<false>(*this);
    return *this;
}

/**
 * @brief Convert this UTF-8 string to uppercase in place.
 *
 * Applies the full Unicode 17.0 default uppercase mappings, independently of
 * the current locale. Mappings can produce multiple code points; for example,
 * sharp s (U+00DF) becomes "SS" and the ffi ligature (U+FB03) becomes "FFI".
 *
 * Processes the entire string, including text after embedded null bytes.
 * Characters without a mapping are preserved. The UTF-8 byte length and number
 * of code points may change. Reallocation may invalidate pointers, references,
 * and iterators.
 *
 * @pre The string contains valid UTF-8.
 * @return A reference to this string (@c *this).
 * @see lower(), is_valid_utf8(), sanitize_utf8()
 *
 * \author Kurt Skauen
 *****************************************************************************/

PString& PString::upper()
{
    ConvertStringCase<true>(*this);
    return *this;
}

/**
 * @brief Format a byte count as a human-readable byte size string.
 *
 * Converts a byte count into a compact, human-readable representation
 * using SI or IEC unit prefixes. The function automatically selects the
 * largest suitable unit and formats the value according to the requested
 * precision mode.
 *
 * Precision control is specified by @p numDecimalsOrDigits:
 *   - If @p numDecimalsOrDigits == 0, the value is formatted as an integer
 *     without fractional digits.
 *   - If @p numDecimalsOrDigits > 0, the value is formatted with a fixed
 *     number of fractional decimal places.
 *   - If @p numDecimalsOrDigits < 0, the absolute value specifies the
 *     number of significant digits to retain.
 *
 * In significant-digits mode, the integer part is never truncated; if the
 * integer part already contains the requested number of digits or more,
 * no fractional digits are shown.
 *
 * Unit selection and formatting depend on @p unitSystem:
 *   - PUnitSystem::SI:
 *       * Uses decimal units (1000^n) with prefixes: k, M, G, T, P, E
 *   - PUnitSystem::IEC:
 *       * Uses binary units (1024^n) with prefixes: K, M, G, T, P, E
 *   - PUnitSystem::Auto:
 *       * Equivalent to IEC for formatting (binary units)
 *
 * Notes:
 *   - The largest unit not exceeding the absolute value is selected.
 *   - Values are rounded to the nearest representable value
 *     (half away from zero).
 *   - Fractional formatting is suppressed for the base unit (bytes).
 *   - The number of fractional digits is clamped internally to a
 *     reasonable maximum to avoid excessive precision.
 *   - Negative values are supported and formatted with a leading '-'.
 *
 * @param size               Size in bytes.
 * @param numDecimalsOrDigits
 *        Precision control:
 *          - 0  : integer output
 *          - >0 : fixed number of decimal places
 *          - <0 : number of significant digits
 * @param unitSystem         Unit system to use (Auto, SI, or IEC).
 *
 * @return A formatted, human-readable byte-size string.
 *
 * \author Kurt Skauen
 *****************************************************************************/

PString PString::format_byte_size(int64_t size, int numDecimalsOrDigits, PUnitSystem unitSystem)
{
    const uint64_t absSize = PMath::signed_to_unsigned_abs(size);

    const int64_t kiloSize = (unitSystem == PUnitSystem::SI) ? 1000 : 1024;
    const int64_t nextToMaxScale = (unitSystem == PUnitSystem::SI)
        ? (std::numeric_limits<int64_t>::max() / 1000)
        : (std::numeric_limits<int64_t>::max() / 1024);

    static constexpr const char* siUnits[]  = { "", "k", "M", "G", "T", "P", "E" };
    static constexpr const char* iecUnits[] = { "", "K", "M", "G", "T", "P", "E" };
    static_assert(ARRAY_COUNT(siUnits) == ARRAY_COUNT(iecUnits));

    const char* const* const units = (unitSystem == PUnitSystem::SI) ? siUnits : iecUnits;
    static constexpr size_t unitsCount = ARRAY_COUNT(siUnits);

    int64_t scale = 1;
    size_t unit = 0;

    while (unit + 1 < unitsCount)
    {
        if (scale > nextToMaxScale) {
            break;
        }
        const int64_t nextScale = scale * kiloSize;
        if (absSize < uint64_t(nextScale)) {
            break;
        }
        scale = nextScale;
        ++unit;
    }

    // Integer only:
    if (numDecimalsOrDigits == 0 || unit == 0) {
        return format_string("{}{}", size / scale, units[unit]);
    }

    double value = double(size) / double(scale);

    int decimals = 0;

    if (numDecimalsOrDigits >= 0)
    {
        // Fixed decimals mode.
        decimals = std::min(12, numDecimalsOrDigits);
    }
    else
    {
        // Significant digits mode: -N means "keep N significant digits"
        const int digits = -std::max(-12, numDecimalsOrDigits);

        const double absValue = std::abs(value);

        // Determine how many integer digits absValue has. Values < 1 threated as 1 digit.
        const int intDigits = (absValue >= 1.0) ? (int(std::floor(std::log10(absValue))) + 1) : 1;

        // If the integer part already has >= digits, show no decimals (don't truncate integer part).
        decimals = std::max(0, digits - intDigits);
    }

    if (decimals > 0)
    {
        const double p = std::pow(10.0, double(decimals));
        value = std::round(value * p) / p;
        if (value == 0.0) value = 0.0; // Normalize -0.0
    }

    return format_string("{:.{}f}{}", value, decimals, units[unit]);
}

/**
 * @brief Format a duration as a human-readable time period.
 *
 * Formats years, days, hours, minutes, and seconds, starting with the largest
 * applicable unit. Uses 365-day years and 24-hour days. Smaller fields are
 * included even when zero, unless omitted due to @p maxCharacters.
 *
 * Durations below one minute use seconds with one decimal place. At one
 * minute or more, fractional seconds are discarded; hours, minutes, and
 * seconds use two digits. Days and years are not zero-padded.
 *
 * With @p includeUnits enabled, appends y, d, h, m, and s to the respective
 * fields without separators. Otherwise, separates fields with colons. For
 * example, 90.5 seconds becomes "01m30s" or "01:30", respectively.
 *
 * Fields are appended from largest to smallest. If the next complete field,
 * including its suffix or separator, would exceed @p maxCharacters, that
 * field and all smaller fields are omitted without rounding. The years
 * field and a seconds-only result are always emitted, even if they exceed
 * the limit. A zero limit disables length limiting.
 *
 * @param timeVal Duration to format.
 * @param includeUnits Whether to append unit suffixes instead of using
 *                     colon separators.
 * @param maxCharacters Preferred maximum output length, including suffixes
 *                      or separators; zero means unlimited.
 *
 * @pre @p timeVal is non-negative and its whole-second value fits in
 *      uint32_t, as required by the current field calculations.
 * @return The formatted duration, possibly empty if the first day, hour,
 *         or minute field does not fit within @p maxCharacters.
 *
 * \author Kurt Skauen
 *****************************************************************************/

PString PString::format_time_period(const TimeValNanos& timeVal, bool includeUnits, size_t maxCharacters)
{
    if (maxCharacters == 0) maxCharacters = std::numeric_limits<size_t>::max();

    bigtime_t seconds = timeVal.AsSecondsI();

    char secondStr[6] = { 0 };
    char minuteStr[4] = { 0 };
    char hourStr[4]   = { 0 };
    char dayStr[5]    = { 0 };
    char yearStr[16]  = { 0 }; // Max years in a TimeValNanos: 292

    if (seconds > -60 && seconds < 60)
    {
        sprintf(secondStr, "%2.1f", timeVal.AsSecondsF());
        if (includeUnits) strcat(secondStr, "s");
    }
    else
    {
        sprintf(secondStr, "%02lu", uint32_t(seconds) % 60);
        if (includeUnits) strcat(secondStr, "s");
        if (seconds >= 60LL)
        {
            sprintf(minuteStr, "%02lu", uint32_t(seconds) / 60 % 60);
            if (includeUnits) strcat(minuteStr, "m");
            if (seconds >= 60LL * 60LL)
            {
                sprintf(hourStr, "%02lu", uint32_t(seconds) / 60 / 60 % 24);
                if (includeUnits) strcat(hourStr, "h");
                if (seconds >= 60LL * 60LL * 24LL)
                {
                    sprintf(dayStr, "%lu", uint32_t(seconds) / 60 / 60 / 24 % 365);
                    if (includeUnits) strcat(dayStr, "d");
                    if (seconds >= 60LL * 60LL * 24LL * 365LL)
                    {
                        sprintf(yearStr, "%" PRIu64, seconds / 60 / 60 / 24 / 365);
                        if (includeUnits) strcat(yearStr, "y");
                    }
                }
            }
        }
    }
    PString result;
    if (yearStr[0] != '\0') {
        result += yearStr;
    }
    if (dayStr[0] != '\0')
    {
        bool addColon = !includeUnits && !result.empty();
        if (result.size() + strlen(dayStr) + (addColon ? 1 : 0) > maxCharacters) return result;
        if (addColon) result += ":";
        result += dayStr;
    }
    if (hourStr[0] != '\0')
    {
        bool addColon = !includeUnits && !result.empty();
        if (result.size() + strlen(hourStr) + (addColon ? 1 : 0) > maxCharacters) return result;
        if (addColon) result += ":";
        result += hourStr;
    }
    if (minuteStr[0] != '\0')
    {
        bool addColon = !includeUnits && !result.empty();
        if (result.size() + strlen(minuteStr) + (addColon ? 1 : 0) > maxCharacters) return result;
        if (addColon) result += ":";
        result += minuteStr;
    }
    bool addColon = !includeUnits && !result.empty();
    if (!result.empty() && (result.size() + strlen(secondStr) + (addColon ? 1 : 0) > maxCharacters)) return result;
    if (addColon) result += ":";
    result += secondStr;
    return result;
}

/**
 * @brief Format POSIX file mode bits as a symbolic permission string.
 *
 * Converts a POSIX-style file mode value into a human-readable
 * 10-character permission string of the form:
 *
 *     "drwxr-xr-x"
 *
 * The first character indicates the file type:
 *   - 'b'  Block device
 *   - 'c'  Character device
 *   - 'd'  Directory
 *   - 'p'  FIFO / pipe
 *   - 'l'  Symbolic link
 *   - '-'  Regular file
 *   - 's'  Socket
 *   - '?'  Unknown or unsupported type
 *
 * The remaining nine characters represent the read, write, and execute
 * permissions for the owner, group, and others, in that order.
 *
 * Special permission bits are rendered using the conventional symbolic
 * semantics:
 *   - SUID replaces the user execute bit with:
 *       's' if execute is set, 'S' if execute is not set
 *   - SGID replaces the group execute bit with:
 *       's' if execute is set, 'S' if execute is not set
 *   - Sticky bit (ISVTX) replaces the other execute bit with:
 *       't' if execute is set, 'T' if execute is not set
 *
 * @param mode  File mode value (typically stat::st_mode).
 *
 * @return A PString containing the formatted permission representation.
 *
 * \author Kurt Skauen
 *****************************************************************************/

PString PString::format_file_permissions(mode_t mode)
{
    PString text;

    switch(mode & S_IFMT)
    {
        case S_IFBLK:  text = "b";  break;
        case S_IFCHR:  text = "c";  break;
        case S_IFDIR:  text = "d";  break;
        case S_IFIFO:  text = "p";  break;
        case S_IFLNK:  text = "l";  break;
        case S_IFREG:  text = "-";  break;
        case S_IFSOCK: text = "s";  break;
        default:       text = "?";  break;
    }

    // User
    text += (mode & S_IRUSR) ? "r" : "-";
    text += (mode & S_IWUSR) ? "w" : "-";
    if (mode & S_ISUID) {
        text += (mode & S_IXUSR) ? "s" : "S";
    } else {
        text += (mode & S_IXUSR) ? "x" : "-";
    }

    // Group
    text += (mode & S_IRGRP) ? "r" : "-";
    text += (mode & S_IWGRP) ? "w" : "-";
    if (mode & S_ISGID) {
        text += (mode & S_IXGRP) ? "s" : "S";
    } else {
        text += (mode & S_IXGRP) ? "x" : "-";
    }

    // Other
    text += (mode & S_IROTH) ? "r" : "-";
    text += (mode & S_IWOTH) ? "w" : "-";
    if (mode & S_ISVTX) {
        text += (mode & S_IXOTH) ? "t" : "T";
    } else {
        text += (mode & S_IXOTH) ? "x" : "-";
    }
    return text;
}

/**
 * @brief Parse a human-readable byte-size string into a byte count.
 *
 * Parses size strings using common SI and IEC unit suffixes, such as
 * "10K", "10KB", "1.5MiB", or "42". The function returns the size in bytes,
 * or std::nullopt if the string is invalid or the result would overflow
 * int64_t.
 *
 * Supported forms:
 *   - Integer values:        "42", "-1024"
 *   - Fractional values:     "1.5K", "0.5MiB"
 *   - Optional whitespace:   "  10   KB  "
 *
 * Supported suffixes:
 *   - Binary (IEC-style):    K, M, G, T, P, E   or  KiB, MiB, GiB, TiB, PiB, EiB
 *   - Decimal (SI-style):    KB, MB, GB, TB, PB, EB
 *
 * Interpretation depends on @p unitSystem:
 *   - PUnitSystem::Auto:
 *       * K / M / G / ... and KiB / MiB / ... are treated as binary (1024^n)
 *       * KB / MB / ... are treated as decimal (1000^n)
 *   - PUnitSystem::SI:
 *       * All recognized suffixes are treated as decimal (1000^n)
 *   - PUnitSystem::IEC:
 *       * All recognized suffixes are treated as binary (1024^n)
 *
 * Notes:
 *   - No suffix means bytes.
 *   - Scientific notation (e.g. "1e3") is not supported.
 *   - Fractional values are rounded to the nearest integer
 *     (half away from zero).
 *   - Parsing is case-insensitive for suffixes.
 *
 * @param text        The input string to parse.
 * @param unitSystem  Unit interpretation mode (Auto, SI, or IEC).
 *
 * @return The parsed size in bytes, or std::nullopt on error.
 * 
 * \author Kurt Skauen
 *****************************************************************************/

std::optional<int64_t> PString::parse_byte_size(const PString& text, PUnitSystem unitSystem)
{
    PString strippedText = text;
    strippedText.strip();

    if (strippedText.empty()) {
        return std::nullopt;
    }
    const size_t suffixStart = find_suffix_start(strippedText);

    const PString numPart = PString(strippedText.substr(0, suffixStart)).rstrip();
    const PString sufPart = PString(strippedText.substr(suffixStart)).upper();

    if (numPart.empty()) {
        return std::nullopt;
    }

    // Parse suffix -> exponent + implied unit system
    PUnitSystem valueUnitSystem = PUnitSystem::IEC;
    const std::optional<int> expOpt = parse_suffix(sufPart, valueUnitSystem);
    if (!expOpt) {
        return std::nullopt;
    }
    const int exponent = *expOpt;

    if (unitSystem == PUnitSystem::Auto) {
        unitSystem = valueUnitSystem;
    }
    const uint64_t base = (exponent == 0) ? 1ULL : (unitSystem == PUnitSystem::SI ? 1000ULL : 1024ULL);

    if (!numPart.contains('.'))
    {
        // No decimal point. Use integer math.
        int64_t mantissa = 0;

        const char* numStart = numPart.data();
        const char* numEnd = numPart.data() + numPart.size();
        auto [ptr, ec] = std::from_chars(numStart, numEnd, mantissa, 10);

        if (ec != std::errc{} || ptr != numEnd) {
            return std::nullopt;
        }

        // Scale exactly with overflow checks into int64.
        const bool isNegative = (mantissa < 0);
        const uint64_t limitMag = isNegative
            ? (uint64_t(std::numeric_limits<int64_t>::max()) + 1ULL)
            : uint64_t(std::numeric_limits<int64_t>::max());

        // Safe abs into uint64_t
        uint64_t mag = PMath::signed_to_unsigned_abs(mantissa); // isNegative ? (uint64_t(-(mantissa + 1)) + 1ULL) : uint64_t(mantissa);

        if (exponent != 0)
        {
            const uint64_t factor = PMath::pow_u64(base, exponent); // exp<=8 safe
            if (!PMath::checked_mul_u64(mag, factor, limitMag)) {
                return std::nullopt;
            }
        }

        if (isNegative)
        {
            if (mag == uint64_t(std::numeric_limits<int64_t>::max()) + 1ULL) {
                return std::numeric_limits<int64_t>::min();
            }
            return -int64_t(mag);
        }
        return int64_t(mag);
    }
    else
    {
        // With decimal point. Use floating point math.
        double mantissa = 0.0;

        const char* numStart = numPart.data();
        const char* numEnd = numPart.data() + numPart.size();
        auto [ptr, ec] = std::from_chars(numStart, numEnd, mantissa);

        if (ec != std::errc{} || ptr != numEnd) {
            return std::nullopt;
        }

        if (!std::isfinite(mantissa)) {
            return std::nullopt;
        }

        const double scale = (exponent != 0) ? std::pow(double(base), double(exponent)) : 1.0;
        const double scaled = mantissa * scale;
        const double rounded = std::round(scaled);

        // Range-check before casting.
        constexpr double minI = double(std::numeric_limits<int64_t>::min());
        constexpr double maxI = double(std::numeric_limits<int64_t>::max());

        if (!(rounded >= minI && rounded <= maxI)) {
            return std::nullopt;
        }

        return int64_t(rounded);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

std::optional<int> PString::prefix_to_exp(char prefixUpper)
{
    switch (prefixUpper)
    {
        case 'K': return 1;
        case 'M': return 2;
        case 'G': return 3;
        case 'T': return 4;
        case 'P': return 5;
        case 'E': return 6;
        case 'Z': return 7;
        case 'Y': return 8;
        default:  return std::nullopt;
    }
}

/**
 * Returns exponent (0 for none), nullopt on invalid suffix.
 * outUnitSystem:
 *     SI for "KB/MB/..."
 *     IEC for everything else ("K", "KiB", ...)
 * 
 * \author Kurt Skauen
 *****************************************************************************/

std::optional<int> PString::parse_suffix(const PString& suffixIn, PUnitSystem& outUnitSystem)
{
    outUnitSystem = PUnitSystem::IEC;

    if (suffixIn.empty()) {
        return 0;
    }

    const std::optional<int> expOpt = prefix_to_exp(suffixIn[0]);
    if (!expOpt) return std::nullopt;
    const int exp = *expOpt;

    const size_t remLen = suffixIn.size() - 1;

    if (remLen == 0) {                 // "K"
        outUnitSystem = PUnitSystem::IEC;
        return exp;
    }
    if (remLen == 1 && suffixIn[1] == 'B') {  // "KB"
        outUnitSystem = PUnitSystem::SI;
        return exp;
    }
    if (remLen == 2 && suffixIn[1] == 'I' && suffixIn[2] == 'B') { // "KiB"
        outUnitSystem = PUnitSystem::IEC;
        return exp;
    }
    return std::nullopt;
}

/**
 * Find the split between numeric prefix and suffix.
 * Numeric part may contain digits, whitespace, '.', '+', '-'
 * 
 * \author Kurt Skauen
 *****************************************************************************/

size_t PString::find_suffix_start(const PString& text)
{
    size_t i = 0;
    while (i < text.size())
    {
        const char c = text[i];
        const bool ok =
            (c >= '0' && c <= '9') ||
            c == '.' || c == '+' || c == '-' ||
            c == ' ' || c == '\t';
        if (!ok) break;
        ++i;
    }
    return i;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::HasUnicodeCasingProperty(uint32_t character, const UnicodeCasingPropertyRange* ranges, size_t rangeCount) noexcept
{
    size_t rangeBegin = 0;
    size_t rangeEnd = rangeCount;
    while (rangeBegin < rangeEnd)
    {
        const size_t rangeIndex = rangeBegin + (rangeEnd - rangeBegin) / 2;
        const UnicodeCasingPropertyRange& range = ranges[rangeIndex];
        if (character < range.First) {
            rangeEnd = rangeIndex;
        } else if (character > range.Last) {
            rangeBegin = rangeIndex + 1;
        } else {
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint32_t PString::ReadPreviousCaseConvertedCharacter(
    std::string_view written,
    std::string_view overflow,
    size_t& byteOffset) noexcept
{
    const size_t previousEnd = byteOffset;
    --byteOffset;
    while (byteOffset > 0)
    {
        const char byte = (byteOffset < written.size()) ? written[byteOffset] : overflow[byteOffset - written.size()];
        if (!is_utf8_continuation_byte(uint8_t(byte))) {
            break;
        }
        --byteOffset;
    }

    char splitCharacter[UTF8_MAX_CHARACTER_LENGTH];
    const char* characterData;
    if (previousEnd <= written.size())
    {
        characterData = written.data() + byteOffset;
    }
    else if (byteOffset >= written.size())
    {
        characterData = overflow.data() + byteOffset - written.size();
    }
    else
    {
        const size_t writtenLength = written.size() - byteOffset;
        memcpy(splitCharacter, written.data() + byteOffset, writtenLength);
        memcpy(splitCharacter + writtenLength, overflow.data(), previousEnd - written.size());
        characterData = splitCharacter;
    }
    size_t byteLength = 0;
    return utf8_to_unicode(characterData, previousEnd - byteOffset, byteLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::IsUnicodeFinalSigma(std::string_view written, std::string_view overflow, std::string_view following) noexcept
{
    // The generator verifies that default lowercasing preserves sigma context in the converted prefix.
    bool precededByCasedCharacter = false;
    size_t previousOffset = written.size() + overflow.size();
    while (previousOffset > 0)
    {
        const uint32_t character = ReadPreviousCaseConvertedCharacter(written, overflow, previousOffset);
        if (!HasUnicodeCasingProperty(character, UNICODE_CASE_IGNORABLE_RANGES, std::size(UNICODE_CASE_IGNORABLE_RANGES)))
        {
            precededByCasedCharacter = HasUnicodeCasingProperty(character, UNICODE_CASED_RANGES, std::size(UNICODE_CASED_RANGES));
            break;
        }
    }
    if (!precededByCasedCharacter) {
        return false;
    }

    size_t followingOffset = 0;
    while (followingOffset < following.size())
    {
        size_t followingLength = 0;
        const uint32_t character = utf8_to_unicode(
            following.data() + followingOffset,
            following.size() - followingOffset,
            followingLength);
        if (!HasUnicodeCasingProperty(character, UNICODE_CASE_IGNORABLE_RANGES, std::size(UNICODE_CASE_IGNORABLE_RANGES))) {
            return !HasUnicodeCasingProperty(character, UNICODE_CASED_RANGES, std::size(UNICODE_CASED_RANGES));
        }
        followingOffset += followingLength;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<bool TO_UPPER>
uint32_t PString::ConvertASCIICharacterCase(uint32_t character) noexcept
{
    constexpr uint32_t FIRST = TO_UPPER ? 'a' : 'A';
    constexpr uint32_t LAST = TO_UPPER ? 'z' : 'Z';
    constexpr int32_t DELTA = TO_UPPER ? ('A' - 'a') : ('a' - 'A');
    return (character >= FIRST && character <= LAST) ? uint32_t(int32_t(character) + DELTA) : character;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<bool TO_UPPER>
size_t PString::MapUnicodeCharacterCase(uint32_t character, uint32_t* mappedCharacters) noexcept
{
    constexpr const UnicodeCaseMappingRange* RANGES = TO_UPPER ? UNICODE_UPPER_RANGES : UNICODE_LOWER_RANGES;
    size_t rangeBegin = 0;
    size_t rangeEnd = TO_UPPER ? std::size(UNICODE_UPPER_RANGES) : std::size(UNICODE_LOWER_RANGES);
    while (rangeBegin < rangeEnd)
    {
        const size_t rangeIndex = rangeBegin + (rangeEnd - rangeBegin) / 2;
        const UnicodeCaseMappingRange& range = RANGES[rangeIndex];
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
            if (range.Stride == 1 || ((character - range.First) & 1) == 0)
            {
                mappedCharacters[0] = uint32_t(int32_t(character) + range.Delta);
                return 1;
            }
            break;
        }
    }

    constexpr const UnicodeCaseExpansion* EXPANSIONS = TO_UPPER ? UNICODE_UPPER_EXPANSIONS : UNICODE_LOWER_EXPANSIONS;
    constexpr const uint32_t* EXPANSION_CHARACTERS =
        TO_UPPER ? UNICODE_UPPER_EXPANSION_CHARACTERS : UNICODE_LOWER_EXPANSION_CHARACTERS;
    size_t expansionBegin = 0;
    size_t expansionEnd = TO_UPPER ? std::size(UNICODE_UPPER_EXPANSIONS) : std::size(UNICODE_LOWER_EXPANSIONS);
    while (expansionBegin < expansionEnd)
    {
        const size_t expansionIndex = expansionBegin + (expansionEnd - expansionBegin) / 2;
        const UnicodeCaseExpansion& expansion = EXPANSIONS[expansionIndex];
        if (character < expansion.Source)
        {
            expansionEnd = expansionIndex;
        }
        else if (character > expansion.Source)
        {
            expansionBegin = expansionIndex + 1;
        }
        else
        {
            for (size_t characterIndex = 0; characterIndex < expansion.Count; ++characterIndex) {
                mappedCharacters[characterIndex] = EXPANSION_CHARACTERS[expansion.Offset + characterIndex];
            }
            return expansion.Count;
        }
    }
    mappedCharacters[0] = character;
    return 1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<bool TO_UPPER>
size_t PString::EncodeCaseConvertedCharacter(uint32_t character, bool finalSigma, char* encodedCharacter) noexcept
{
    if (character < 0x80)
    {
        encodedCharacter[0] = char(ConvertASCIICharacterCase<TO_UPPER>(character));
        return 1;
    }

    uint32_t mappedCharacters[UNICODE_CASE_MAPPING_MAX_EXPANSION_LENGTH];
    size_t mappedCount = 1;
    if (!TO_UPPER && finalSigma) {
        mappedCharacters[0] = 0x03c2;
    } else {
        mappedCount = MapUnicodeCharacterCase<TO_UPPER>(character, mappedCharacters);
    }
    size_t encodedLength = 0;
    for (size_t characterIndex = 0; characterIndex < mappedCount; ++characterIndex) {
        encodedLength += unicode_to_utf8(encodedCharacter + encodedLength, mappedCharacters[characterIndex]);
    }
    return encodedLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PString::WriteCaseConversionOverflow(
    PString& source,
    size_t& writeOffset,
    size_t readOffset,
    const char* convertedBytes,
    size_t convertedLength,
    PString& overflow,
    size_t& overflowOffset)
{
    const size_t pendingLength = overflow.size() - overflowOffset;
    const size_t flushLength = std::min(pendingLength, readOffset - writeOffset);
    if (flushLength != 0)
    {
        memcpy(source.data() + writeOffset, overflow.data() + overflowOffset, flushLength);
        writeOffset += flushLength;
        overflowOffset += flushLength;
    }
    if (overflowOffset == overflow.size())
    {
        overflow.clear();
        overflowOffset = 0;
    }

    const size_t directLength = std::min(convertedLength, readOffset - writeOffset);
    if (directLength != 0)
    {
        memcpy(source.data() + writeOffset, convertedBytes, directLength);
        writeOffset += directLength;
    }
    if (directLength < convertedLength)
    {
        // Reclaim consumed bytes only when at least half the buffer is consumed, keeping total copying linear.
        if (overflowOffset != 0 && overflowOffset >= overflow.size() - overflowOffset)
        {
            overflow.erase(0, overflowOffset);
            overflowOffset = 0;
        }
        overflow.append(convertedBytes + directLength, convertedLength - directLength);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<bool TO_UPPER>
void PString::ConvertStringCase(PString& source)
{
    size_t readOffset = 0;
    while (readOffset < source.size() && uint8_t(source[readOffset]) < 0x80)
    {
        source[readOffset] = char(ConvertASCIICharacterCase<TO_UPPER>(uint8_t(source[readOffset])));
        ++readOffset;
    }
    if (readOffset == source.size()) {
        return;
    }

    size_t writeOffset = readOffset;
    PString overflow;
    size_t overflowOffset = 0;
    while (readOffset < source.size())
    {
        size_t byteLength = 0;
        const uint32_t character = utf8_to_unicode(source.data() + readOffset, source.size() - readOffset, byteLength);
        readOffset += byteLength;
        bool finalSigma = false;
        if constexpr (!TO_UPPER)
        {
            if (character == 0x03a3)
            {
                finalSigma = IsUnicodeFinalSigma(
                    std::string_view(source.data(), writeOffset),
                    std::string_view(overflow.data() + overflowOffset, overflow.size() - overflowOffset),
                    std::string_view(source.data() + readOffset, source.size() - readOffset));
            }
        }
        char convertedBytes[UNICODE_CASE_MAPPING_MAX_EXPANSION_LENGTH * UTF8_MAX_CHARACTER_LENGTH];
        const size_t convertedLength = EncodeCaseConvertedCharacter<TO_UPPER>(character, finalSigma, convertedBytes);

        if (overflow.empty() && convertedLength <= readOffset - writeOffset)
        {
            memcpy(source.data() + writeOffset, convertedBytes, convertedLength);
            writeOffset += convertedLength;
        }
        else
        {
            WriteCaseConversionOverflow(
                source,
                writeOffset,
                readOffset,
                convertedBytes,
                convertedLength,
                overflow,
                overflowOffset);
        }
    }
    if (overflow.empty())
    {
        if (writeOffset != source.size()) {
            source.resize(writeOffset);
        }
    }
    else
    {
        // Pending output implies all original bytes have been filled; grow the source only once, after decoding finishes.
        source.append(overflow.data() + overflowOffset, overflow.size() - overflowOffset);
    }
}
