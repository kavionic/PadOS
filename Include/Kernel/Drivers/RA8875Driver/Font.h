// This file is part of PadOS.
//
// Copyright (C) 2014-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 26.01.2014 15:05:06

// ASCII / ISO 8859-1 (Latin-1) characters:
// " !"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~¡¢£¤¥¦§¨©ª«¬®¯°±²³´µ¶·¸¹º»¼½¾¿ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏĞÑÒÓÔÕÖ×ØÙÚÛÜİŞßàáâãäåæçèéêëìíîïğñòóôõö÷øùúûüışÿ"

#pragma once

#include <stdint.h>

// This structure describes a single character's display information
typedef struct
{
    const uint8_t widthBits;					// width, in bits (or pixels), of the character
    const uint16_t offset;					// offset of the character's bitmap, in bytes, into the the FONT_INFO's data array
    
} FONT_CHAR_INFO;

// Describes a single font
typedef struct
{
    const uint8_t 			heightPages;	// height, in pages (8 pixels), of the font's characters
    const uint8_t 			startChar;		// the first character in the font (e.g. in charInfo and data)
    const uint8_t 			endChar;		// the last character in the font
    //	const uint8_t			spacePixels;	// number of pixels that a space character takes up
    const FONT_CHAR_INFO*	charInfo;		// pointer to array of char information
    const uint8_t*			data;			// pointer to generated array of character visual representation
    
} FONT_INFO;
