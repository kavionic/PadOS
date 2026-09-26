// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 19.04.2026 22:00

#pragma once

#include <map>
#include <sys/syslimits.h>

#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KFileHandle.h>

namespace kernel
{


class KPipeInode;

class KPipeVolume : public KFSVolume
{
public:
    KPipeVolume(fs_id volumeID, const PString& devicePath);

    KMutex                              m_Mutex;        // "pipevol"
    std::map<ino_t, Ptr<KPipeInode>>    m_PipeInodes;   // keeps anonymous pipes alive until all FDs closed
    ino_t                               m_NextInodeID = 1;
};


class KPipeInode : public KInode
{
public:
    KPipeInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KFilesystemFileOps* fileOps);

    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

    static constexpr size_t PIPE_BUF_SIZE = PIPE_BUF; // Must be a power of two.
    static_assert(PIPE_BUF_SIZE > 0 && (PIPE_BUF_SIZE & (PIPE_BUF_SIZE - 1)) == 0, "PIPE_BUF must be a power of two");

    KMutex              m_Mutex;            // "pipebuf"
    KConditionVariable  m_ReadCondition;    // "piperead"  — readers wait here when buffer is empty
    KConditionVariable  m_WriteCondition;   // "pipewrite" — writers wait here when buffer is full

    uint8_t m_Buffer[PIPE_BUF_SIZE];
    size_t  m_ReadPos         = 0;
    size_t  m_WritePos        = 0;
    size_t  m_BytesAvailable  = 0;

    int     m_ReaderCount = 0;  // Number of open read-end FDs (decremented in CloseFile)
    int     m_WriterCount = 0;  // Number of open write-end FDs (decremented in CloseFile)
};


class KPipeFilesystem : public KFilesystem, public KFilesystemFileOps
{
public:
    static KPipeFilesystem& Instance();
    static Ptr<KPipeVolume> GetVolume();

    // KFilesystem:
    virtual Ptr<KFSVolume>  Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;
    virtual Ptr<KInode>     LoadInode(Ptr<KFSVolume> volume, ino_t inodeID) override;

    // KFilesystemFileOps:
    virtual Ptr<KFileNode>  OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags) override;
    virtual void            CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;
    virtual size_t          Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t          Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void            ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

private:
    static Ptr<KPipeVolume> s_Volume;
};

void kpipe_trw(int pipefd[2]);

} // namespace kernel
