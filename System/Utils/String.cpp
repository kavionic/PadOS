// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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

#include "Utils/String.h"
#include "Utils/UTF8Utils.h"

using namespace os;

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

size_t String::count_chars() const
{
    size_t numChars = 0;
    for (size_t i = 0 ; i < size() ; i += utf8_char_length((*this)[i])) {
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
    
    for ( i = begin() ; i != end() ; ++i)
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
    for (ssize_t i = size() - 1 ; i >= 0 ; --i)
    {
        if (!isspace((*this)[i])) {
            break;
        }
        spaces++;
    }
    resize(size() - spaces );
    return *this;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

String& String::lower()
{
    for (size_t i = 0 ; i < size() ; ++i) {
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
