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

namespace os
{

class String : public std::string
{
public:
    template<typename... ARGS>
    String(ARGS&&... args) : std::string(args...) {}
        
        
    String& Format(const char* format, va_list pArgs)
    {
        int maxLen = 128;
        int length;
        {
            char buffer[maxLen];
            length = vsnprintf( buffer, maxLen, format, pArgs );

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
            
            length = vsnprintf( buffer.data(), maxLen, format, pArgs );
            if ( length < maxLen ) {
                assign(buffer.data());
            }
        }
        return *this;
    }

    String& Format(const char* format, ...)
    {
        va_list argList;
        va_start( argList, format );
        Format( format, argList );
        va_end( argList );
        return *this;
    }

    static String FormatString(const char* format, ...)
    {
        String result;
        va_list argList;
        va_start(argList, format);
        result.Format(format, argList);
        va_end(argList);
        return result;
    }
        
};

}
