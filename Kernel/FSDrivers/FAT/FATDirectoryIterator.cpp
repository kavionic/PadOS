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

#include "FATCodePage.h"
#include "FATDirectoryIterator.h"
#include "FATVolume.h"


namespace kernel
{
    
const char g_ValidShortNameCharacters[]="!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";

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
    if (UnicodeToCP437(utf16, &character))
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
        destination.append_utf32_char(CP437ToUnicode(character));
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
            destination.append_utf32_char(CP437ToUnicode(character));
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
        if (!UnicodeToCP437(longName[nameIndex], &character) || !IsShortNameCharacterCompatible(character, baseHasUppercase, baseHasLowercase)) {
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
        if (!UnicodeToCP437(longName[nameIndex], &character) || !IsShortNameCharacterCompatible(character, extensionHasUppercase, extensionHasLowercase)) {
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
