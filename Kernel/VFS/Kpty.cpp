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

#include <atomic>
#include <set>

#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ttycom.h>
#include <sys/filio.h>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/KProcess.h>
#include <Kernel/KHandleArray.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/FileIO.h>
#include <Kernel/VFS/Kpty.h>
#include <System/System.h>
#include <System/ExceptionHandling.h>
#include <System/AppDefinition.h>
#include <Utils/String.h>


namespace kernel
{

fs_id KPTYFilesystem::s_VolumeID = -1;


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KPTYFilesystem::Mount(fs_id volumeID, const char* devicePath, uint32_t flags, const char* args, size_t argLength)
{
    kassert(s_VolumeID == -1);
    s_VolumeID = volumeID;

    Ptr<KPTYVolume> volume = ptr_new<KPTYVolume>(volumeID, devicePath);
    DoMount(volume, flags, args, argLength);

    const Ptr<KVirtualFSBaseInode> rootNode = ptr_static_cast<KVirtualFSBaseInode>(volume->m_RootNode);

    volume->m_MasterFolder = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(rootNode), this, S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    volume->m_SlaveFolder = ptr_new<KVirtualFSBaseInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(rootNode), this, S_IFDIR | S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

    volume->m_MasterFolder->m_InodeID = INODE_MASTER;
    volume->m_SlaveFolder->m_InodeID  = INODE_SLAVE;

    rootNode->m_Children["master"] = volume->m_MasterFolder;
    rootNode->m_Children["slave"] = volume->m_SlaveFolder;

    return volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KPTYFilesystem::OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> inode, int openFlags)
{
    Ptr<KFileNode> file = KFilesystemFileOps::OpenFile(volume, inode, openFlags);
    Ptr<KPTYInode> fsInode = ptr_static_cast<KPTYInode>(inode);
    fsInode->m_OpenCount++;

    if (fsInode->IsSlave())
    {
        if (fsInode->m_TermInfo->SessionID == -1)
        {
            const KProcess& process = kget_current_process();
            KIOContext& ioContext = kget_io_context(KLocateFlag::None);

            if ((openFlags & O_NOCTTY) == 0 && process.IsGroupLeader() && ioContext.GetControllingTTY() == nullptr)
            {
                ioContext.SetControllingTTY(fsInode);
                fsInode->m_TermInfo->SessionID = process.GetSession();
                fsInode->m_TermInfo->PGroupID  = process.GetPGroupID();
            }
        }
    }
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KPTYFilesystem::CloseFile(Ptr<KFSVolume> volume, KFileNode* file)
{
    Ptr<KPTYInode> inode = ptr_static_cast<KPTYInode>(file->GetInode());
    inode->m_OpenCount--;
    KFilesystemFileOps::CloseFile(volume, file);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileNode> KPTYFilesystem::CreateFile(Ptr<KFSVolume> volume, Ptr<KInode> parent, const char* name, int nameLength, int openFlags, int permission)
{
    const Ptr<KPTYVolume>           fsVolume = ptr_static_cast<KPTYVolume>(volume);
    const Ptr<KVirtualFSBaseInode>  fsParent = ptr_static_cast<KVirtualFSBaseInode>(parent);

    kassert(!fsVolume->m_Mutex.IsLocked());
    CRITICAL_SCOPE(fsVolume->m_Mutex);


    if (parent != fsVolume->m_MasterFolder) {
        PERROR_THROW_CODE(PErrorCode::NoAccess);
    }
    if (LocateInodeInternal(volume, parent, name, nameLength) != nullptr) {
        PERROR_THROW_CODE(PErrorCode::Exist);
    }

    Ptr<KPTYInode> master = ptr_new<KPTYInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(fsParent), this, S_IFCHR | permission);
    Ptr<KPTYInode> slave = ptr_new<KPTYInode>(ptr_tmp_cast(this), volume, ptr_raw_pointer_cast(fsVolume->m_SlaveFolder), this, S_IFCHR | permission);

    master->m_Partner = ptr_raw_pointer_cast(slave);
    slave->m_Partner = ptr_raw_pointer_cast(master);

    Ptr<KPTYTermInfo> termInfo = ptr_new<KPTYTermInfo>();
    master->m_TermInfo = termInfo;
    slave->m_TermInfo  = termInfo;

    termInfo->Termios = {};
    termInfo->WinSize = {};

    termInfo->Termios.c_iflag = BRKINT | ICRNL | IXON;
    termInfo->Termios.c_oflag = OPOST | ONLCR;
    termInfo->Termios.c_cflag = CS8 | CREAD | HUPCL | CLOCAL;
    termInfo->Termios.c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN | ECHOCTL | ECHOKE;

    termInfo->Termios.c_cc[VEOF]        = 4;    // ^D
    termInfo->Termios.c_cc[VEOL]        = _POSIX_VDISABLE;
    termInfo->Termios.c_cc[VEOL2]       = _POSIX_VDISABLE;
    termInfo->Termios.c_cc[VERASE]      = 8;    // ^H
    termInfo->Termios.c_cc[VWERASE]     = 23;   // ^W
    termInfo->Termios.c_cc[VKILL]       = 21;   // ^U
    termInfo->Termios.c_cc[VREPRINT]    = 18;   // ^R
    termInfo->Termios.c_cc[VINTR]       = 3;    // ^C
    termInfo->Termios.c_cc[VQUIT]       = 28;   // ^backslash
    termInfo->Termios.c_cc[VSUSP]       = 26;   // ^Z
    termInfo->Termios.c_cc[VSTART]      = 17;   // ^Q
    termInfo->Termios.c_cc[VSTOP]       = 19;   // ^S
    termInfo->Termios.c_cc[VLNEXT]      = 22;   // ^V
    termInfo->Termios.c_cc[VDISCARD]    = 15;   // ^O
    termInfo->Termios.c_cc[VMIN]        = 1;    // min chars for read
    termInfo->Termios.c_cc[VTIME]       = 0;    // timeout (deciseconds)

    termInfo->WinSize.ws_row = 80;
    termInfo->WinSize.ws_col = 25;
    termInfo->WinSize.ws_xpixel = 80 * 8;
    termInfo->WinSize.ws_ypixel = 25 * 8;

    termInfo->PGroupID = -1;
    termInfo->SessionID = -1;

    // Must be power of two.
    master->m_FileData.resize(512);
    slave->m_FileData.resize(4096);

    PString deviceName(name, nameLength);

    fsVolume->m_MasterFolder->m_Children[deviceName] = master;
    fsVolume->m_SlaveFolder->m_Children[deviceName]  = slave;

    Ptr<KFileNode> fileNode = ptr_new<KFileNode>(openFlags);
    fileNode->SetInode(master);
    master->m_OpenCount = 1;

    return fileNode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPTYFilesystem::Read(Ptr<KFileNode> file, void* buffer, size_t length, off64_t position)
{
    Ptr<KPTYInode> inode = ptr_static_cast<KPTYInode>(file->GetInode());
    Ptr<KVirtualFSVolume> volume = ptr_static_cast<KVirtualFSVolume>(inode->m_Volume);

    CRITICAL_SCOPE(volume->m_Mutex);

    return inode->Read(buffer, length, file->GetOpenFlags());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPTYFilesystem::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position)
{
    Ptr<KPTYInode>          inode = ptr_static_cast<KPTYInode>(file->GetInode());
    Ptr<KVirtualFSVolume>   volume = ptr_static_cast<KVirtualFSVolume>(inode->m_Volume);
    size_t                  bytesWritten = 0;

    CRITICAL_SCOPE(volume->m_Mutex);

    switch (inode->m_Parent->m_InodeID)
    {
        case INODE_SLAVE:
            bytesWritten = inode->m_Partner->WriteToMaster(buffer, length, file->GetOpenFlags(), false);
            break;
        case INODE_MASTER:
            bytesWritten = inode->m_Partner->WriteToSlave(buffer, length, file->GetOpenFlags());
            break;
        default:
            PERROR_THROW_CODE(PErrorCode::NotImplemented);
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KPTYFilesystem::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<KPTYInode>          inode = ptr_static_cast<KPTYInode>(file->GetInode());
    Ptr<KVirtualFSVolume>   volume = ptr_static_cast<KVirtualFSVolume>(inode->m_Volume);

    CRITICAL_SCOPE(volume->m_Mutex);

    switch (uint32_t(request))
    {
        case TCSETA:
        case TCSETAW:
        case TCSETAF:
            if (inDataLength == sizeof(inode->m_TermInfo->Termios)) {
                inode->m_TermInfo->Termios = *static_cast<const struct termios*>(inData);
            } else {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            break;
        case TCGETA:
            if (outDataLength == sizeof(inode->m_TermInfo->Termios)) {
                *static_cast<struct termios*>(outData) = inode->m_TermInfo->Termios;
            } else {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            break;
        case TIOCSWINSZ:
            if (inDataLength == sizeof(inode->m_TermInfo->WinSize)) {
                inode->m_TermInfo->WinSize = *static_cast<const struct winsize*>(inData);
            } else {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            if (inode->IsMaster() && inode->m_TermInfo->PGroupID != -1) {
                kkill(-inode->m_TermInfo->PGroupID, SIGWINCH);
            }
            break;
        case TIOCGWINSZ:
            if (outDataLength == sizeof(inode->m_TermInfo->WinSize)) {
                *static_cast<struct winsize*>(outData) = inode->m_TermInfo->WinSize;
            } else {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            break;
        case TIOCSPGRP: // Set foreground process group
        {
            if (inDataLength != sizeof(pid_t)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            const KProcess&     process = kget_current_process();
            const KIOContext&   ioContext = kget_io_context(KLocateFlag::None);
            const pid_t         pgroup = *static_cast<const pid_t*>(inData);

            KProcessLock processLock;

            if (ioContext.GetControllingTTY() != inode || process.GetSession() != inode->m_TermInfo->SessionID)
            {
                kernel_log<PLogSeverity::NOTICE>(LogCatKernel_PTY, "{} rejecting TIOCSPGRP. ctrl-tty = {}, proc-session = {}, pty-session = {}",
                    __PRETTY_FUNCTION__, reinterpret_cast<const void*>(ptr_raw_pointer_cast(ioContext.GetControllingTTY())), process.GetSession(), inode->m_TermInfo->SessionID);

                PERROR_THROW_CODE(PErrorCode::NOTTY);
            }
            inode->m_TermInfo->PGroupID = pgroup;
            break;
        }
        case TIOCGPGRP: // Get foreground process group
        {
            const KIOContext& ioContext = kget_io_context(KLocateFlag::None);

            KProcessLock processLock;
            if (ioContext.GetControllingTTY() != inode) {
                PERROR_THROW_CODE(PErrorCode::NOTTY);
            }
            *static_cast<pid_t*>(outData) = inode->m_TermInfo->PGroupID;
            break;
        }
        case TIOCNOTTY:
        {
            KIOContext& ioContext = kget_io_context(KLocateFlag::None);
            if (ioContext.GetControllingTTY() != inode) {
                PERROR_THROW_CODE(PErrorCode::NOTTY);
            }
            const KProcess& process = kget_current_process();
            if (process.IsGroupLeader()) {
                kdisassociate_controlling_tty_trw(true);
            }
            ioContext.SetControllingTTY(nullptr);
            break;
        }
        case FIONBIO:
        {
            if (inDataLength != sizeof(int)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            int nArg = *static_cast<const int*>(inData);
            if (nArg == 0) {
                file->SetOpenFlags(file->GetOpenFlags() & ~O_NONBLOCK);
            } else {
                file->SetOpenFlags(file->GetOpenFlags() | O_NONBLOCK);
            }
            break;
        }
        case FIONREAD:
        {
            if (outDataLength != sizeof(int)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            int length;
            if (inode->IsSlave() && (inode->m_TermInfo->Termios.c_lflag & ICANON)) {
                length = (inode->m_NewLineCount > 0) ? inode->m_BytesAvailable : 0;
            } else {
                length = inode->m_BytesAvailable;
            }
            *static_cast<int*>(outData) = length;
            break;
        }
        case TIOCPKT:
        {
            if (inDataLength != sizeof(int)) {
                PERROR_THROW_CODE(PErrorCode::InvalidArg);
            }
            if (!inode->IsMaster()) {
                PERROR_THROW_CODE(PErrorCode::NOTTY);
            }
            int arg = *static_cast<const int*>(inData);
            if (arg == 0)
            {
                inode->m_PacketMode = false;
            }
            else
            {
                if (!inode->m_PacketMode)
                {
                    inode->m_PacketMode = true;
                    inode->m_TermInfo->CtrlStatus = 0;
                }
            }
            break;
        }
        default:
            kernel_log<PLogSeverity::NOTICE>(LogCatKernel_PTY, "{} Unknown command {}", __PRETTY_FUNCTION__, request);
            PERROR_THROW_CODE(PErrorCode::InvalidArg);
            break;

    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KPTYInode::KPTYInode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume, KVirtualFSBaseInode* parent, KFilesystemFileOps* fileOps, mode_t fileMode)
    : KVirtualFSBaseInode(filesystem, volume, parent, fileOps, fileMode)
    , m_IOCondition("ptyio")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KPTYInode::AddListener(KThreadWaitNode* waitNode, ObjectWaitMode mode)
{
    Ptr<KVirtualFSVolume>   volume = ptr_static_cast<KVirtualFSVolume>(m_Volume);

    kassert(!volume->m_Mutex.IsLocked());
    CRITICAL_SCOPE(volume->m_Mutex);

    switch (mode)
    {
        case ObjectWaitMode::Read:
        case ObjectWaitMode::ReadWrite:
            if (!CanRead()) {
                return m_IOCondition.AddListener(waitNode, ObjectWaitMode::Read);
            } else {
                return false; // Will not block.
            }
        case ObjectWaitMode::Write:
            return false;
        default:
            return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KPTYInode::IsMaster() const noexcept
{
    return m_Parent->m_InodeID == KPTYFilesystem::INODE_MASTER;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KPTYInode::IsSlave() const noexcept
{
    return m_Parent->m_InodeID == KPTYFilesystem::INODE_SLAVE;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KPTYInode::NeedNewline() const noexcept
{
    return IsSlave() && (m_TermInfo->Termios.c_lflag & ICANON) != 0;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool KPTYInode::CanRead() const
{
    if (m_Parent == nullptr || m_TermInfo == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    return m_NewLineCount != 0 || (!NeedNewline() && m_BytesAvailable != 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPTYInode::Read(void* buffer, size_t length, int openFlags)
{
    uint8_t* bufferDst = static_cast<uint8_t*>(buffer);

    Ptr<KVirtualFSVolume> volume = ptr_static_cast<KVirtualFSVolume>(m_Volume);

    kassert(volume->m_Mutex.IsLocked());

    if (length == 0) {
        return 0;
    }

    if (m_TermInfo == nullptr || m_FileData.empty())
    {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }

    while (!CanRead() && !khas_pending_signals())
    {
        if (m_Partner == nullptr || m_Partner->m_OpenCount == 0) {
            return 0;
        }
        if (openFlags & O_NONBLOCK) {
            PERROR_THROW_CODE(PErrorCode::WouldBlock);
        }
        m_IOCondition.Wait(volume->m_Mutex);
    }
    if (!CanRead()) {
        PERROR_THROW_CODE(PErrorCode::Interrupted);
    }

    size_t bytesRead = 0;
    if (m_PacketMode)
    {
        if (length > 0)
        {
            bufferDst[0] = TIOCPKT_DATA; // TODO: Maintain m_TermInfo->CtrlStatus and return it here.
            bufferDst++;
            bytesRead++;
        }
    }
    const size_t bytesToRead = std::min(length - bytesRead, m_BytesAvailable);

    bool needNewLine = NeedNewline();
    for (size_t i = 0; i < bytesToRead; ++i)
    {
        if (m_FileData[m_ReadPos] == 0x04) // CTRL-d
        {
            if (needNewLine)
            {
                if (i == 0)
                {
                    m_NewLineCount--;
                    m_BytesAvailable--;
                    m_ReadPos = (m_ReadPos + 1) & (m_FileData.size() - 1);
                }
                break;
            }
            else
            {
                m_NewLineCount--;
            }
        }
        if (m_FileData[m_ReadPos] == '\n') {
            m_NewLineCount--;
        }
        bufferDst[i] = m_FileData[m_ReadPos];
        m_ReadPos = (m_ReadPos + 1) & (m_FileData.size() - 1);
        bytesRead++;
    }
    m_BytesAvailable -= bytesRead;

    m_IOCondition.WakeupAll();

    return bytesRead;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPTYInode::Write(const void* buffer, size_t length, int openFlags)
{
    Ptr<KVirtualFSVolume> volume = ptr_static_cast<KVirtualFSVolume>(m_Volume);

    kassert(volume->m_Mutex.IsLocked());

    size_t  bytesWritten = 0;

    if (m_Parent == nullptr) {
        PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    bool wasReadable = CanRead();

    while (m_BytesAvailable == m_FileData.size())
    {
        if (m_Partner == nullptr || m_OpenCount == 0)
        {
            PERROR_THROW_CODE(PErrorCode::IOError);
        }
        if (openFlags & O_NONBLOCK) {
            PERROR_THROW_CODE(PErrorCode::WouldBlock);
        }
        m_IOCondition.Wait(volume->m_Mutex);
    }

    const uint8_t* bufferSrc = static_cast<const uint8_t*>(buffer);

    while (bytesWritten < length && m_BytesAvailable < m_FileData.size())
    {
        switch (bufferSrc[bytesWritten])
        {
            case 0x04:  /* CTRL-d */
            case '\n':
                m_NewLineCount++;
                break;
        }
        m_FileData[m_WritePos++] = bufferSrc[bytesWritten++];
        m_WritePos &= m_FileData.size() - 1;
        m_BytesAvailable++;
    }
    const bool isReadable = CanRead();
    if (!wasReadable && isReadable)
    {
        wasReadable = true;
        m_IOCondition.WakeupAll();
    }

    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPTYInode::WriteToMaster(const void* buffer, size_t length, int openFlags, bool echoMode)
{
    const struct termios* termios = &m_TermInfo->Termios;
    bool    nlToCrNl    = false;
    bool    crToNl      = false;
    bool    noCrAtCol0  = false;    /* Don't output CR at col 0 */
    bool    noCr        = false;    /* Don't output CR */
    size_t  bytesWritten = 0;

    if (termios->c_oflag & OPOST)
    {
        if (termios->c_oflag & ONLCR) {
            nlToCrNl = true;
        }
        if (termios->c_oflag & OCRNL) {
            crToNl = true;
        }
        if (termios->c_oflag & ONOCR) {
            noCrAtCol0 = true;
        }
        if (termios->c_oflag & ONLRET) {
            noCr = true;
        }
    }

    const uint8_t* bufferSrc = static_cast<const uint8_t*>(buffer);

    try
    {
        for (size_t i = 0; i < length; ++i)
        {
            size_t result = 1;
            switch (bufferSrc[i])
            {
                case '\n':
                    if (nlToCrNl) {
                        result = Write("\r\n", 2, openFlags);
                    } else {
                        result = Write("\n", 1, openFlags);
                    }
                    break;
                case '\r':
                    if (!noCr && !(noCrAtCol0 && m_CursorPos == 0)) {
                        result = Write(crToNl ? "\n" : "\r", 1, openFlags);
                    }
                    break;
                case 0x08:  /* BACKSPACE */
                    if (echoMode && (termios->c_lflag & ICANON)) {
                        result = Write("\x08 \x08", 3, openFlags);
                    } else {
                        result = Write(&bufferSrc[i], 1, openFlags);
                    }
                    break;
                case 0x03: // CTRL-c
                    if (echoMode) {
                        result = Write(&bufferSrc[i], 1, openFlags);
                    }
                    break;
                case 26:    /* CTRL-z */
                    if (echoMode) {
                        result = Write(&bufferSrc[i], 1, openFlags);
                    }
                    break;
                default:
                    result = Write(&bufferSrc[i], 1, openFlags);
                    break;
            }
            if (result > 0) {
                bytesWritten++;
            } else {
                break;
            }
        }
    }
    catch (const std::system_error& exc)
    {
        if (bytesWritten > 0) {
            return bytesWritten;
        } else {
            throw;
        }
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t KPTYInode::WriteToSlave(const void* buffer, size_t length, int openFlags)
{
    const struct termios* termios = &m_TermInfo->Termios;
    bool    nlToCr      = false;
    bool    ignoreCr    = false;
    bool    crToNl      = false;
    bool    doEcho      = false;

    if (termios->c_iflag & INLCR) {
        nlToCr = true;
    }
    if (termios->c_iflag & IGNCR) {
        ignoreCr = true;
    }
    if (termios->c_iflag & ICRNL) {
        crToNl = true;
    }
    if (termios->c_lflag & ECHO) {
        doEcho = true;
    }

    size_t bytesWritten = 0;
    try
    {
        const uint8_t* bufferSrc = static_cast<const uint8_t*>(buffer);

        for (size_t i = 0; i < length; ++i)
        {
            size_t result = 1;
            if (termios->c_lflag & ISIG)
            {
                if (bufferSrc[i] == termios->c_cc[VINTR])
                {
                    if (m_TermInfo->PGroupID != -1) {
                        kkill(-m_TermInfo->PGroupID, SIGINT);
                    }
                    continue;
                }
                else if (bufferSrc[i] == termios->c_cc[VSUSP])
                {
                    if (m_TermInfo->PGroupID != -1) {
                        kkill(-m_TermInfo->PGroupID, SIGSTOP);
                    }
                    continue;
                }
            }
            switch (bufferSrc[i])
            {
                case '\n':
                    result = Write(nlToCr ? "\r" : "\n", 1, openFlags);
                    if (result > 0 && !doEcho && (termios->c_lflag & ECHONL)) {
                        m_Partner->WriteToMaster("\n", 1, O_NONBLOCK, true);
                    }
                    break;
                case '\r':
                    if (!ignoreCr) {
                        result = Write(crToNl ? "\n" : "\r", 1, openFlags);
                    }
                    break;
                case 0x08:  /* BACKSPACE */
                    if (termios->c_lflag & ICANON)
                    {
                        const size_t newPos = (m_WritePos + m_FileData.size() - 1) & (m_FileData.size() - 1);

                        if (m_BytesAvailable > 0 && m_FileData[newPos] != '\n')
                        {
                            m_BytesAvailable--;
                            m_WritePos = newPos;
                        }
                        m_IOCondition.WakeupAll();
                    }
                    else
                    {
                        result = Write(&bufferSrc[i], 1, openFlags);
                    }
                    break;
                default:
                {
                    result = Write(&bufferSrc[i], 1, openFlags);
                    break;
                }
            }
            // FIX ME : Don't echo ignored backspace and CR characters.
            if (doEcho) {
                m_Partner->WriteToMaster(&bufferSrc[i], 1, O_NONBLOCK, true);
            }
            if (result > 0) {
                bytesWritten++;
            } else {
                break;
            }
        }
    }
    catch (const std::system_error& exc)
    {
        if (bytesWritten > 0) {
            return bytesWritten;
        } else {
            throw;
        }
    }
    return bytesWritten;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kdisassociate_controlling_tty_trw(bool sendSIGCONT)
{
    KProcess& process = kget_current_process();
    const KIOContext& ioContext = kget_io_context(KLocateFlag::None);

    std::set<Ptr<KInode>> ttySet; // Keeps inodes alive until the process lock is released.

    pid_t ttyPGroup = -1;

    {
        KProcessLock processLock;

        Ptr<KPTYInode> inode = ptr_dynamic_cast<KPTYInode>(ioContext.GetControllingTTY());

        if (inode == nullptr) {
            return;
        }

        ttyPGroup = inode->m_TermInfo->PGroupID;

        inode->m_TermInfo->SessionID = 0;
        inode->m_TermInfo->PGroupID = -1;

        const int session = process.GetSession();

        for (const auto& procNode : KProcess::GetProcessMap())
        {
            KProcess& curProcess = *procNode.second;
            KIOContext& ioContext = curProcess.GetIOContext();
            if (curProcess.GetSession() == session)
            {
                ttySet.insert(ioContext.GetControllingTTY());
                ioContext.SetControllingTTY(nullptr);
            }
        }
    }
    if (ttyPGroup > 0)
    {
        kkillpg(ttyPGroup, SIGHUP);
        if (sendSIGCONT) {
            kkillpg(ttyPGroup, SIGCONT);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kis_file_tty_trw(int fd)
{
    stat_t statBuf;
    kread_stat_trw(fd, &statBuf);

    if (statBuf.st_dev != KPTYFilesystem::s_VolumeID) {
        PERROR_THROW_CODE(PErrorCode::NOTTY);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ktcsetattr_trw(int fd, int optionalActions, const struct termios* termios)
{
    int request;

    kis_file_tty_trw(fd);

    switch (optionalActions)
    {
        case TCSANOW:   request = TCSETA; break;
        case TCSADRAIN: request = TCSETAW; break;
        case TCSAFLUSH: request = TCSETAF; break;
        default: PERROR_THROW_CODE(PErrorCode::InvalidArg);
    }
    kdevice_control_trw(fd, request, termios, sizeof(*termios), nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ktcgetattr_trw(int fd, struct termios* termios)
{
    kis_file_tty_trw(fd);
    kdevice_control_trw(fd, TCGETA, nullptr, 0, termios, sizeof(*termios));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ktcsetpgrp_trw(int fd, pid_t pgroup)
{
    kis_file_tty_trw(fd);
    kdevice_control_trw(fd, TIOCSPGRP, &pgroup, sizeof(pgroup), nullptr, 0);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

pid_t ktcgetpgrp_trw(int fd)
{
    pid_t pgroup;
    kis_file_tty_trw(fd);
    kdevice_control_trw(fd, TIOCGPGRP, nullptr, 0, &pgroup, sizeof(pgroup));
    return pgroup;
}

} // namespace kernel
