// This file is part of PadOS.
//
// Copyright (c) 2017-2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18/06/19 23:44:30

#pragma once

#include <map>
#include <vector>

#include <Kernel/KMutex.h>
#include <System/Sections.h>
#include <Ptr/Ptr.h>

class KNodeMonitorNode;

namespace kernel
{
class KFileTableNode;
class KInode;

class KIOContext
{
public:
    KIOContext();
    ~KIOContext();
    
    void Clone(const KIOContext& source);
    void Reset();

    bool AddNodeMonitor(Ptr<KNodeMonitorNode> node);

    void        SetCurrentDirectory(Ptr<KInode> inode) noexcept;
    Ptr<KInode> GetCurrentDirectory() const noexcept;

    int                 AllocFileHandle();
    void                FreeFileHandle(int handle) noexcept;
    Ptr<KFileTableNode> GetFileNode(int handle) const;
    void                SetFileNode(int handle, Ptr<KFileTableNode> node);
    int                 DupeFileHandle(const KIOContext& ioContextOld, int oldHandle, int newHandle);

private:
    int                 AllocFileHandle_pl();
    Ptr<KFileTableNode> GetFileNode_pl(int handle) const;
    void                SetFileNode_pl(int handle, Ptr<KFileTableNode> node);

    static Ptr<KFileTableNode> s_PlaceholderFile;
    mutable KMutex m_Mutex;
    Ptr<KInode> m_CurrentDirectory;

//    std::map<int, Ptr<KNodeMonitorNode>> m_NodeMonitorMap;
    
    std::vector<Ptr<KFileTableNode>>     m_FileTable;
    KIOContext(const KIOContext&) = delete;
    KIOContext& operator=(const KIOContext&) = delete;
};

} // namespace
