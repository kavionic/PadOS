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
// Created: 28.03.2018 20:51:21

#pragma once

#include <string>
#include <vector>
#include <stdarg.h>
#include "System/Types.h"

namespace os
{

class String : public std::string
{
public:
    template<typename... ARGS>
    String(ARGS&&... args) : std::string(args...) {}
    String(const wchar16_t* utf16, size_t length = npos);
    
    String& assign_utf16(const wchar16_t* source, size_t length = npos);

    size_t copy_utf16(wchar16_t* dst, size_t length, size_t pos = 0) const;

    String& append(uint32_t unicode);

    int compare_nocase(const std::string& rhs) const;
    int compare_nocase(const char* rhs) const;

    size_t  count_chars() const;
    String& strip();
    String& lstrip();
    String& rstrip();
    String& lower();
    String& upper();
        
    String& format(const char* fmt, va_list pArgs)
    {
        int maxLen = 128;
        int length;
        {
            char buffer[maxLen];
            length = vsnprintf( buffer, maxLen, fmt, pArgs );

            if ( length < maxLen ) {
                assign(buffer);
                return *this;
            }
        }
        while ( length >= maxLen )
        {
            maxLen *= 2;
            std::vector<char> buffer;
            buffer.resize(maxLen);
            
            length = vsnprintf( buffer.data(), maxLen, fmt, pArgs );
            if ( length < maxLen ) {
                assign(buffer.data());
            }
        }
        return *this;
    }

    String& format(const char* fmt, ...)
    {
        va_list argList;
        va_start( argList, fmt );
        format( fmt, argList );
        va_end( argList );
        return *this;
    }

    static String format_string(const char* fmt, ...)
    {
        String result;
        va_list argList;
        va_start(argList, fmt);
        result.format(fmt, argList);
        va_end(argList);
        return result;
    }

	// FNV-1a 32bit hashing algorithm.
	static constexpr uint32_t hash_string_literal(char const* s, size_t count)
	{
		return ((count ? hash_string_literal(s, count - 1) : 2166136261u) ^ s[count]) * 16777619u;
	}

    static String zero;        
};


}
