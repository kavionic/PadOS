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

#include "sam.h"

#include <string.h>
#include <algorithm>

#include "System/Utils/Utils.h"

#include "FATDirectoryIterator.h"
#include "FATVolume.h"
#include "FATFilesystem.h"

using namespace os;

namespace kernel
{
    
const char g_ValidShortNameCharacters[]="!#$%&'()-0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ^_`{}~";
const char illegal[]   = "\\/:*?\"<>|";
const char underbar[]  = "+,;=[]"
                        "\x83\x85\x88\x89\x8A\x8B\x8C\x8D"
                        "\x93\x95\x96\x97\x98"
                        "\xA0\xA1\xA2\xA3";
const char capitalize_from[] = "\x81\x82\x84\x86\x87\x91\x94\xA4";
const char capitalize_to[]   = "\x9A\x90\x8E\x8F\x80\x92\x99\xA5";

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

uint16_t CP437ToUTF16(uint8_t cp437)
{
    if (cp437 < 0x80) {
        return cp437;
    } else {
        return g_CP437ToUTF162[cp437 - 0x80];
    }
}

static bool UTF16ToCP437(uint16_t unicode, uint8_t* result)
{
    if (unicode < 0x80)
    {
        *result = unicode;
        return true;
    }
    else
    {
        auto i = std::lower_bound(&g_UTF16ToCP437[0], &g_UTF16ToCP437[256], unicode);
        if (i != &g_UTF16ToCP437[256] && i->m_UTF16 == unicode) {
            *result = i->m_CP437;
            return true;
        } else {
            return false;
        }
    }        
}

static bool FilteredUTF16ToCP437(uint16_t utf16, uint8_t* result)
{
    uint8_t character;
    if (UTF16ToCP437(utf16, &character))
    {
        if (strchr(illegal, character)) {
            set_last_error(EINVAL);
            return -1;
        }                
        char *cp;
        if (character >= 'a' && character <= 'z') {
            *result = character - 'a' + 'A';
            return true;
        } else if (strchr(underbar, character)) {
            *result = '_';
            return true;
        } else if ((cp = strchr(capitalize_from, character)) != nullptr) {
            *result = capitalize_to[(int)(cp - capitalize_from)];
            return true;
        } else if (strchr(g_ValidShortNameCharacters, character) || strchr(capitalize_to, character)) {
            *result = character;
            return true;
        }
    }
    return false;
}

static status_t FATRawShortNameToUTF8(const char* src, String* dst)
{
    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATRawShortNameToUTF8().\n");

    try
    {
        for (int i = 0; i < 8 && src[i] != ' '; ++i) {
            dst->append(CP437ToUTF16((i == 0 && src[i] == 5) ? 0xe5 : uint8_t(src[i])));
        }

        if (src[8] != ' ')
        {
            *dst += ".";
            for (int i = 8; i < 11 && src[i] != ' '; ++i) {
                if (src[i] == ' ') break;
                dst->append(CP437ToUTF16(uint8_t(src[i])));
            }
        }
        return 0;
    }
    catch (const std::bad_alloc&)
    {
        set_last_error(ENOMEM);
        return -1;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool FATDirectoryIterator::RequiresLongName(const wchar16_t* longName, size_t longNameLength)
{
    size_t i;

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATDirectoryIterator::RequiresLongName().\n");

    for (i = 0; i < 8; ++i)
    {
        if (i == longNameLength)  return false;
        uint8_t character;
        if (!UTF16ToCP437(longName[i], &character)) return true;
        if (character == '.') break;
        if (strchr(g_ValidShortNameCharacters, character) == nullptr) return true;
    }

    if (i == longNameLength) return false;
    if (i == 8 && longName[i] != '.') return true; // Name too long.
    i++;
    if (i == longNameLength) return true; // Filenames with trailing periods.

    for (size_t j = 0; j < 3; ++j, ++i)
    {
        if (i == longNameLength) return false;
        uint8_t character;
        if (!UTF16ToCP437(longName[i], &character)) return true;
        if (strchr(g_ValidShortNameCharacters, character) == nullptr) return true;
    }
    return i != longNameLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FATDirectoryIterator::MungeShortName(char* shortName, uint32_t value)
{
    char buffer[8];

    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATDirectoryIterator::MungeShortName().\n");

    // Short names must have only numbers following the tilde and cannot begin with 0
    sprintf(buffer, "~%" PRIu32, value);
    int len = strlen(buffer);
    int i = 7 - len;

    kassert(i > 0 && i < 8);

    while (shortName[i] == ' ' && i > 0) i--;
    i++;

    memcpy(shortName + i, buffer, len);

    return 0;
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

status_t FATDirectoryIterator::GenerateShortName(const wchar16_t* longName, size_t longNameLength, char* shortName)
{
    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATDirectoryIterator::GenerateShortName().\n");

    memset(shortName, ' ', 11);

    ssize_t srcPos = 0;
    size_t  dstPos = 0;
    for (; srcPos < longNameLength && dstPos < 8; ++srcPos)
    {
        if (longName[srcPos] == 0) {
            if (dstPos == 0) shortName[0] = '_';
            return 0;
        }            
        if (longName[srcPos] == '.')
        {
            if (dstPos == 0) {
                continue; // Skip leading dots.
            } else {
                break;    // Extension found.
            }
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
    if (srcPos < 0) return 0;

    srcPos++;

    for (size_t dstPos = 8; dstPos < 11 && srcPos < longNameLength; ++srcPos)
    {
        if (longName[srcPos] == 0) return 0;

        uint8_t c;
        if (FilteredUTF16ToCP437(longName[srcPos], &c))
        {
            shortName[dstPos++] = c;
        }        
    }
    return 0;
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
            kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_LOW_VOL, "FATDirectoryIterator::ReleaseCurrentBlock(): Writing updated directory entries.\n");
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
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::FATDirectoryIterator() cluster %d outside volume.\n", cluster);
    }
    m_StartingCluster = cluster;
    m_CurrentIndex    = index;
    if (index >= m_EntriesPerSector)
    {
        if (m_SectorIterator.Increment(m_CurrentIndex / m_EntriesPerSector) != 0) {
            return;
        }            
    }
    m_CurrentBlock = m_SectorIterator.GetBlock();
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
        return nullptr;
    }
    ;
    if (m_SectorIterator.Set(cluster, 0) < 0) {
        return nullptr;
    }
    m_IsDirty = false;
    m_StartingCluster = cluster;
    m_CurrentIndex    = index;
    if (index >= m_EntriesPerSector)
    {
        if (m_SectorIterator.Increment(m_CurrentIndex / m_EntriesPerSector) != 0) {
            return nullptr;
        }
    }

    m_CurrentBlock = m_SectorIterator.GetBlock();

    if (m_CurrentBlock.m_Buffer == nullptr) {
        return nullptr;
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
        if (m_SectorIterator.Increment(1) < 0) {
            return nullptr;
        }            
        m_CurrentBlock = m_SectorIterator.GetBlock();
        if (m_CurrentBlock.m_Buffer == nullptr) {
            return nullptr;
        }            
    }
    return static_cast<FATDirectoryEntryCombo*>(m_CurrentBlock.m_Buffer) + (m_CurrentIndex % m_EntriesPerSector);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FATDirectoryIterator::GetNextLFNEntry(FATDirectoryEntryInfo* oinfo, String* filename)
{
    uint8_t            hash = 0;
    std::vector<wchar16_t> utf16Buffer;

    if (filename != nullptr) {
        utf16Buffer.resize(512);
    }
    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATDirectoryIterator::GetNextLFNEntry(): %ld\n", m_CurrentIndex);

    // LFN state
    uint32_t startIndex  = 0xffff;
    uint32_t filenameLen = 0;
    uint32_t lfnCount    = 0;

    FATDirectoryEntryCombo* buffer;
    for (buffer = GetCurrentEntry(); buffer != nullptr; buffer = GetNextRawEntry())
    {
        kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATDirectoryIterator::GetNextLFNEntry(): %lx/%lx/%ld\n", m_SectorIterator.m_CurrentCluster, m_SectorIterator.m_CurrentSector, m_CurrentIndex);
        if (buffer->m_LFN.m_SequenceNumber == 0) // quit if at end of table
        {
            if (startIndex != 0xffff) {
                kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::GetNextLFNEntry(): LFN entry (%s) with no alias.\n", (filename != nullptr) ? filename->c_str() : "*none*");
            }
            set_last_error(ENOENT);
            return -1;
        }
        
        if (buffer->m_LFN.m_SequenceNumber == 0xe5) // skip erased entries
        {
            if (startIndex != 0xffff) {
                kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::WARNING, "FATDirectoryIterator::GetNextLFNEntry(): LFN entry (%s) with intervening erased entries.\n", (filename != nullptr) ? filename->c_str() : "*none*");
                startIndex = 0xffff;
            }
            kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATDirectoryIterator::GetNextLFNEntry(): Entry erased, skipping...\n");
            continue;
        }
        
        if (buffer->m_LFN.m_Attribs == 0x0f) // long file name
        {
            if ((buffer->m_LFN.m_Reserved1 != 0) || (buffer->m_LFN.m_Reserved2[0] != 0) || (buffer->m_LFN.m_Reserved2[1] != 0)) {
                kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::GetNextLFNEntry(): Invalid LFN entry: reserved fields clobbered.\n");
                continue;
            }
            if (startIndex == 0xffff)
            {
                if ((buffer->m_LFN.m_SequenceNumber & 0x40) == 0) {
                    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::GetNextLFNEntry(): Bad LFN start entry in directory.\n");
                    continue;
                }
                hash = buffer->m_LFN.m_Hash;
                lfnCount = buffer->m_LFN.m_SequenceNumber & 0x1f;
                startIndex = m_CurrentIndex;
                
                if (filename == nullptr) {
                    continue;
                }
                
                wchar16_t* dst = utf16Buffer.data() + 13 * (lfnCount - 1);
                
                bool done = false;
                for (int i = 0; !done && i < ARRAY_COUNT(buffer->m_LFN.m_NamePart1); ++i) {
                    if (buffer->m_LFN.m_NamePart1[i] != 0xffff) {
                        *dst++ = buffer->m_LFN.m_NamePart1[i];
                    } else {
                        done = true;
                    }
                }
                for (int i = 0; !done && i < ARRAY_COUNT(buffer->m_LFN.m_NamePart2); ++i) {
                    if (buffer->m_LFN.m_NamePart2[i] != 0xffff) {
                        *dst++ = buffer->m_LFN.m_NamePart2[i];
                    } else {
                        done = true;
                    }
                }
                for (int i = 0; !done && i < ARRAY_COUNT(buffer->m_LFN.m_NamePart3); ++i) {
                    if (buffer->m_LFN.m_NamePart3[i] != 0xffff) {
                        *dst++ = buffer->m_LFN.m_NamePart3[i];
                    } else {
                        done = true;
                    }
                }
                filenameLen = dst - utf16Buffer.data() - 1;
                continue;
            }
            else
            {
                if (buffer->m_LFN.m_Hash != hash) {
                    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::GetNextLFNEntry(): Error in long file name: hash values don't match.\n");
                    startIndex = 0xffff;
                    continue;
                }
                if (buffer->m_LFN.m_SequenceNumber != --lfnCount) {
                    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::GetNextLFNEntry(): Bad LFN sequence number.\n");
                    startIndex = 0xffff;
                    continue;
                }
                if (filename != nullptr)
                {
                    wchar16_t* dst = utf16Buffer.data() + 13 * (lfnCount - 1);
                    memcpy(dst, buffer->m_LFN.m_NamePart1, sizeof(buffer->m_LFN.m_NamePart1));
                    dst += ARRAY_COUNT(buffer->m_LFN.m_NamePart1);
                    memcpy(dst, buffer->m_LFN.m_NamePart2, sizeof(buffer->m_LFN.m_NamePart2));
                    dst += ARRAY_COUNT(buffer->m_LFN.m_NamePart2);
                    memcpy(dst, buffer->m_LFN.m_NamePart3, sizeof(buffer->m_LFN.m_NamePart3));
                }                
                continue;
            }
        }
        break;
    }

    // Hit end of directory with no luck
    if (buffer == nullptr) {
        set_last_error(ENOENT);
        return -1;
    }        

    // Process long name
    if (startIndex != 0xffff)
    {
        if (lfnCount != 1)
        {
            kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::GetNextLFNEntry(): Unfinished LFN in directory\n");
            startIndex = 0xffff;
        }
        else
        {
            if (filename != nullptr) {
                filename->assign_utf16(utf16Buffer.data(), filenameLen);
            }            
            if (HashMSDOSName(buffer->m_Normal.m_Filename) != hash)
            {
                kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::CRITICAL, "FATDirectoryIterator::GetNextLFNEntry(): long file name (%s) hash and short file name (%11.11s) don't match\n", ((filename != nullptr) ? filename->c_str() : "*none*"), buffer->m_Normal.m_Filename);
                startIndex = 0xffff;
            }
        }
    }

    // Process short name
    if (startIndex == 0xffff) {
        startIndex = m_CurrentIndex;
        if (filename != nullptr) {
            FATRawShortNameToUTF8(buffer->m_Normal.m_Filename, filename);
        }            
    }

    if (oinfo)
    {
        oinfo->m_StartIndex = startIndex;
        oinfo->m_EndIndex   = m_CurrentIndex;
        oinfo->m_DOSAttribs = buffer->m_Normal.m_Attribs;
        oinfo->m_StartCluster = buffer->m_Normal.m_FirstClusterLow;
        if (m_SectorIterator.m_Volume->m_FATBits == 32) {
            oinfo->m_StartCluster |= uint32_t(buffer->m_Normal.m_FirstClusterHigh) << 16;
        }            
        oinfo->m_Size = buffer->m_Normal.m_FileSize;
        oinfo->m_FATTime = buffer->m_Normal.m_Time;
    }

    GetNextRawEntry();

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

status_t FATDirectoryIterator::GetNextDirectoryEntry(Ptr<FATINode> directory, ino_t* inodeID, String* filename)
{
    FATDirectoryEntryInfo info;
    status_t result;

    if (!m_SectorIterator.m_Volume->CheckMagic(__func__)) {
        set_last_error(EINVAL);
        return -1;
    }

    do {
        filename->clear();
        result = GetNextLFNEntry(&info, filename);
        if (result < 0) return result;
        // Only hide volume label entries in the root directory.
    } while ((info.m_DOSAttribs & FAT_VOLUME) && (directory->m_INodeID == m_SectorIterator.m_Volume->m_RootINode->m_INodeID));
    
    if (*filename == ".")
    {
        // Assign inode ID based on parent.
        if (inodeID != nullptr) *inodeID = directory->m_INodeID;
    }
    else if (*filename == "..")
    {
        // Assign inode ID based on parent of parent.
        if (inodeID != nullptr) *inodeID = directory->m_ParentINodeID;
    }
    else
    {
        if (inodeID != nullptr)
        {
            ino_t loc = (m_SectorIterator.m_Volume->IsDataCluster(info.m_StartCluster)) ? GENERATE_DIR_CLUSTER_INODEID(directory->m_INodeID, info.m_StartCluster) : GENERATE_DIR_INDEX_INODEID(directory->m_INodeID, info.m_StartIndex);

            // If an inode ID is already associated with the location, use that.
            if (!m_SectorIterator.m_Volume->GetLocationIDToINodeIDMapping(loc, inodeID))
            {
                // ...else check if another inode is already using our preferred ID
                if (m_SectorIterator.m_Volume->HasINodeIDToLocationIDMapping(loc))
                {
                    // if one does, create a random one to prevent a collision
                    *inodeID = m_SectorIterator.m_Volume->AllocUniqueINodeID();
                    // and add it to the inode cache
                    if (!m_SectorIterator.m_Volume->SetINodeIDToLocationIDMapping(*inodeID, loc)) {
                        return -1;
                    }
                }
                else
                {
                    *inodeID = loc;
                }
            }

            if (info.m_DOSAttribs & FAT_SUBDIR)
            {
                if (m_SectorIterator.m_Volume->GetDirectoryMapping(info.m_StartCluster) == -1) {
                    if (!m_SectorIterator.m_Volume->AddDirectoryMapping(*inodeID)) {
                        return -1;
                    }
                }
            }
        }
    }
    kernel_log(FATFilesystem::LOGC_DIR, KLogSeverity::INFO_HIGH_VOL, "FATDirectoryIterator::GetNextDirectoryEntry(): found %s (inode ID %" PRIx64 ").\n", filename->c_str(), *inodeID);
    return 0;
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
        if (m_SectorIterator.Set(m_StartingCluster, 0) < 0) {
            return nullptr;
        }            
        m_CurrentBlock = m_SectorIterator.GetBlock();
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
        c = (c << 7) + (c >> 1) + *(p++);
    }
    return c;
}

} // namespace
