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
// Created: 19.04.2026 22:00

#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>

#include <System/ExceptionHandling.h>
#include <Kernel/KLogging.h>
#include <Kernel/Scheduler.h>
#include <Kernel/KThreadCB.h>
#include <Kernel/KPosixSignals.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KPipeFilesystem.h>


namespace kernel
{

Ptr<KPipeVolume> KPipeFilesystem::s_Volume;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPipeVolume::KPipeVolume(fs_id volumeID, const PString& devicePath)
    : KFSVolume(volumeID, devicePath)
    , m_Mutex("pipevol", PEMutexRecursionMode_RaiseError)
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPipeInode::KPipeInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KFilesystemFileOps* fileOps)
    : KInode(filesystem, volume, fileOps, S_IFIFO | 0600)
    , m_Mutex("pipebuf", PEMutexRecursionMode_RaiseError)
    , m_ReadCondition("piperead")
    , m_WriteCondition("pipewrite")
{
    SetDontCache(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPipeFilesystem& KPipeFilesystem::Instance()
{
    static KPipeFilesystem instance;
    return instance;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KPipeVolume> KPipeFilesystem::GetVolume()
{
    return s_Volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KPipeFilesystem::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    kassert(s_Volume == nullptr);

    Ptr<KPipeFilesystem> self = ptr_tmp_cast(this);

    s_Volume = ptr_new<KPipeVolume>(volumeID, devicePath);

    // Create a minimal root directory inode to satisfy the VFS mount mechanism.
    s_Volume->m_RootNode = ptr_new<KInode>(self, s_Volume, this, S_IFDIR | 0555);
    s_Volume->m_RootNode->m_InodeID = 1;

    return s_Volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KPipeFilesystem::LoadInode(Ptr<KFSVolume> volume, ino_t inodeID)
{
    Ptr<KPipeVolume> pipeVolume = ptr_static_cast<KPipeVolume>(volume);
    KScopedLock lock(pipeVolume->m_Mutex);

    auto i = pipeVolume->m_PipeInodes.find(inodeID);
    if (i == pipeVolume->m_PipeInodes.end()) {
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }
    return i->second;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KPipeFilesystem::OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags)
{
    if (!S_ISFIFO(inode->m_FileMode)) {
        return KFilesystemFileOps::OpenFile(volume, inode, openFlags);
    }

    Ptr<KPipeInode> pipeInode = ptr_static_cast<KPipeInode>(inode);
    KScopedLock lock(pipeInode->m_Mutex);

    Ptr<KFileNode> fileNode = ptr_new<KFileNode>(openFlags);
    if (fileNode->HasWriteAccess()) {
        pipeInode->m_WriterCount++;
    } else {
        pipeInode->m_ReaderCount++;
    }

    return fileNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KPipeFilesystem::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    if (!S_ISFIFO(file->GetInode()->m_FileMode)) {
        return;
    }

    Ptr<KPipeVolume> pipeVolume = ptr_static_cast<KPipeVolume>(volume);
    Ptr<KPipeInode>  pipeInode  = ptr_static_cast<KPipeInode>(file->GetInode());

    bool  shouldErase = false;
    ino_t inodeID     = 0;
    {
        KScopedLock lock(pipeInode->m_Mutex);

        if (file->HasWriteAccess())
        {
            pipeInode->m_WriterCount--;
            if (pipeInode->m_WriterCount == 0) {
                pipeInode->m_ReadCondition.WakeupAll();  // Wake EOF readers.
            }
        }
        else
        {
            pipeInode->m_ReaderCount--;
            if (pipeInode->m_ReaderCount == 0) {
                pipeInode->m_WriteCondition.WakeupAll(); // Wake SIGPIPE writers.
            }
        }

        if (pipeInode->m_ReaderCount == 0 && pipeInode->m_WriterCount == 0)
        {
            shouldErase = true;
            inodeID     = pipeInode->m_InodeID;
        }
    }  // Pipe mutex released before acquiring volume mutex (lock-order rule).

    if (shouldErase)
    {
        KScopedLock volLock(pipeVolume->m_Mutex);
        pipeVolume->m_PipeInodes.erase(inodeID);
        // Ptr<KPipeInode> in map destroyed — ref count decremented.
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPipeFilesystem::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t /*position*/)
{
    if (length == 0) {
        return 0;
    }

    Ptr<KPipeInode> pipeInode = ptr_static_cast<KPipeInode>(file->GetInode());
    KScopedLock lock(pipeInode->m_Mutex);

    while (pipeInode->m_BytesAvailable == 0)
    {
        if (pipeInode->m_WriterCount == 0) {
            return 0;  // EOF
        }
        if (file->GetOpenFlags() & O_NONBLOCK) {
            PERROR_THROW_CODE(PErrorCode::WouldBlock);
        }
        const PErrorCode result = pipeInode->m_ReadCondition.WaitCancelable(pipeInode->m_Mutex);
        if (result == PErrorCode::Interrupted) {
            PERROR_THROW_CODE(PErrorCode::Interrupted);
        }
    }

    size_t bytesRead = 0;
    size_t toRead = std::min(length, pipeInode->m_BytesAvailable);

    // Copy from ring buffer, handling wrap-around.
    while (bytesRead < toRead)
    {
        const size_t contiguous = std::min(toRead - bytesRead, KPipeInode::PIPE_BUF_SIZE - pipeInode->m_ReadPos);
        memcpy(static_cast<uint8_t*>(buffer) + bytesRead, pipeInode->m_Buffer + pipeInode->m_ReadPos, contiguous);
        pipeInode->m_ReadPos = (pipeInode->m_ReadPos + contiguous) & (KPipeInode::PIPE_BUF_SIZE - 1);
        bytesRead += contiguous;
    }
    pipeInode->m_BytesAvailable -= bytesRead;

    pipeInode->m_WriteCondition.WakeupAll();
    return bytesRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPipeFilesystem::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t /*position*/)
{
    if (length == 0) {
        return 0;
    }

    Ptr<KPipeInode> pipeInode = ptr_static_cast<KPipeInode>(file->GetInode());
    KScopedLock lock(pipeInode->m_Mutex);

    if (pipeInode->m_ReaderCount == 0)
    {
        ksend_signal_to_thread(kget_current_thread(), SIGPIPE);
        PERROR_THROW_CODE(PErrorCode::BrokenPipe);
    }

    // For writes <= PIPE_BUF_SIZE, POSIX requires atomicity: wait until the entire
    // write fits before copying anything, so it is never interleaved with other writes.
    const bool atomic = (length <= KPipeInode::PIPE_BUF_SIZE);
    size_t written = 0;

    while (written < length)
    {
        const size_t needed = (atomic && written == 0) ? length : 1;

        // Wait until enough free space is available.
        for (;;)
        {
            const size_t freeSpace = KPipeInode::PIPE_BUF_SIZE - pipeInode->m_BytesAvailable;
            if (freeSpace >= needed) {
                break;
            }
            if (pipeInode->m_ReaderCount == 0)
            {
                ksend_signal_to_thread(kget_current_thread(), SIGPIPE);
                PERROR_THROW_CODE(PErrorCode::BrokenPipe);
            }
            if (file->GetOpenFlags() & O_NONBLOCK) {
                PERROR_THROW_CODE(PErrorCode::WouldBlock);
            }
            const PErrorCode result = pipeInode->m_WriteCondition.WaitCancelable(pipeInode->m_Mutex);
            if (result == PErrorCode::Interrupted) {
                PERROR_THROW_CODE(PErrorCode::Interrupted);
            }
        }

        const size_t currentFree = KPipeInode::PIPE_BUF_SIZE - pipeInode->m_BytesAvailable;
        const size_t toCopy = std::min(length - written, currentFree);

        // Copy into ring buffer, handling wrap-around.
        size_t copied = 0;
        while (copied < toCopy)
        {
            const size_t contiguous = std::min(toCopy - copied, KPipeInode::PIPE_BUF_SIZE - pipeInode->m_WritePos);
            memcpy(pipeInode->m_Buffer + pipeInode->m_WritePos, static_cast<const uint8_t*>(buffer) + written + copied, contiguous);
            pipeInode->m_WritePos = (pipeInode->m_WritePos + contiguous) & (KPipeInode::PIPE_BUF_SIZE - 1);
            copied += contiguous;
        }
        pipeInode->m_BytesAvailable += toCopy;
        written += toCopy;

        pipeInode->m_ReadCondition.WakeupAll();

        if (atomic) {
            break;  // Atomic write done in one pass.
        }
    }
    return written;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KPipeFilesystem::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);

    if (S_ISFIFO(inode->m_FileMode))
    {
        Ptr<KPipeInode> pipeInode = ptr_static_cast<KPipeInode>(inode);
        KScopedLock lock(pipeInode->m_Mutex);
        statBuf->st_size = static_cast<off_t>(pipeInode->m_BytesAvailable);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kpipe_trw(int pipefd[2])
{
    Ptr<KPipeVolume> volume = KPipeFilesystem::GetVolume();
    if (volume == nullptr) {
        PERROR_THROW_CODE(PErrorCode::NoEntry);
    }

    ino_t inodeID;
    Ptr<KPipeInode> pipeInode;
    {
        KScopedLock lock(volume->m_Mutex);
        inodeID   = volume->m_NextInodeID++;
        pipeInode = ptr_new<KPipeInode>(ptr_tmp_cast(&KPipeFilesystem::Instance()), volume, &KPipeFilesystem::Instance());
        pipeInode->m_InodeID = inodeID;
        volume->m_PipeInodes[inodeID] = pipeInode;
    }

    // On exception: remove from map so the inode can be freed.
    PScopeFail inodeGuard([&]()
        {
            KScopedLock lock(volume->m_Mutex);
            volume->m_PipeInodes.erase(inodeID);
        }
    );

    pipefd[0] = kopen_from_inode_trw(pipeInode, O_RDONLY);

    // If write-end open throws, close the read end.
    PScopeFail readGuard([&]() { kclose(pipefd[0]); });

    pipefd[1] = kopen_from_inode_trw(pipeInode, O_WRONLY);
}


} // namespace kernel
