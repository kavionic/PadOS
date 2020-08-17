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
// Created: 18/05/19 13:40:19

#pragma once


#include "Kernel/VFS/KINode.h"

namespace kernel
{
class FATFilesystem;

#define ARTIFICIAL_INODEID_BITS    (0x6LL << 60)
#define DIR_CLUSTER_INODEID_BITS   (0x4LL << 60)
#define DIR_INDEX_INODEID_BITS     0
#define INVALID_INODEID_BITS_MASK  (0x9LL << 60)
#define IS_DIR_CLUSTER_INODEID(id) (((id) & ARTIFICIAL_INODEID_BITS) == DIR_CLUSTER_INODEID_BITS)
#define IS_DIR_INDEX_INODEID(id)   (((id) & ARTIFICIAL_INODEID_BITS) == DIR_INDEX_INODEID_BITS)
#define IS_ARTIFICIAL_INODEID(id)  (((id) & ARTIFICIAL_INODEID_BITS) == ARTIFICIAL_INODEID_BITS)

#define IS_INVALID_INODEID(id)                                ((!IS_DIR_CLUSTER_INODEID((id)) && !IS_DIR_INDEX_INODEID((id)) && !IS_ARTIFICIAL_INODEID((id))) ||  ((id) & INVALID_INODEID_BITS_MASK))
#define GENERATE_DIR_INDEX_INODEID(dircluster, index)         (DIR_INDEX_INODEID_BITS | (ino_t(dircluster) << 32) | (index))
#define GENERATE_DIR_CLUSTER_INODEID(dircluster, filecluster) (DIR_CLUSTER_INODEID_BITS | (ino_t(dircluster) << 32) | (filecluster))

#define CLUSTER_OF_DIR_CLUSTER_INODEID(id) (uint32_t((id) & 0xffffffff))
#define INDEX_OF_DIR_INDEX_INODEID(id)     (uint32_t((id) & 0xffffffff))
#define DIR_OF_INODEID(id)                 (uint32_t(((id) >> 32) & ~0xf0000000))


// Mode bits
#define FAT_READ_ONLY   0x01
#define FAT_HIDDEN      0x02
#define FAT_SYSTEM      0x04
#define FAT_VOLUME      0x08
#define FAT_SUBDIR      0x10
#define FAT_ARCHIVE     0x20



class FATINode : public KINode
{
public:
    static const uint32_t MAGIC = 0x6eb89a76;
    FATINode(Ptr<FATFilesystem> filesystem, Ptr<KFSVolume> volume, bool isDirectory);
    ~FATINode();
        
    bool CheckMagic(const char* functionName);

    bool Write();

    static time_t   FATTimeToUnixTime(uint32_t fatTime);
    static uint32_t UnixTimeToFATTime(time_t unixTime);

private:
    uint32_t m_Magic;
    
public:
    ino_t    m_ParentINodeID; // Parent inode number (directory containing entry)
    
    // This is incremented when the fat chain changes to tell the read/write code it needs to re-traverse the FAT chain
    uint32_t m_Iteration = 0;

    // Any changes to this block of information should immediately be written to disk/cache so that FATDirectoryIterator continues to function properly.
    uint32_t m_DirStartIndex; // Starting index of directory entry.
    uint32_t m_DirEndIndex;   // Ending index of directory entry.
    uint32_t m_StartCluster;  // Data starting cluster.
    uint32_t m_EndCluster;    // Last data cluster.
    off64_t  m_Size;          // Size in bytes.
    time_t   m_Time;          // Unix type timestamp.
    uint8_t  m_DOSAttribs;    // DOS-style attributes.

private:
    FATINode(const FATINode&) = delete;
    FATINode& operator=(const FATINode&) = delete;
};

} // namespace
