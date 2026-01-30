// This file is part of PadOS.
//
// Copyright (C) 2018-2021 Kurt Skauen <http://kavionic.com/>
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
// Created: 15.04.2018 16:58:13

#include <strings.h>
#include <string.h>

#include "Utils/String.h"
#include "Utils/UTF8Utils.h"

PString PString::zero;

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
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
    reserve(length);
    for (size_t i = 0; i < length; ++i)
    {
        char utf8Buf[8];
        size_t utf8Length = unicode_to_utf8(utf8Buf, source[i]);
        insert(end(), utf8Buf, utf8Buf + utf8Length);
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PString::copy_utf16(wchar16_t* dst, size_t length, size_t pos) const
{
    size_t outPos = 0;
    for (size_t i = pos; i < size() && outPos < length; i += utf8_char_length(at(i)))
    {
        dst[outPos++] = wchar16_t(utf8_to_unicode(data() + i));
    }
    return outPos;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::append(uint32_t unicode)
{
    char utf8Buf[8];
    size_t utf8Length = unicode_to_utf8(utf8Buf, unicode);
    insert(end(), utf8Buf, utf8Buf + utf8Length);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PString::compare_nocase(const std::string& rhs) const
{
    return strcasecmp(c_str(), rhs.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int PString::compare_nocase(const char* rhs) const
{
    return strcasecmp(c_str(), rhs);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::starts_with(const char* token, size_t length /*= INVALID_INDEX*/) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    if (length <= size()) {
        return strncmp(c_str(), token, length) == 0;
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
    if (length <= size()) {
        return strncasecmp(c_str(), token, length) == 0;
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool PString::ends_with(const char* token, size_t length) const
{
    if (length == INVALID_INDEX) length = strlen(token);
    if (length <= size()) {
        return strncmp(c_str() + size() - length, token, length) == 0;
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
    if (length <= size()) {
        return strncasecmp(c_str() + size() - length, token, length) == 0;
    } else {
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PString::count_chars() const
{
    size_t numChars = 0;
    for (size_t i = 0; i < size(); i += utf8_char_length((*this)[i])) {
        numChars++;
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::lower()
{
    for (size_t i = 0; i < size(); ++i) {
        (*this)[i] = char(tolower((*this)[i]));
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PString& PString::upper()
{
    for (size_t i = 0; i < size(); ++i) {
        (*this)[i] = char(toupper((*this)[i]));
    }
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

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
