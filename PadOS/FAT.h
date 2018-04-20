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

#pragma once


#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include "Kernel/Drivers/HSMCIDriver.h"
#include "System/Utils/Utils.h"

namespace kernel
{

class FAT;
class File;
class Directory;

#define FAT16_CLUSTER_FREE         0x0000
#define FAT16_CLUSTER_RESERVED_MIN 0xfff0
#define FAT16_CLUSTER_RESERVED_MAX 0xfff6
#define FAT16_CLUSTER_BAD          0xfff7
#define FAT16_CLUSTER_LAST_MIN     0xfff8
#define FAT16_CLUSTER_LAST_MAX     0xffff

#define FAT32_CLUSTER_FREE         0x00000000
#define FAT32_CLUSTER_RESERVED_MIN 0x0ffffff0
#define FAT32_CLUSTER_RESERVED_MAX 0x0ffffff6
#define FAT32_CLUSTER_BAD          0x0ffffff7
#define FAT32_CLUSTER_LAST_MIN     0x0ffffff8
#define FAT32_CLUSTER_LAST_MAX     0x0fffffff
/*
inline void* operator new(size_t size)
{
    return malloc(size);
}

inline void operator delete(void* p)
{
    free(p);
}
*/
struct PartitionTabelEntry
{
    uint8_t m_Active;
    uint8_t m_CHSStartAddress[3];
    uint8_t m_Type;
    uint8_t m_CHSEndAddress[3];
    uint32_t m_LBAStart;
    uint32_t m_LBASize;
};

struct MasterBootRecord
{
    uint8_t m_BootLoader[446];
    PartitionTabelEntry m_Partitions[4];
};

struct FAT32SuperBlock
{
    uint8_t  m_JmpBoot[3];
    uint8_t  m_OEMName[8];
    uint16_t m_BytesPerSector;
    uint8_t  m_SectorsPerCluster;
    uint16_t m_ReservedSectors;
    uint8_t  m_FATCount;
    
    uint16_t m_RootDirEntryCount;   // Number of entries in the root directory. Only used for FAT16.
    uint16_t m_TotalSectorCount16;
    uint8_t  m_Media;
    uint16_t m_SectorsPerFAT16;
    uint16_t m_SectorsPerTrack;
    uint16_t m_HeadCount;
    uint32_t m_HiddenSectorCount;
    uint32_t m_TotalSectorCount32;
    
    union
    {
        struct FAT16
        {
            uint8_t  m_DriveNumber;
            uint8_t  m_Reserved1; // Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
            uint8_t  m_BootSignature; // Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
            uint32_t m_VolumeID;
            uint8_t  m_VolumeLabel[11];
            uint8_t  m_FilesType[8];
            uint8_t  m_Reserved[28];
        } FAT16 __attribute__((packed)); // 26
        struct FAT32
        {
            uint32_t m_SectorsPerFAT;
            uint16_t m_ExtendedFlags;
            uint16_t m_FSVersion;
            uint32_t m_RootDirectory;
            uint16_t m_FSInfo;
            uint16_t m_BackupBootSector;
            uint8_t  m_Reserved[12];
            uint8_t  m_DriveNumber;
            uint8_t  m_Reserved1; // Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
            uint8_t  m_BootSignature; // Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
            uint32_t m_VolumeID;
            uint8_t  m_VolumeLabel[11];
            uint8_t  m_FilesType[8];            
        } FAT32 __attribute__((packed)); // 54
    } m_FSDependent;
    
//    uint8_t  m_Unused2[19];
    
    
    
//    uint8_t  m_Unused4[462];
    uint8_t  m_Unused4[420];
    uint16_t  m_Signature; // 16 Bits Always 0xAA55    
} __attribute__((packed));

#define FAT32_ATTR_READ_ONLY (1<<0)
#define FAT32_ATTR_HIDDEN    (1<<1)
#define FAT32_ATTR_SYSTEM    (1<<2)
#define FAT32_ATTR_VOLUME_ID (1<<3)
#define FAT32_ATTR_DIR       (1<<4)
#define FAT32_ATTR_ARCHIVE   (1<<5)

struct FAT32DirEntry
{
    char m_Filename[11];
    uint8_t  m_Attribs;
    uint8_t  m_Unused1[8];
    uint16_t m_FirstClusterHigh;
    uint8_t  m_Unused2[4];
    uint16_t m_FirstClusterLow;
    uint32_t m_FileSize;
} __attribute__((packed));

struct FileEntry
{
    char     m_Name[13];
    uint32_t m_FirstCluster;
    uint8_t  m_Flags;
    uint32_t m_Size;
    
};

typedef bool (*StreamCallback)(int16_t length);

struct FileHandle
{
    FileHandle(FAT* filesystem) : m_Filesystem(filesystem) {}

