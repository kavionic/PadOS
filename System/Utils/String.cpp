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

namespace os
{

String String::zero;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String::String(const wchar16_t* utf16, size_t length)
{
    assign_utf16(utf16, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String& String::assign_utf16(const wchar16_t* source, size_t length)
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

size_t String::copy_utf16(wchar16_t* dst, size_t length, size_t pos) const
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

String& String::append(uint32_t unicode)
{
    char utf8Buf[8];
    size_t utf8Length = unicode_to_utf8(utf8Buf, unicode);
    insert(end(), utf8Buf, utf8Buf + utf8Length);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int String::compare_nocase(const std::string& rhs) const
{
    return strcasecmp(c_str(), rhs.c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int String::compare_nocase(const char* rhs) const
{
    return strcasecmp(c_str(), rhs);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool String::starts_with(const char* token, size_t length /*= INVALID_INDEX*/) const
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

bool String::starts_with_nocase(const char* token, size_t length /*= INVALID_INDEX*/) const
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

bool String::ends_with(const char* token, size_t length) const
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

bool String::ends_with_nocase(const char* token, size_t length) const
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

size_t String::count_chars() const
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

String& String::strip()
{
    rstrip();
    lstrip();
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String& String::lstrip()
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

String& String::rstrip()
{
    size_t spaces = 0;
    for (ssize_t i = size() - 1; i >= 0; --i)
    {
        if (!isspace((*this)[i])) {
            break;
        }
        spaces++;
    }
    resize(size() - spaces);
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String& String::lower()
{
    for (size_t i = 0; i < size(); ++i) {
        (*this)[i] = char(tolower((*this)[i]));
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String& String::upper()
{
    for (size_t i = 0; i < size(); ++i) {
        (*this)[i] = char(toupper((*this)[i]));
    }
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String String::format_file_size(off64_t size)
{
    if (size < 1024LL) {
        return format_string("%u", uint32_t(size));
    } else if (size < 1024LL * 1024LL) {
        return format_string("%uKB", uint32_t(size / 1024LL));
    } else if (size < 1024LL * 1024LL * 1024LL) {
        return format_string("%uMB", uint32_t(size / (1024LL * 1024LL)));
    } else if (size < 1024LL * 1024LL * 1024LL * 1024LL) {
        return format_string("%uGB", uint32_t(size / (1024LL * 1024LL * 1024LL)));
    } else if (size < 1024LL * 1024LL * 1024LL * 1024LL * 1024LL) {
        return format_string("%uTB", uint32_t(size / (1024LL * 1024LL * 1024LL * 1024LL)));
    } else {
        return format_string("%uPB", uint32_t(size / (1024LL * 1024LL * 1024LL * 1024LL * 1024LL)));
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String String::format_time_period(const TimeValMicros& timeVal, bool includeUnits, size_t maxCharacters)
{
    if (maxCharacters == 0) maxCharacters = std::numeric_limits<size_t>::max();

    bigtime_t seconds = timeVal.AsSecondsI();

    char secondStr[6] = { 0 };
    char minuteStr[4] = { 0 };
    char hourStr[4]   = { 0 };
    char dayStr[5]    = { 0 };
    char yearStr[16]  = { 0 }; // Max years in a TimeValMicros: 584942

    if (seconds < 60)
    {
        sprintf(secondStr, "%.1f", timeVal.AsSecondsF());
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
    String result;
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

} //namespace os
