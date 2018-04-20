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
// Created: 08.02.2014 01:55:56

#include "sam.h"

#include <algorithm>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "SystemSetup.h"

#include "FAT.h"

using namespace kernel;

extern FILE g_DisplayFile;

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

FAT::FAT()
{
    m_PartitionStart = 0;
    m_PartitionStart = 0;
    m_CurrentSector = -1;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool FAT::Initialize(HSMCIDriver* blockDevice)
{    
    m_BlockDevice = blockDevice;
    MasterBootRecord* mbr = (MasterBootRecord*)ReadSector(0);
    if ( mbr != nullptr )
    {
        for ( uint8_t i = 0 ; i < 4 ; ++i )
        {
            const PartitionTabelEntry& partition = mbr->m_Partitions[i];
            printf("%d: %02x/%02x %ld - %ld (%ldMB)\n", i, partition.m_Active, partition.m_Type, partition.m_LBAStart, partition.m_LBASize, partition.m_LBASize / 2048);
            if ( partition.m_Type == 0x0b || partition.m_Type == 0x0c )
            {
                m_PartitionStart = partition.m_LBAStart;
                m_PartitionSize  = partition.m_LBASize;
                break;
            }
        }
        if ( m_PartitionSize == 0 ) // Assume the card is a "super floppy" without MBR.
        {
            m_PartitionStart = 0;
            m_PartitionSize = -1;
        }
        printf("Partition start: %ld, size: %ld\n", m_PartitionStart, m_PartitionSize);
        if ( m_PartitionSize > 0 )
        {
            const FAT32SuperBlock* superBlock = (const FAT32SuperBlock*)ReadSector(m_PartitionStart);
            if (superBlock != nullptr)
            {
                m_BytesPerSector    = superBlock->m_BytesPerSector;
                m_SectorsPerCluster = superBlock->m_SectorsPerCluster;
                m_FATCount          = superBlock->m_FATCount;
                m_FATStart          = m_PartitionStart + superBlock->m_ReservedSectors;
                m_SectorsPerFAT     = (superBlock->m_SectorsPerFAT16) ? uint32_t(superBlock->m_SectorsPerFAT16) : superBlock->m_FSDependent.FAT32.m_SectorsPerFAT;
                m_RootDirEntryCount = superBlock->m_RootDirEntryCount;
                m_CurrentDir        = 0;
                
                uint32_t sectorCount = (superBlock->m_TotalSectorCount32) ? superBlock->m_TotalSectorCount32 : uint32_t(superBlock->m_TotalSectorCount16);

                if ( sectorCount == 0 || m_SectorsPerFAT == 0 ) {
                    printf("Sector count is %ld, fat-size = %ld\n", sectorCount, m_SectorsPerFAT);
                    return false;
                }
                m_DataSectorCount = sectorCount - superBlock->m_ReservedSectors - m_SectorsPerFAT * m_FATCount - (m_RootDirEntryCount * 32 + m_BytesPerSector - 1) / m_BytesPerSector;
                uint32_t data_cluster_count = m_DataSectorCount / m_SectorsPerCluster;
                if ( data_cluster_count < 4085 ) {
                    printf("FAT12 is not supported\n");
                    return false; // FAT12 not supported.
                } else if ( data_cluster_count < 65525 ) {
                    m_PartitionType = e_PartitionTypeFAT16;
                    m_RootDir           = m_FATStart + m_SectorsPerFAT * m_FATCount;
                    m_ClusterStart      = m_RootDir + (m_RootDirEntryCount * 32 + m_BytesPerSector - 1) / m_BytesPerSector;
                } else {
                    m_PartitionType = e_PartitionTypeFAT32;
                    m_RootDir           = superBlock->m_FSDependent.FAT32.m_RootDirectory;
                    m_ClusterStart      = m_FATStart + m_SectorsPerFAT * m_FATCount;
                }
                printf("SZ: %d, SIG: %04x, type: %s\n", sizeof(FAT32SuperBlock), superBlock->m_Signature, (m_PartitionType==e_PartitionTypeFAT16) ? "FAT16" : "FAT32");
                printf("BPS: %d, SPC: %d, FAT#: %d FATS: %ld\n", m_BytesPerSector, m_SectorsPerCluster, m_FATCount, m_FATStart);
            }            
            return true;
        }
        printf("Partiting size is 0\n");
    }
    printf("Failed to read super block\n");
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

FAT::~FAT()
{
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool FAT::ChangeDirectory(const char* path)
{
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void FAT::Open(FileHandle* file, uint32_t startCluster, uint32_t size)
{
    file->m_StartCluster  = startCluster;
    file->m_SegmentOffset = 0;
    file->m_SegmentStart  = m_ClusterStart + (file->m_StartCluster - 2) * m_SectorsPerCluster;
    file->m_SegmentSize   = static_cast<uint32_t>(m_SectorsPerCluster) << 9;
    file->m_Size = size;
    file->m_Position = 0;
//    printf("Dir cl: %ld, sc: %d, SS: %ld/%ld\n", file->m_StartCluster, m_SectorsPerCluster, file->m_SegmentStart, file->m_SegmentSize);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

IOError FAT::Open(FileHandle* file, uint32_t parentCluster, const char* path, bool openDirectory)
{
    FileHandle parent(this);
    
    if ( parentCluster != 0 )
    {
        Open(&parent, parentCluster, ~uint32_t(0));
    }
    else
    {
        parent.m_StartCluster  = 0;
        parent.m_SegmentOffset = 0;
        parent.m_SegmentStart  = m_RootDir;
        parent.m_SegmentSize = m_RootDirEntryCount * 32;
        parent.m_Size = parent.m_SegmentSize;
        parent.m_Position = 0;        
    }    
    char name[12];
    name[11] = 0;
    memset(name, ' ', 11);
    
    char* dot = strrchr(path, '.');
    if ( dot )
    {
        memcpy(name, path, dot - path);
        strncpy(name + 8, dot + 1, 3 );
    }
    else
    {
        strncpy(name, path, 8 );
    }
    FAT32DirEntry entry;

    printf("Searching for '%11s'\n", name);
    uint32_t startCluster = 0;
    uint32_t size = 0;
    Rewind(&parent);

    while( Read(&parent, &entry, sizeof(entry)) == sizeof(entry) )
    {
        if ( entry.m_Filename[0] == 0 ) return IOERR_NOT_FOUND; // End of directory.
        if ( entry.m_Filename[0] == 0xe5 || (entry.m_Attribs & FAT32_ATTR_VOLUME_ID) ) continue; // Skip deleted files, volume ID and, LFN entres.
        
        if ( openDirectory ) {
            if ( (entry.m_Attribs & FAT32_ATTR_DIR) == 0 ) continue;
        } else {
            if ( (entry.m_Attribs & FAT32_ATTR_DIR) != 0 ) continue;
        }
        if ( entry.m_Filename[0] == 'J') { printf("Test '%11s'\n", entry.m_Filename); }
        if ( memcmp(name, entry.m_Filename, 11) == 0 )
        {
            startCluster = uint32_t(entry.m_FirstClusterHigh) << 16 | entry.m_FirstClusterLow;;
            printf("Found match at %ld\n", startCluster);
            size         = entry.m_FileSize;
            break;
        }
    }
    if ( startCluster != 0 )
    {
        Open(file, startCluster, size);
        printf("F cl: %ld, sc: %d, SS: %ld/%ld\n", file->m_StartCluster, m_SectorsPerCluster, file->m_SegmentStart, file->m_SegmentSize);
        return IOERR_OK;
    }
    printf("File not found!\n");

    return IOERR_NOT_FOUND;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

File* FAT::OpenFile(const char* path)
{
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

File* FAT::OpenFile(Directory* parent, const char* name)
{
    File* file = new File(this);
    if ( Open(file, parent->m_StartCluster, name, false) == IOERR_OK )
    {
        return file;        
    }
    else
    {
        delete file;
        return nullptr;
    }
//    File* file = static_cast<File*>(Open(parent, name, false));
//    return file;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

Directory* FAT::OpenDir(const char* path)
{
    if ( *path == '\0' )
    {
        Directory* file = new Directory(this);
        if ( m_PartitionType == e_PartitionTypeFAT16 )
        {
            file->m_StartCluster = 0;
            file->m_SegmentStart = m_RootDir;
            file->m_SegmentSize = m_RootDirEntryCount * 32;
            file->m_Size = file->m_SegmentSize;
            file->m_Position = 0; //file->m_SegmentSize - 128;
        }
        else
        {
            file->m_StartCluster = (m_CurrentDir) ? m_CurrentDir : m_RootDir;
            file->m_SegmentStart = m_ClusterStart + (file->m_StartCluster - 2) * m_SectorsPerCluster;
            file->m_SegmentSize = static_cast<uint32_t>(m_SectorsPerCluster) << 9;
            file->m_Size = 0xffffffff;
            file->m_Position = 0; //file->m_SegmentSize - 128;
        }            
        printf("Dir cl: %ld, sc: %d, SS: %ld/%ld\n", file->m_StartCluster, m_SectorsPerCluster, file->m_SegmentStart, file->m_SegmentSize);
        return file;
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

Directory* FAT::OpenDir(Directory* parent, const char* name)
{
    Directory* file = new Directory(this);
    if ( Open(file, parent->m_StartCluster, name, true) == IOERR_OK )
    {
        return file;
    }
    else
    {
        delete file;
        return nullptr;
    }
    
//    Directory* dir = static_cast<Directory*>(Open(parent, name, true));
//    return dir;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void* FAT::ReadSector(uint32_t sector)
{
    if ( sector == m_CurrentSector )
    {
        return m_SectorBuffer;
    }
    else
    {
        if ( m_BlockDevice->ReadBlocks(sector, m_SectorBuffer, 1) )
        {
            m_CurrentSector = sector;
            return m_SectorBuffer;
        }
        else
        {
            m_CurrentSector = -1; // We might have trashed the previous buffer before the failure.
            return nullptr;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
    
int16_t FAT::Read(FileHandle* file, void* buffer, int16_t size)
{
    uint8_t* dst = static_cast<uint8_t*>(buffer);
    if ( size <= 0 || file->m_Position == file->m_Size ) return 0;   // EOF
    if ( static_cast<uint16_t>(size) > file->m_Size - file->m_Position ) size = file->m_Size - file->m_Position;

    int16_t remainingBytes = size;
    while( remainingBytes )
    {
        while ( file->m_Position >= file->m_SegmentOffset + file->m_SegmentSize )
        {
            if ( !UpdateSegment(file) ) return -1;
        }
        
        uint32_t posInSegment = file->m_Position - file->m_SegmentOffset;
        int16_t bytesToRead = std::min( static_cast<uint32_t>(remainingBytes), file->m_SegmentSize - posInSegment );
        remainingBytes -= bytesToRead;
        file->m_Position += bytesToRead;
        uint32_t sector = file->m_SegmentStart + (posInSegment >> 9);
        int16_t sectorOffset = posInSegment & 0x1ff;
        
        
        // Read end of first sector if partial.
        if ( sectorOffset )
        {
            uint8_t* data = static_cast<uint8_t*>(ReadSector(sector++));
            if (data == nullptr) return -1;
            
            int16_t bytesToReadFromSector = std::min<int16_t>(bytesToRead, 512 - sectorOffset);
            
            memcpy(dst, data + sectorOffset, bytesToReadFromSector);
            dst += bytesToReadFromSector;
            bytesToRead -= bytesToReadFromSector;
        }
//        printf("bytesToRead: %d\n", bytesToRead);
        if ( bytesToRead >= 512 )
        {
            m_BlockDevice->StartReadBlocks(sector, bytesToRead / 512);
            while ( bytesToRead >= 512 )
            {
//                uint8_t* data = static_cast<uint8_t*>(ReadSector(sector++));
//                if ( data == nullptr ) return -1;
//                memcpy(dst, data, 512);
                
                
                if ( !m_BlockDevice->ReadNextBlocks(dst, 1) ) { m_BlockDevice->EndReadBlocks(); return -1; }
                sector++;
                dst += 512;
                bytesToRead -= 512;
            }
            m_BlockDevice->EndReadBlocks();
        }            
        if ( bytesToRead )
        {
            uint8_t* data = static_cast<uint8_t*>(ReadSector(sector++));
            if (data == nullptr) return -1;
            
            memcpy(dst, data, bytesToRead);
            dst += bytesToRead;
        }
    }
    return size - remainingBytes;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////
/*
int32_t FAT::Stream(FileHandle* file, int32_t size, StreamCallback callback)
{
    if ( size <= 0 || file->m_Position == file->m_Size ) return 0;   // EOF
    if ( static_cast<uint32_t>(size) > file->m_Size - file->m_Position ) size = file->m_Size - file->m_Position;

    int32_t remainingBytes = size;

    bool isReading = false;
    while( remainingBytes )
    {
        while ( file->m_Position >= file->m_SegmentOffset + file->m_SegmentSize )
        {
            if (isReading)
            {
                m_BlockDevice->EndReadBlocks();
                isReading = false;
            }
            if ( !UpdateSegment(file) ) return -1;
        }

        uint32_t posInSegment = file->m_Position - file->m_SegmentOffset;
        int32_t bytesToRead = std::min( static_cast<uint32_t>(remainingBytes), file->m_SegmentSize - posInSegment );
        remainingBytes -= bytesToRead;
        file->m_Position += bytesToRead;
        uint32_t sector = file->m_SegmentStart + (posInSegment >> 9);
        int16_t sectorOffset = posInSegment & 0x1ff;

        if ( !isReading )
        {
            isReading = true;
            m_BlockDevice->StartReadBlocks(sector);
        }        
        // Read end of first sector if partial.
        if ( sectorOffset )
        {
            sector++;
            m_BlockDevice->StartBlock();
            for ( int16_t i = 0 ; i < sectorOffset ; ++i ) m_BlockDevice->ReadNextByte();
            int16_t bytesToReadFromSector = min(bytesToRead, int32_t(512 - sectorOffset));

            callback(bytesToReadFromSector);
            m_BlockDevice->EndBlock();
            bytesToRead -= bytesToReadFromSector;
        }
        while ( bytesToRead >= 512 )
        {
            m_BlockDevice->StartBlock();
            callback(512);
            m_BlockDevice->EndBlock();
            sector++;
            bytesToRead -= 512;
        }
        if ( bytesToRead )
        {
            sector++;            
            m_BlockDevice->StartBlock();
            callback(bytesToRead);
            for ( int16_t i = 512 - bytesToRead ; i ; --i ) m_BlockDevice->ReadNextByte();
            m_BlockDevice->EndBlock();
        }
    }
    m_BlockDevice->EndReadBlocks();
    return size - remainingBytes;
}
*/
///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

void FAT::Rewind(FileHandle* file)
{
    file->m_Position = 0;
    if ( file->m_SegmentOffset != 0 )
    {
        file->m_SegmentStart = m_ClusterStart + (file->m_StartCluster - 2) * m_SectorsPerCluster;
        file->m_SegmentSize = static_cast<uint32_t>(m_SectorsPerCluster) << 9;
        file->m_SegmentOffset = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool FAT::UpdateSegment(FileHandle* file)
{
    if ( file->m_StartCluster != 0 )
    {
        if ( m_PartitionType == e_PartitionTypeFAT16 )
        {
            uint16_t currentCluster = (file->m_SegmentStart + (file->m_SegmentSize >> 9) - m_ClusterStart) / m_SectorsPerCluster + 1;
            printf("UpdateCluster: %d (%ld, %ld, %ld)\n", currentCluster, file->m_SegmentStart, file->m_SegmentSize, m_ClusterStart);
        
            uint32_t fatOffset = currentCluster * 2;
            uint32_t fatSector = m_FATStart + (fatOffset >> 9);
            printf("Load fat: %ld, %ld\n", fatOffset, fatSector);
            uint16_t* fat = static_cast<uint16_t*>(ReadSector(fatSector));
            
            if (fat != nullptr)
            {
                uint32_t i = (fatOffset / 2) & (512/2 - 1);
                currentCluster++;
                uint16_t nextCluster = fat[i];
                printf("UpdateCluster: %d\n", nextCluster);
                
                if (nextCluster == FAT16_CLUSTER_FREE || nextCluster == FAT16_CLUSTER_BAD || (nextCluster >= FAT16_CLUSTER_RESERVED_MIN && nextCluster <= FAT16_CLUSTER_RESERVED_MAX) || nextCluster >= FAT16_CLUSTER_LAST_MIN)
                {
                    return false;
                }
                if ( nextCluster == currentCluster )
                {
                    printf("Expand segment: %d\n", nextCluster);
                    file->m_SegmentSize += static_cast<uint32_t>(m_SectorsPerCluster) << 9;
                 } else {
                    printf("Set segment: %d\n", nextCluster);
                    file->m_SegmentOffset += file->m_SegmentSize;
                    file->m_SegmentStart = m_ClusterStart + (nextCluster - 2) * m_SectorsPerCluster;
                    file->m_SegmentSize = static_cast<uint32_t>(m_SectorsPerCluster) << 9;
                }
                ++i;
                for (  ; i < 512/2 ; ++i )
                {
                    currentCluster++;
                    printf("Expand segment: %ld / %d\n", i, fat[i]);
                    if ( fat[i] == currentCluster ) {
                        file->m_SegmentSize += static_cast<uint32_t>(m_SectorsPerCluster) << 9;
                    } else {
                        break;
                    }
                }
                printf("Done\n");
                return true;
            }
        }
        else
        {
            uint32_t currentCluster = (file->m_SegmentStart + (file->m_SegmentSize >> 9) - m_ClusterStart) / m_SectorsPerCluster + 1;
            uint32_t fatOffset = currentCluster * 4;
            uint32_t fatSector = m_FATStart + (fatOffset >> 9);
            uint32_t* fat = static_cast<uint32_t*>(ReadSector(fatSector));
    
            if (fat != nullptr)
            {
                uint32_t i = (fatOffset / 4) & (512/4 - 1);
                currentCluster++;
                uint32_t nextCluster = fat[i] & 0x0fffffff;
        
                //        printf("UpdateCluster: %ld\n", nextCluster);
        
                if ( nextCluster == FAT32_CLUSTER_FREE || nextCluster == FAT32_CLUSTER_BAD || (nextCluster >= FAT32_CLUSTER_RESERVED_MIN && nextCluster <= FAT32_CLUSTER_RESERVED_MAX) || (nextCluster >= FAT32_CLUSTER_LAST_MIN && nextCluster <= FAT32_CLUSTER_LAST_MAX) )
                {
                    return false;
                }
                if ( nextCluster == currentCluster )
                {
                    //            printf("Expand segment: %ld\n", nextCluster);
                    file->m_SegmentSize += static_cast<uint32_t>(m_SectorsPerCluster) << 9;
                } else {
                    //            printf("Set segment: %ld\n", nextCluster);
                    file->m_SegmentOffset += file->m_SegmentSize;
                    file->m_SegmentStart = m_ClusterStart + (nextCluster - 2) * m_SectorsPerCluster;
                    file->m_SegmentSize = static_cast<uint32_t>(m_SectorsPerCluster) << 9;
                }
                ++i;
                for (  ; i < 512/4 ; ++i )
                {
                    currentCluster++;
                    //            printf("Expand segment: %ld / %ld\n", i, fat[i]);
                    if ( fat[i] == currentCluster ) {
                        file->m_SegmentSize += static_cast<uint32_t>(m_SectorsPerCluster) << 9;
                        } else {
                        break;
                    }
                }
                //        printf("Done\n");
                return true;
            }
        }            
    }
    printf("Fail\n");
    return false;
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool Directory::ReadDir(FileEntry* entry)
{
    return m_Filesystem->ReadDir(this, entry);
}

///////////////////////////////////////////////////////////////////////////////
///
///////////////////////////////////////////////////////////////////////////////

bool FAT::ReadDir(FileHandle* directory, FileEntry* entry)
{
    FAT32DirEntry fatDirEntry;
    while( Read(directory, &fatDirEntry, sizeof(fatDirEntry)) == sizeof(fatDirEntry) )
    {
        if ( fatDirEntry.m_Filename[0] == 0 ) return false; // End of directory.
        if ( fatDirEntry.m_Filename[0] == 0xe5 || (fatDirEntry.m_Attribs & FAT32_ATTR_VOLUME_ID) ) continue; // Skip deleted files, volume ID and, LFN entres.
        
        uint8_t namePos = 0;
        for ( uint8_t i = 0 ; i < 8 ; ++i )
        {
            if ( fatDirEntry.m_Filename[i] != ' ' )
            {
                entry->m_Name[namePos++] = fatDirEntry.m_Filename[i];
            }
        }
        if ( fatDirEntry.m_Filename[8] != ' ' )
        {
            entry->m_Name[namePos++] = '.';
            for ( uint8_t i = 8 ; i < 11 && fatDirEntry.m_Filename[i] != ' ' ; ++i )
            {
                entry->m_Name[namePos++] = fatDirEntry.m_Filename[i];
            }
        }
        entry->m_Name[namePos++] = 0;
        entry->m_Flags = fatDirEntry.m_Attribs;
        entry->m_Size  = fatDirEntry.m_FileSize;
        entry->m_FirstCluster = uint32_t(fatDirEntry.m_FirstClusterHigh) << 16 | fatDirEntry.m_FirstClusterLow;
        return true;
    }
    printf("End of directory\n");
    
    return false;
}
