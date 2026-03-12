// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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
// Created: 14.02.2026 23:00

#pragma once

#include <map>

#include <sys/_winsize.h>
#include <sys/_termios.h>

#include <Utils/String.h>
#include <Kernel/KMutex.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/FSDrivers/VirtualFSBase.h>

namespace kernel
{


struct KPTYTermInfo : public PtrTarget
{
    struct termios  Termios;
    struct winsize  WinSize;
    int             SessionID;
    pid_t           PGroupID;
    uint32_t        CtrlStatus;
};

class KPTYVolume : public KVirtualFSVolume
{
public:
    KPTYVolume(fs_id volumeID, const PString& devicePath) : KVirtualFSVolume(volumeID, devicePath) {}

    Ptr<KVirtualFSBaseInode> m_MasterFolder;
    Ptr<KVirtualFSBaseInode> m_SlaveFolder;

};

class KPTYInode : public KVirtualFSBaseInode
{
public:
    KPTYInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KVirtualFSBaseInode* parent, KFilesystemFileOps* fileOps, mode_t fileMode);

    virtual bool AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode) override;

    bool IsMaster() const noexcept;
    bool IsSlave() const noexcept;

    bool NeedNewline() const noexcept;
    bool CanRead() const;

    size_t Read(void* buffer, size_t length, int openFlags);
    size_t Write(const void* buffer, size_t length, int openFlags);

    size_t WriteToMaster(const void* buffer, size_t length, int openFlags, bool echoMode);
    size_t WriteToSlave(const void* buffer, size_t length, int openFlags);

    KConditionVariable  m_IOCondition;

    Ptr<KPTYTermInfo> m_TermInfo;
    bool        m_PacketMode = false;
    KPTYInode*  m_Partner = nullptr;
    int         m_OpenCount = 0;
    size_t      m_BytesAvailable = 0;
    size_t      m_NewLineCount = 0;
    size_t      m_ReadPos = 0;
    size_t      m_WritePos = 0;
    size_t      m_CursorPos = 0;
};

class KPTYFilesystem : public KVirtualFilesystemBase
{
public:
    static constexpr ino_t INODE_ROOT   = 1;
    static constexpr ino_t INODE_MASTER = 2;
    static constexpr ino_t INODE_SLAVE  = 3;

    virtual Ptr<KFSVolume>  Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength) override;
    virtual Ptr<KFileNode>  OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags) override;
    virtual void            CloseFile(Ptr<KFSVolume> volume, KFileNode* file) override;
    virtual Ptr<KFileNode>  CreateFile(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int openFlags, int permission) override;

    virtual size_t          Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t          Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;

    virtual void            DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

    static fs_id s_VolumeID;
};

void kdisassociate_controlling_tty_trw(bool sendSIGCONT);

void ktcsetattr_trw(int fd, int optionalActions, const struct termios* termios);
void ktcgetattr_trw(int fd, struct termios* termios);

void ktcsetpgrp_trw(int fd, pid_t pgroup);
pid_t ktcgetpgrp_trw(int fd);


} // namespace kernel
