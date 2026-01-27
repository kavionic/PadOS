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

#pragma once

#include "System/Types.h"
#include "Ptr/Ptr.h"
#include "Kernel/VFS/KBlockCache.h"
#include "FATClusterSectorIterator.h"

class PString;


namespace kernel
{
    
class FATVolume;
class FATINode;

struct FATDirectoryEntry
{
    char     m_Filename[11];     // 0x00
    uint8_t  m_Attribs;          // 0x0b
    uint8_t  m_Unused1[8];       // 0x0c
    uint16_t m_FirstClusterHigh; // 0x14
    uint32_t m_Time;             // 0x16
    uint16_t m_FirstClusterLow;  // 0x1a
    uint32_t m_FileSize;         // 0x1c
} __attribute__((packed));

struct FATDirectoryEntryLFN
{
    uint8_t  m_SequenceNumber; // 0x00
    uint16_t m_NamePart1[5];   // 0x01
    uint8_t  m_Attribs;        // 0x0b
    uint8_t  m_Reserved1;      // 0x0c
    uint8_t  m_Hash;           // 0x0d
    uint16_t m_NamePart2[6];   // 0x0e
    uint8_t  m_Reserved2[2];   // 0x1a
    uint16_t m_NamePart3[2];   // 0x1c
} __attribute__((packed));

struct FATDirectoryEntryCombo
{
    union {
        FATDirectoryEntry    m_Normal;
        FATDirectoryEntryLFN m_LFN;    
    } __attribute__((packed));
} __attribute__((packed));


// Structure for returning data from GetNextLFNEntry()
struct FATDirectoryEntryInfo
{
    uint32_t m_StartIndex;
    uint32_t m_EndIndex;
    uint32_t m_StartCluster;
    uint32_t m_Size;
    uint32_t m_FATTime;
    uint8_t  m_DOSAttribs;
};


#define DIRI_MAGIC '!duM'
class FATDirectoryIterator
{
public:
    FATDirectoryIterator(Ptr<FATVolume> vol, uint32_t cluster, uint32_t index);
    ~FATDirectoryIterator();
    
    FATDirectoryEntryCombo* Set(uint32_t cluster, uint32_t index);
    FATDirectoryEntryCombo* GetCurrentEntry();
    FATDirectoryEntryCombo* GetNextRawEntry();

    bool                    GetNextLFNEntry(FATDirectoryEntryInfo* outInfo, PString* outFilename);
    bool                    GetNextDirectoryEntry(Ptr<FATINode> directory, ino_t* outInodeID, PString* outFilename, uint32_t* outDosAttribs);

    FATDirectoryEntryCombo* Rewind();
    void                    MarkDirty() { m_IsDirty = true; }  

    static bool RequiresLongName(const wchar16_t* longName, size_t longNameLength);
    static void MungeShortName(char* shortName, uint32_t iteration);
    static void GenerateShortName(const wchar16_t* longName, size_t longNameLength, char* shortName);

    static uint8_t  HashMSDOSName(const char *name);

private:
    void     ReleaseCurrentBlock();    
    
public:    
    FATClusterSectorIterator m_SectorIterator;
    size_t                   m_EntriesPerSector;
    bool                     m_IsDirty;
    uint32_t                 m_StartingCluster;
    uint32_t                 m_CurrentIndex;
    KCacheBlockDesc          m_CurrentBlock;
};


} // namespace

