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
// Created: 18/05/18 11:36:18

#pragma once

inline bool is_first_utf8_byte(uint8_t byte)
{
    return( (byte & 0x80) == 0 || (byte & 0xc0) == 0xc0 );
    
}

inline int utf8_char_length(uint8_t firstByte)
{
    return ((0xe5000000 >> ((firstByte >> 3) & 0x1e)) & 3) + 1;
}

inline int utf8_to_unicode(const char* source)
{
    if ( (source[0] & 0x80) == 0 ) {
        return *source;
    } else if ((source[1] & 0xc0) != 0x80) {
        return 0xfffd;
    } else if ((source[0]&0x20) == 0) {
        return ((source[0] & 0x1f) << 6) | (source[1] & 0x3f);
    } else if ( (source[2] & 0xc0) != 0x80 ) {
        return 0xfffd;
    } else if ( (source[0] & 0x10) == 0 ) {
        return ((source[0] & 0x0f)<<12) | ((source[1] & 0x3f)<<6) | (source[2] & 0x3f);
    } else if ((source[3] & 0xC0) != 0x80) {
        return 0xfffd;
    } else {
        int   nValue;
        nValue = ((source[0] & 0x07)<<18) | ((source[1] & 0x3f)<<12) | ((source[2] & 0x3f)<<6) | (source[3] & 0x3f);
        return ((0xd7c0+(nValue>>10)) << 16) | (0xdc00+(nValue & 0x3ff));
    }
    
}

inline int unicode_to_utf8(char* dest, uint32_t character)
{
    if ((character & 0xff80) == 0) {
        *dest = character;
        return 1;
    } else if ((character & 0xf800) == 0) {
        dest[0] = 0xc0 | (character >> 6);
        dest[1] = 0x80 | ((character) & 0x3f);
        return 2;
    } else if ((character & 0xfc00) != 0xd800) {
        dest[0] = 0xe0 | (character >> 12);
        dest[1] = 0x80 | ((character >> 6) & 0x3f);
        dest[2] = 0x80 | ((character) & 0x3f);
        return 3;
    } else {
        int   nValue;
        nValue = ( ((character << 16) - 0xd7c0) << 10 ) | (character & 0x3ff);
        dest[0] = 0xf0 | (nValue >> 18);
        dest[1] = 0x80 | ((nValue >> 12) & 0x3f);
        dest[2] = 0x80 | ((nValue >> 6) & 0x3f);
        dest[3] = 0x80 | (nValue & 0x3f);
        return 4;
    }
}