    FAT* m_Filesystem;

    uint32_t    m_StartCluster = 0;     // First cluster in the file.
    uint32_t    m_Size = 0;             // File size.
    uint32_t    m_Position = 0;         // Current read/write position in the file.
    uint32_t    m_SegmentStart = 0;     // Starting cluster of the current segment.
    uint32_t    m_SegmentOffset = 0;    // Where in the file the current segment start.
    uint32_t    m_SegmentSize = 0;      // Size of the current segment.
};

enum IOError
{
    IOERR_OK,
    IOERR_UNKNOWN_FS,
    IOERR_NOT_FOUND,
    IOERR_NOT_EMPTY
};
class FAT
{
public:
    enum PartitionType_e { e_PartitionTypeFAT16, e_PartitionTypeFAT32 };
    FAT();
    ~FAT();
    bool Initialize(HSMCIDriver* blockDevice);
    bool    ChangeDirectory(const char* path);
    
    File*      OpenFile(const char* path);
    File*      OpenFile(Directory* parent, const char* name);
    
    Directory* OpenDir(const char* path);
    Directory* OpenDir(Directory* parent, const char* name);

    int16_t Read(FileHandle* file, void* buffer, int16_t size);
    int32_t Stream(FileHandle* file, int32_t size, StreamCallback callback);
    
    bool    ReadDir(FileHandle* directory, FileEntry* entry);
    void    Rewind(FileHandle* file);

    void* ReadSector(uint32_t sector);
    
private:
    void Open(FileHandle* file, uint32_t startCluster, uint32_t size);
    IOError  Open(FileHandle* file, uint32_t parentCluster, const char* path, bool openDirectory);
//    FileBase*      Open(Directory* parent, const char* path, bool openDirectory);
    bool UpdateSegment(FileHandle* file);

    friend class FileBase;
    friend class Directory;

    HSMCIDriver* m_BlockDevice = nullptr;
    uint8_t  m_SectorBuffer[512];
    
    PartitionType_e m_PartitionType;
    uint32_t m_CurrentSector;
    
    uint32_t m_PartitionStart;
    uint32_t m_PartitionSize;
    uint32_t m_DataSectorCount;
    uint16_t m_BytesPerSector;    
    uint8_t  m_SectorsPerCluster;
    uint32_t m_FATStart;
    uint8_t  m_FATCount;
    uint32_t m_SectorsPerFAT;
    uint16_t m_RootDirEntryCount;   // Number of entries in the root directory. Only used for FAT16.
    uint32_t m_ClusterStart; // Sector of first cluster.
    uint32_t m_RootDir;
    uint32_t m_CurrentDir;

    FAT( const FAT &c );
    FAT& operator=( const FAT &c );
};


//extern FAT filesystem;

class File : public FileHandle
{
public:
        
    inline int16_t Read(void* buffer, int16_t size) { return m_Filesystem->Read(this, buffer, size); }
    inline int32_t Stream(int32_t size, StreamCallback callback) { return m_Filesystem->Stream(this, size, callback); }
    void    Rewind() { m_Filesystem->Rewind(this); }

protected:
    friend class FAT;
    File(FAT* filesystem) : FileHandle(filesystem) {}
};

class Directory : public FileHandle
{
public:
    bool ReadDir( FileEntry* entry );
    void    Rewind() { m_Filesystem->Rewind(this); }

private:
    friend class FAT;
    Directory(FAT* filesystem) : FileHandle(filesystem) {}

};

} // namespace
