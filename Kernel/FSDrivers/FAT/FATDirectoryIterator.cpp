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
// Created: 18/06/01 1:03:46

#include "System/Platform.h"

#include <string.h>
#include <algorithm>

#include <System/ExceptionHandling.h>
#include <Utils/UTF8Utils.h>
#include <Utils/Utils.h>
#include <Kernel/KLogging.h>
#include <Kernel/FSDrivers/FAT/FATFilesystem.h>

#include "FATDirectoryIterator.h"
#include "FATVolume.h"


namespace kernel
{
    
const char g_ValidShortNameCharacters[]="!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";

static const uint16_t g_CP437ToUTF162[] = {
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7, 0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9, 0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA, 0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556, 0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F, 0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B, 0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4, 0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248, 0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0,
};

struct UTF16ToCP437Node
{
    uint16_t m_UTF16;
    uint8_t  m_CP437;
    
    bool operator<(uint16_t rhs) const { return m_UTF16 < rhs; }
    bool operator<(const UTF16ToCP437Node& rhs) const { return m_UTF16 < rhs.m_UTF16; }
};

static const UTF16ToCP437Node g_UTF16ToCP437[] = 
{
    {0x00A0, 0xff}, {0x00A1, 0xad}, {0x00A2, 0x9b}, {0x00A3, 0x9c}, {0x00A5, 0x9d}, {0x00AA, 0xa6}, {0x00AB, 0xae}, {0x00AC, 0xaa}, {0x00B0, 0xf8}, {0x00B1, 0xf1}, {0x00B2, 0xfd}, {0x00B5, 0xe6}, {0x00B7, 0xfa}, {0x00BA, 0xa7}, {0x00BB, 0xaf}, {0x00BC, 0xac},
    {0x00BD, 0xab}, {0x00BF, 0xa8}, {0x00C4, 0x8e}, {0x00C5, 0x8f}, {0x00C6, 0x92}, {0x00C7, 0x80}, {0x00C9, 0x90}, {0x00D1, 0xa5}, {0x00D6, 0x99}, {0x00DC, 0x9a}, {0x00DF, 0xe1}, {0x00E0, 0x85}, {0x00E1, 0xa0}, {0x00E2, 0x83}, {0x00E4, 0x84}, {0x00E5, 0x86},
    {0x00E6, 0x91}, {0x00E7, 0x87}, {0x00E8, 0x8a}, {0x00E9, 0x82}, {0x00EA, 0x88}, {0x00EB, 0x89}, {0x00EC, 0x8d}, {0x00ED, 0xa1}, {0x00EE, 0x8c}, {0x00EF, 0x8b}, {0x00F1, 0xa4}, {0x00F2, 0x95}, {0x00F3, 0xa2}, {0x00F4, 0x93}, {0x00F6, 0x94}, {0x00F7, 0xf6},
    {0x00F9, 0x97}, {0x00FA, 0xa3}, {0x00FB, 0x96}, {0x00FC, 0x81}, {0x00FF, 0x98}, {0x0192, 0x9f}, {0x0393, 0xe2}, {0x0398, 0xe9}, {0x03A3, 0xe4}, {0x03A6, 0xe8}, {0x03A9, 0xea}, {0x03B1, 0xe0}, {0x03B4, 0xeb}, {0x03B5, 0xee}, {0x03C0, 0xe3}, {0x03C3, 0xe5},
    {0x03C4, 0xe7}, {0x03C6, 0xed}, {0x207F, 0xfc}, {0x20A7, 0x9e}, {0x2219, 0xf9}, {0x221A, 0xfb}, {0x221E, 0xec}, {0x2229, 0xef}, {0x2248, 0xf7}, {0x2261, 0xf0}, {0x2264, 0xf3}, {0x2265, 0xf2}, {0x2310, 0xa9}, {0x2320, 0xf4}, {0x2321, 0xf5}, {0x2500, 0xc4},
    {0x2502, 0xb3}, {0x250C, 0xda}, {0x2510, 0xbf}, {0x2514, 0xc0}, {0x2518, 0xd9}, {0x251C, 0xc3}, {0x2524, 0xb4}, {0x252C, 0xc2}, {0x2534, 0xc1}, {0x253C, 0xc5}, {0x2550, 0xcd}, {0x2551, 0xba}, {0x2552, 0xd5}, {0x2553, 0xd6}, {0x2554, 0xc9}, {0x2555, 0xb8},
    {0x2556, 0xb7}, {0x2557, 0xbb}, {0x2558, 0xd4}, {0x2559, 0xd3}, {0x255A, 0xc8}, {0x255B, 0xbe}, {0x255C, 0xbd}, {0x255D, 0xbc}, {0x255E, 0xc6}, {0x255F, 0xc7}, {0x2560, 0xcc}, {0x2561, 0xb5}, {0x2562, 0xb6}, {0x2563, 0xb9}, {0x2564, 0xd1}, {0x2565, 0xd2},
    {0x2566, 0xcb}, {0x2567, 0xcf}, {0x2568, 0xd0}, {0x2569, 0xca}, {0x256A, 0xd8}, {0x256B, 0xd7}, {0x256C, 0xce}, {0x2580, 0xdf}, {0x2584, 0xdc}, {0x2588, 0xdb}, {0x258C, 0xdd}, {0x2590, 0xde}, {0x2591, 0xb0}, {0x2592, 0xb1}, {0x2593, 0xb2}, {0x25A0, 0xfe}
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint16_t CP437ToUTF16(uint8_t cp437)
{
    if (cp437 < 0x80) {
        return cp437;
    } else {
        return g_CP437ToUTF162[cp437 - 0x80];
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool UTF16ToCP437(uint16_t unicode, uint8_t* result)
{
    if (unicode < 0x80)
    {
        *result = uint8_t(unicode);
        return true;
    }
    else
    {
        auto i = std::lower_bound(std::begin(g_UTF16ToCP437), std::end(g_UTF16ToCP437), unicode);
        if (i != std::end(g_UTF16ToCP437) && i->m_UTF16 == unicode) {
            *result = i->m_CP437;
            return true;
        } else {
            return false;
        }
    }        
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static uint8_t ConvertCP437CharacterCase(uint8_t character, bool toLowercase)
{
    static const char lowercaseCharacters[] = "\x81\x82\x84\x86\x87\x91\x94\xA4\xE5\xED";
    static const char uppercaseCharacters[] = "\x9A\x90\x8E\x8F\x80\x92\x99\xA5\xE4\xE8";

    if (toLowercase && character >= 'A' && character <= 'Z') {
        return uint8_t(character - 'A' + 'a');
    }
    if (!toLowercase && character >= 'a' && character <= 'z') {
        return uint8_t(character - 'a' + 'A');
    }

    const char* sourceCharacters = toLowercase ? uppercaseCharacters : lowercaseCharacters;
    const char* destinationCharacters = toLowercase ? lowercaseCharacters : uppercaseCharacters;
    const char* sourceCharacter = strchr(sourceCharacters, character);
    if (sourceCharacter != nullptr)
    {
        const size_t mappingIndex = size_t(sourceCharacter - sourceCharacters);
        return uint8_t(destinationCharacters[mappingIndex]);
    }
    return character;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool IsCharacterValid(uint16_t character)
{
	static const char illegal[]   = "\\/:*?\"<>|";
	if (character < 0x20 || character == 0xfffe || character == 0xffff) {
		return false;
	}
	return character >= 0x80 || strchr(illegal, char(character)) == nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool CopyLFNNamePart(
    const FATDirectoryEntryLFN& entry,
    wchar16_t* destination,
    bool isLastNamePart,
    size_t& outCharacterCount,
    bool& needsHighSurrogate)
{
    wchar16_t nameCharacters[FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY];

    memcpy(nameCharacters, entry.m_NamePart1, sizeof(entry.m_NamePart1));
    memcpy(nameCharacters + ARRAY_COUNT(entry.m_NamePart1), entry.m_NamePart2, sizeof(entry.m_NamePart2));
    memcpy(nameCharacters + ARRAY_COUNT(entry.m_NamePart1) + ARRAY_COUNT(entry.m_NamePart2), entry.m_NamePart3, sizeof(entry.m_NamePart3));

    size_t characterCount = ARRAY_COUNT(nameCharacters);
    if (isLastNamePart)
    {
        bool terminatorFound = false;
        for (size_t characterIndex = 0; characterIndex < ARRAY_COUNT(nameCharacters); ++characterIndex)
        {
            if (!terminatorFound)
            {
                if (nameCharacters[characterIndex] == 0)
                {
                    characterCount = characterIndex;
                    terminatorFound = true;
                }
                else if (nameCharacters[characterIndex] == 0xffff)
                {
                    return false;
                }
            }
            else if (nameCharacters[characterIndex] != 0xffff)
            {
                return false;
            }
        }

        if (characterCount != 0 &&
            (nameCharacters[characterCount - 1] == ' ' ||
             nameCharacters[characterCount - 1] == '.'))
        {
            return false;
        }
    }

    // LFN entries are encountered from the end of the name toward its
    // beginning. Validate UTF-16 in that same direction so surrogate pairs
    // spanning two entries do not require another pass over the full name.
    for (size_t characterIndex = characterCount; characterIndex > 0; --characterIndex)
    {
        const uint16_t character = nameCharacters[characterIndex - 1];
        if (character == 0 || character == 0xffff) {
            return false;
        }

        if (is_utf16_low_surrogate(character))
        {
            if (needsHighSurrogate) {
                return false;
            }
            needsHighSurrogate = true;
        }
        else if (is_utf16_high_surrogate(character))
        {
            if (!needsHighSurrogate) {
                return false;
            }
            needsHighSurrogate = false;
        }
        else if (needsHighSurrogate || !IsCharacterValid(character))
        {
            return false;
        }

        destination[characterIndex - 1] = character;
    }

    outCharacterCount = characterCount;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool FilteredUTF16ToCP437(uint16_t utf16, uint8_t* result)
{
	static const char underbar[]  = "+,;=[]"
									"\x83\x85\x88\x89\x8A\x8B\x8C\x8D"
									"\x93\x95\x96\x97\x98"
									"\xA0\xA1\xA2\xA3";

	uint8_t character;
    if (UTF16ToCP437(utf16, &character))
    {
        const uint8_t uppercaseCharacter = ConvertCP437CharacterCase(character, false);
        if (uppercaseCharacter != character) {
            *result = uppercaseCharacter;
            return true;
        } else if (strchr(underbar, character)) {
            *result = '_';
            return true;
        } else if (strchr(g_ValidShortNameCharacters, character) != nullptr || ConvertCP437CharacterCase(character, true) != character) {
            *result = character;
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static bool IsShortNameCharacterCompatible(uint8_t character, bool& hasUppercase, bool& hasLowercase)
{
    const uint8_t uppercaseCharacter = ConvertCP437CharacterCase(character, false);
    const bool isCasedCharacter = ConvertCP437CharacterCase(uppercaseCharacter, true) != uppercaseCharacter;

    if (strchr(g_ValidShortNameCharacters, uppercaseCharacter) == nullptr && !isCasedCharacter) {
        return false;
    }

    if (uppercaseCharacter != character)
    {
        if (hasUppercase) {
            return false;
        }
        hasLowercase = true;
    }
    else if (isCasedCharacter)
    {
        if (hasLowercase) {
            return false;
        }
        hasUppercase = true;
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void FATRawShortNameToUTF8(const FATDirectoryEntry& entry, PString& destination)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATRawShortNameToUTF8().");

    destination.clear();
    const bool lowercaseBase = (entry.m_ShortNameCaseFlags & FAT_SHORT_NAME_LOWERCASE_BASE) != 0;
    const bool lowercaseExtension = (entry.m_ShortNameCaseFlags & FAT_SHORT_NAME_LOWERCASE_EXTENSION) != 0;

    size_t baseLength = 8;
    while (baseLength > 0 && entry.m_Filename[baseLength - 1] == ' ') {
        --baseLength;
    }

    for (size_t characterIndex = 0; characterIndex < baseLength; ++characterIndex)
    {
        uint8_t character = (characterIndex == 0 && entry.m_Filename[characterIndex] == 5) ? 0xe5 : uint8_t(entry.m_Filename[characterIndex]);
        if (lowercaseBase) {
            character = ConvertCP437CharacterCase(character, true);
        }
        destination.append_utf32_char(CP437ToUTF16(character));
    }

    size_t extensionLength = 3;
    while (extensionLength > 0 && entry.m_Filename[8 + extensionLength - 1] == ' ') {
        --extensionLength;
    }

    if (extensionLength != 0)
    {
        destination += ".";
        for (size_t characterIndex = 8; characterIndex < 8 + extensionLength; ++characterIndex)
        {
            uint8_t character = uint8_t(entry.m_Filename[characterIndex]);
            if (lowercaseExtension) {
                character = ConvertCP437CharacterCase(character, true);
            }
            destination.append_utf32_char(CP437ToUTF16(character));
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATDirectoryIterator::RequiresLongName(const wchar16_t* longName, size_t longNameLength, uint8_t& outShortNameCaseFlags)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::RequiresLongName().");

    outShortNameCaseFlags = 0;

    bool baseHasUppercase = false;
    bool baseHasLowercase = false;
    size_t nameIndex = 0;

    for (; nameIndex < longNameLength && nameIndex < 8; ++nameIndex)
    {
        if (longName[nameIndex] == '.') {
            break;
        }

        uint8_t character;
        if (!UTF16ToCP437(longName[nameIndex], &character) || !IsShortNameCharacterCompatible(character, baseHasUppercase, baseHasLowercase)) {
            return true;
        }
    }

    if (nameIndex == longNameLength)
    {
        if (baseHasLowercase) {
            outShortNameCaseFlags |= FAT_SHORT_NAME_LOWERCASE_BASE;
        }
        return false;
    }

    if (nameIndex == 0) {
        return true; // Names beginning with a period require an LFN entry.
    }
    if (nameIndex == 8 && longName[nameIndex] != '.') {
        return true; // Name too long.
    }

    ++nameIndex;
    if (nameIndex == longNameLength) {
        return true; // Filenames with trailing periods.
    }

    bool extensionHasUppercase = false;
    bool extensionHasLowercase = false;
    size_t extensionLength = 0;
    for (; extensionLength < 3 && nameIndex < longNameLength; ++extensionLength, ++nameIndex)
    {
        uint8_t character;
        if (!UTF16ToCP437(longName[nameIndex], &character) || !IsShortNameCharacterCompatible(character, extensionHasUppercase, extensionHasLowercase)) {
            return true;
        }
    }

    if (nameIndex != longNameLength) {
        return true;
    }
    if (baseHasLowercase) {
        outShortNameCaseFlags |= FAT_SHORT_NAME_LOWERCASE_BASE;
    }
    if (extensionHasLowercase) {
        outShortNameCaseFlags |= FAT_SHORT_NAME_LOWERCASE_EXTENSION;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static size_t GetShortNameNumericTailStart(const char shortName[11], size_t numericTailLength)
{
    kassert(numericTailLength >= 2 && numericTailLength <= 7);

    size_t baseLength = 8;
    while (baseLength > 0 && shortName[baseLength - 1] == ' ') {
        --baseLength;
    }

    kassert(baseLength != 0);
    return std::min(baseLength, size_t(8) - numericTailLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATDirectoryIterator::GetGeneratedShortNameNumericTailValue(
    const char shortName[11],
    const char baseShortName[11],
    uint32_t& outNumericTailValue)
{
    size_t numericTailMarkerIndex = 8;
    size_t characterIndex = 0;
    for (; characterIndex < 8; ++characterIndex)
    {
        const char shortNameCharacter = shortName[characterIndex];
        if (shortNameCharacter != baseShortName[characterIndex])
        {
            if (shortNameCharacter == '~') {
                numericTailMarkerIndex = characterIndex;
            }
            break;
        }

        // The generated marker can overlap a '~' already present in the base.
        if (shortNameCharacter == '~') {
            numericTailMarkerIndex = characterIndex;
        }
    }

    if (characterIndex == 8 ||
        numericTailMarkerIndex == 0 ||
        numericTailMarkerIndex == 8)
    {
        return false;
    }

    characterIndex = numericTailMarkerIndex + 1;

    uint32_t numericTailValue = 0;
    while (characterIndex < 8 && shortName[characterIndex] >= '0' && shortName[characterIndex] <= '9')
    {
        numericTailValue = numericTailValue * 10 + uint32_t(shortName[characterIndex] - '0');
        ++characterIndex;
    }
    if (numericTailValue == 0) {
        return false;
    }

    while (characterIndex < 8 && shortName[characterIndex] == ' ') {
        ++characterIndex;
    }

    if (characterIndex != 8) {
        return false;
    }
    if (memcmp(shortName + 8, baseShortName + 8, 3) != 0) {
        return false;
    }

    outNumericTailValue = numericTailValue;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATDirectoryIterator::MungeShortName(char* shortName, uint32_t numericTailValue)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::MungeShortName().");

    kassert(numericTailValue > 0 && numericTailValue <= FAT_SHORT_NAME_MAX_NUMERIC_TAIL_VALUE);

    char numericTail[7];
    size_t numericTailBufferIndex = sizeof(numericTail);
    uint32_t remainingValue = numericTailValue;
    do
    {
        numericTail[--numericTailBufferIndex] = char('0' + remainingValue % 10);
        remainingValue /= 10;
    } while (remainingValue != 0);
    numericTail[--numericTailBufferIndex] = '~';

    const size_t numericTailLength = sizeof(numericTail) - numericTailBufferIndex;
    const size_t numericTailStart = GetShortNameNumericTailStart(shortName, numericTailLength);
    memcpy(shortName + numericTailStart, numericTail + numericTailBufferIndex, numericTailLength);
}

///////////////////////////////////////////////////////////////////////////////
/// Generate an 8.3 compatible name from a long name.
///
///  *  Leading '.'s are ignored. If there are multiple '.'s in the name,
///     the last one signals the extension.
///  *  Characters in short-name are up-cased.
///
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATDirectoryIterator::GenerateShortName(const wchar16_t* longName, size_t longNameLength, char* shortName)
{
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::GenerateShortName().");

    memset(shortName, ' ', 11);

    ssize_t srcPos = 0;
    size_t  dstPos = 0;
    for (; srcPos < longNameLength && dstPos < 8; ++srcPos)
    {
        if (longName[srcPos] == 0) {
            if (dstPos == 0) shortName[0] = '_';
            return;
        }            
        if (longName[srcPos] == '.')
        {
            if (dstPos == 0) {
                continue; // Skip leading dots.
            } else {
                break;    // Extension found.
            }
        }
		if (!IsCharacterValid(longName[srcPos])) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
		}
        uint8_t c;
        if (FilteredUTF16ToCP437(longName[srcPos], &c))
        {
            shortName[dstPos++] = c;
        }
    }
    if (dstPos == 0) shortName[dstPos++] = '_';

    // Find the final dot.
    for (srcPos = longNameLength - 1; srcPos >= 0; --srcPos)
    {
        if (longName[srcPos] == '.') {
            break;
        }                    
    }
    if (srcPos < 0) return;

    srcPos++;

    for (size_t dstPos = 8; dstPos < 11 && srcPos < longNameLength; ++srcPos)
    {
        if (longName[srcPos] == 0) return;

		if (!IsCharacterValid(longName[srcPos])) {
            PERROR_THROW_CODE(PErrorCode::INVAL);
		}
        uint8_t c;
        if (FilteredUTF16ToCP437(longName[srcPos], &c))
        {
			shortName[dstPos++] = c;
        }        
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void FATDirectoryIterator::ReleaseCurrentBlock()
{
    if (m_CurrentBlock.m_Buffer != nullptr)
    {
        if (m_IsDirty)
        {
            m_SectorIterator.MarkBlockDirty();
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCat_FATDIR, "FATDirectoryIterator::ReleaseCurrentBlock(): Writing updated directory entries.");
            m_IsDirty = false;
        }
        m_CurrentBlock.Reset();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATDirectoryIterator::FATDirectoryIterator(Ptr<FATVolume> vol, uint32_t cluster, uint32_t index) : m_SectorIterator(vol, cluster, 0)
{
    m_IsDirty = false;
    
    m_EntriesPerSector = vol->m_BytesPerSector / sizeof(FATDirectoryEntry);

    if (cluster >= vol->m_TotalClusters + 2) {
        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::FATDirectoryIterator() cluster {} outside volume.", cluster);
    }
    m_StartingCluster = cluster;
    m_CurrentIndex    = index;
    if (index >= m_EntriesPerSector)
    {
        m_SectorIterator.Increment(m_CurrentIndex / m_EntriesPerSector);
    }
    m_CurrentBlock = m_SectorIterator.GetBlock_(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATDirectoryIterator::~FATDirectoryIterator()
{
    ReleaseCurrentBlock();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATDirectoryEntryCombo* FATDirectoryIterator::Set(uint32_t cluster, uint32_t index)
{
    m_CurrentBlock.Reset();;

    if (cluster >= m_SectorIterator.m_Volume->m_TotalClusters + 2) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    ;
    m_SectorIterator.Set(cluster, 0);

    m_IsDirty = false;
    m_StartingCluster = cluster;
    m_CurrentIndex    = index;
    if (index >= m_EntriesPerSector)
    {
        m_SectorIterator.Increment(m_CurrentIndex / m_EntriesPerSector);
    }

    m_CurrentBlock = m_SectorIterator.GetBlock_(true);

    if (m_CurrentBlock.m_Buffer == nullptr) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }
    return static_cast<FATDirectoryEntryCombo*>(m_CurrentBlock.m_Buffer) + (m_CurrentIndex % m_EntriesPerSector);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATDirectoryEntryCombo* FATDirectoryIterator::GetCurrentEntry()
{
    if (m_CurrentBlock.m_Buffer == nullptr) {
        return nullptr;
    }
    return static_cast<FATDirectoryEntryCombo*>(m_CurrentBlock.m_Buffer) + (m_CurrentIndex % m_EntriesPerSector);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATDirectoryEntryCombo* FATDirectoryIterator::GetNextRawEntry()
{
    if (m_CurrentBlock.m_Buffer == nullptr) {
        return nullptr;
    }
    if ((++m_CurrentIndex % m_EntriesPerSector) == 0)
    {
        ReleaseCurrentBlock();
        if (!m_SectorIterator.Increment(1)) {
            return nullptr;
        }
        m_CurrentBlock = m_SectorIterator.GetBlock_(true);
        if (m_CurrentBlock.m_Buffer == nullptr) {
            return nullptr;
        }            
    }
    return static_cast<FATDirectoryEntryCombo*>(m_CurrentBlock.m_Buffer) + (m_CurrentIndex % m_EntriesPerSector);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATDirectoryIterator::GetNextLFNEntry(FATDirectoryEntryInfo* outInfo, PString* filename, PString* outShortFilename)
{
    uint8_t            hash = 0;
    std::vector<wchar16_t> utf16Buffer;

    if (filename != nullptr) {
        utf16Buffer.resize(FAT_LONG_NAME_MAX_ENTRY_COUNT * FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY);
    }
    kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): {}", m_CurrentIndex);

    // LFN state
    bool hasLongName = false;
    bool needsHighSurrogate = false;
    uint32_t startIndex = 0;
    size_t filenameLen = 0;
    uint32_t lfnCount = 0;

    FATDirectoryEntryCombo* buffer;
    for (buffer = GetCurrentEntry(); buffer != nullptr; buffer = GetNextRawEntry())
    {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): {:x}/{:x}/{}", m_SectorIterator.m_CurrentCluster, m_SectorIterator.m_CurrentSector, m_CurrentIndex);
        if (buffer->m_LFN.m_SequenceNumber == 0) // quit if at end of table
        {
            if (hasLongName) {
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): LFN entry ({}) with no alias.", (filename != nullptr) ? filename->c_str() : "*none*");
            }
            return false;
        }
        
        if (buffer->m_LFN.m_SequenceNumber == 0xe5) // skip erased entries
        {
            if (hasLongName) {
                kernel_log<PLogSeverity::WARNING>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): LFN entry ({}) with intervening erased entries.", (filename != nullptr) ? filename->c_str() : "*none*");
                hasLongName = false;
            }
            kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): Entry erased, skipping...");
            continue;
        }
        
        if ((buffer->m_LFN.m_Attribs & FAT_LONG_NAME_ATTRIBUTE_MASK) == FAT_LONG_NAME_ATTRIBUTES)
        {
            if ((buffer->m_LFN.m_Reserved1 != 0) || (buffer->m_LFN.m_Reserved2[0] != 0) || (buffer->m_LFN.m_Reserved2[1] != 0))
            {
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): Invalid LFN entry: reserved fields clobbered.");
                hasLongName = false;
                continue;
            }
            if (!hasLongName)
            {
                const uint8_t sequenceNumber = buffer->m_LFN.m_SequenceNumber;
                const uint32_t entryCount = sequenceNumber & 0x1f;
                if ((sequenceNumber & 0xe0) != 0x40 || entryCount == 0 || entryCount > FAT_LONG_NAME_MAX_ENTRY_COUNT)
                {
                    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): invalid LFN start sequence number 0x{:02x}.", sequenceNumber);
                    continue;
                }

                hash = buffer->m_LFN.m_Hash;
                lfnCount = entryCount;

                if (filename != nullptr)
                {
                    wchar16_t* destination = utf16Buffer.data() + FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY * (lfnCount - 1);
                    size_t namePartLength = 0;
                    needsHighSurrogate = false;
                    if (!CopyLFNNamePart(buffer->m_LFN, destination, true, namePartLength, needsHighSurrogate) ||
                        (namePartLength == 0 && lfnCount == 1))
                    {
                        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): invalid final LFN name payload.");
                        continue;
                    }

                    filenameLen = size_t(destination - utf16Buffer.data()) + namePartLength;
                    if (filenameLen > FAT_LONG_NAME_MAX_LENGTH)
                    {
                        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): long file name exceeds {} characters.", FAT_LONG_NAME_MAX_LENGTH);
                        continue;
                    }
                }

                startIndex = m_CurrentIndex;
                hasLongName = true;
                continue;
            }
            else
            {
                if (buffer->m_LFN.m_Hash != hash)
                {
                    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): Error in long file name: hash values don't match.");
                    hasLongName = false;
                    continue;
                }
                if (lfnCount <= 1 || buffer->m_LFN.m_SequenceNumber != lfnCount - 1)
                {
                    kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): Bad LFN sequence number.");
                    hasLongName = false;
                    continue;
                }
                --lfnCount;
                if (filename != nullptr)
                {
                    wchar16_t* destination = utf16Buffer.data() + FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY * (lfnCount - 1);
                    size_t namePartLength = 0;
                    if (!CopyLFNNamePart(buffer->m_LFN, destination, false, namePartLength, needsHighSurrogate))
                    {
                        kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): invalid LFN name payload.");
                        hasLongName = false;
                        continue;
                    }
                    kassert(namePartLength == FAT_LONG_NAME_CHARACTERS_PER_LFN_ENTRY);
                }
                continue;
            }
        }
        break;
    }

    // Hit end of directory with no luck
    if (buffer == nullptr) {
        return false;
    }        

    // Process long name
    if (hasLongName)
    {
        if (lfnCount != 1)
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): Unfinished LFN in directory");
            hasLongName = false;
        }
        else if (filename != nullptr && needsHighSurrogate)
        {
            kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): LFN begins with an unmatched low surrogate.");
            hasLongName = false;
        }
        else
        {
            if (filename != nullptr) {
                filename->assign_utf16(utf16Buffer.data(), filenameLen);
            }            
            if (HashMSDOSName(buffer->m_Normal.m_Filename) != hash)
            {
                kernel_log<PLogSeverity::CRITICAL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextLFNEntry(): long file name ({}) hash and short file name ({:11.11}) don't match", ((filename != nullptr) ? filename->c_str() : "*none*"), buffer->m_Normal.m_Filename);
                hasLongName = false;
            }
        }
    }

    // Process short name
    if (!hasLongName)
    {
        startIndex = m_CurrentIndex;
        if (filename != nullptr) {
            FATRawShortNameToUTF8(buffer->m_Normal, *filename);
        }            
    }

    if (outShortFilename != nullptr) {
        FATRawShortNameToUTF8(buffer->m_Normal, *outShortFilename);
    }

    if (outInfo != nullptr)
    {
        outInfo->m_StartIndex = startIndex;
        outInfo->m_EndIndex   = m_CurrentIndex;
        outInfo->m_DOSAttribs = buffer->m_Normal.m_Attribs;
        outInfo->m_StartCluster = buffer->m_Normal.m_FirstClusterLow;
        if (m_SectorIterator.m_Volume->m_FATBits == 32) {
            outInfo->m_StartCluster |= uint32_t(buffer->m_Normal.m_FirstClusterHigh) << 16;
        }            
        outInfo->m_Size                 = buffer->m_Normal.m_FileSize;
        outInfo->m_FATCreateTime        = uint32_t(buffer->m_Normal.m_CreateTime) | (uint32_t(buffer->m_Normal.m_CreateDate) << 16);
        outInfo->m_FATAccessTime        = uint32_t(buffer->m_Normal.m_AccessDate) << 16;
        outInfo->m_FATModificationTime  = uint32_t(buffer->m_Normal.m_ModificationTime) | (uint32_t(buffer->m_Normal.m_ModificationDate) << 16);
        outInfo->m_FATCreateTimeFine    = buffer->m_Normal.m_CreateTimeFine;
    }

    GetNextRawEntry();

    return true;
}

static void ValidateFATDirectoryEntry(Ptr<FATVolume> volume, Ptr<FATInode> directory, const FATDirectoryEntryInfo& info, const PString& filename)
{
    if (info.m_EndIndex < info.m_StartIndex || info.m_EndIndex - info.m_StartIndex > FAT_LONG_NAME_MAX_ENTRY_COUNT)
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FAT directory entry {} through {} has an invalid entry count.", info.m_StartIndex, info.m_EndIndex);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (filename.empty() || (info.m_DOSAttribs & FAT_VOLUME))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FAT directory entry {} has an invalid name or attributes.", info.m_StartIndex);
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    if (filename == ".")
    {
        if (directory->m_InodeID == volume->m_RootInode->m_InodeID || info.m_StartIndex != 0 || info.m_EndIndex != 0 || !(info.m_DOSAttribs & FAT_SUBDIR) || info.m_StartCluster != directory->m_StartCluster || info.m_Size != 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FAT directory inode {:x} has an invalid '.' entry.", directory->m_InodeID);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }
    else if (filename == "..")
    {
        uint32_t expectedParentCluster = 0;
        if (directory->m_ParentInodeID != volume->m_RootInode->m_InodeID) {
            expectedParentCluster = CLUSTER_OF_DIR_CLUSTER_INODEID(directory->m_ParentInodeID);
        }
        if (directory->m_InodeID == volume->m_RootInode->m_InodeID || info.m_StartIndex != 1 || info.m_EndIndex != 1 || !(info.m_DOSAttribs & FAT_SUBDIR) || info.m_StartCluster != expectedParentCluster || info.m_Size != 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FAT directory inode {:x} has an invalid '..' entry.", directory->m_InodeID);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }
    else if (info.m_DOSAttribs & FAT_SUBDIR)
    {
        if (!volume->IsDataCluster(info.m_StartCluster) || info.m_Size != 0)
        {
            kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FAT directory entry {} refers to invalid directory cluster {} or has a nonzero size.", info.m_StartIndex, info.m_StartCluster);
            PERROR_THROW_CODE(PErrorCode::IO);
        }
    }
    else if ((info.m_StartCluster == 0) != (info.m_Size == 0) || (info.m_StartCluster != 0 && !volume->IsDataCluster(info.m_StartCluster)))
    {
        kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FAT directory entry {} has invalid file cluster {} for size {}.", info.m_StartIndex, info.m_StartCluster, info.m_Size);
        PERROR_THROW_CODE(PErrorCode::IO);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATDirectoryIterator::GetNextDirectoryEntry(Ptr<FATInode> directory, ino_t* outInodeID, PString* outFilename, uint32_t* outDosAttribs)
{
    FATDirectoryEntryInfo info;

    if (!m_SectorIterator.m_Volume->CheckMagic(__func__)) {
        PERROR_THROW_CODE(PErrorCode::IO);
    }

    do
    {
        outFilename->clear();
        if (!GetNextLFNEntry(&info, outFilename)) {
            return false;
        }
        // Only hide volume label entries in the root directory.
    } while ((info.m_DOSAttribs & FAT_VOLUME) && (directory->m_InodeID == m_SectorIterator.m_Volume->m_RootInode->m_InodeID));

    ValidateFATDirectoryEntry(m_SectorIterator.m_Volume, directory, info, *outFilename);

    if (outDosAttribs != nullptr) {
	    *outDosAttribs = info.m_DOSAttribs;
    }
    if (*outFilename == ".")
    {
        // Assign inode ID based on parent.
        if (outInodeID != nullptr) *outInodeID = directory->m_InodeID;
    }
    else if (*outFilename == "..")
    {
        // Assign inode ID based on parent of parent.
        if (outInodeID != nullptr) *outInodeID = directory->m_ParentInodeID;
    }
    else
    {
        if (outInodeID != nullptr)
        {
            ino_t loc = (m_SectorIterator.m_Volume->IsDataCluster(info.m_StartCluster)) ? GENERATE_DIR_CLUSTER_INODEID(directory->m_InodeID, info.m_StartCluster) : GENERATE_DIR_INDEX_INODEID(directory->m_InodeID, info.m_StartIndex);

            // If an inode ID is already associated with the location, use that.
            if (!m_SectorIterator.m_Volume->GetLocationIDToInodeIDMapping(loc, outInodeID))
            {
                // ...else check if another inode is already using our preferred ID
                if (m_SectorIterator.m_Volume->HasInodeIDToLocationIDMapping(loc))
                {
                    // if one does, create a random one to prevent a collision
                    *outInodeID = m_SectorIterator.m_Volume->AllocUniqueInodeID();
                    // and add it to the inode cache
                    m_SectorIterator.m_Volume->SetInodeIDToLocationIDMapping(*outInodeID, loc);
                }
                else
                {
                    *outInodeID = loc;
                }
            }

            if (info.m_DOSAttribs & FAT_SUBDIR)
            {
                const ino_t mappedInodeID = m_SectorIterator.m_Volume->GetDirectoryMapping(info.m_StartCluster);
                if (mappedInodeID == -1)
                {
                    if (!m_SectorIterator.m_Volume->AddDirectoryMapping(info.m_StartCluster, *outInodeID)) {
                        PERROR_THROW_CODE(PErrorCode::IO);
                    }
                }
                else if (mappedInodeID != *outInodeID)
                {
                    kernel_log<PLogSeverity::ERROR>(LogCat_FATDIR, "FAT directory cluster {} is referenced by both inode {:x} and inode {:x}.", info.m_StartCluster, mappedInodeID, *outInodeID);
                    PERROR_THROW_CODE(PErrorCode::IO);
                }
            }
        }
    }
    if (outInodeID != nullptr) {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextDirectoryEntry(): found {} (inode ID {:x}).", *outFilename, *outInodeID);
    } else {
        kernel_log<PLogSeverity::INFO_HIGH_VOL>(LogCat_FATDIR, "FATDirectoryIterator::GetNextDirectoryEntry(): found {}.", *outFilename);
    }
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

FATDirectoryEntryCombo* FATDirectoryIterator::Rewind()
{
    if (m_CurrentIndex > (m_EntriesPerSector - 1))
    {
        if (m_CurrentBlock.m_Buffer != nullptr) {
            ReleaseCurrentBlock();
        }            
        m_SectorIterator.Set(m_StartingCluster, 0);
        m_CurrentBlock = m_SectorIterator.GetBlock_(true);
    }
    m_CurrentIndex = 0;
    return static_cast<FATDirectoryEntryCombo*>(m_CurrentBlock.m_Buffer);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

uint8_t FATDirectoryIterator::HashMSDOSName(const char* name)
{
    const uint8_t* p = reinterpret_cast<const uint8_t*>(name);
    uint8_t c = 0;
    for (int i = 0; i < 11; ++i) {
        c = uint8_t((c << 7) + (c >> 1) + *(p++));
    }
    return c;
}

} // namespace
