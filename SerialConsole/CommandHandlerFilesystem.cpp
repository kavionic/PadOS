// This file is part of PadOS.
//
// Copyright (C) 2021-2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 18.04.2021 23:30

#include <memory>

#include <stdio.h>
#include <dirent.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
//#include <sys/statvfs.h>

#include <Storage/FSNode.h>
#include <Utils/HashCalculator.h>
#include <Kernel/VFS/FileIO.h>
#include <SerialConsole/SerialCommandHandler.h>
#include <SerialConsole/CommandHandlerFilesystem.h>
#include <SerialConsole/FilesystemMessages.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::Setup(SerialCommandHandler* commandHandler)
{
    m_CommandHandler = commandHandler;

    commandHandler->RegisterPacketHandler<SerialProtocol::OpenSession>(this, &CommandHandlerFilesystem::HandleOpenSession);
    commandHandler->RegisterPacketHandler<SerialProtocol::CloseSession>(this, &CommandHandlerFilesystem::HandleCloseSession);
    commandHandler->RegisterPacketHandler<SerialProtocol::RenameFile>(this, &CommandHandlerFilesystem::HandleRenameFile);
    commandHandler->RegisterPacketHandler<SerialProtocol::GetVolumeInfo>(this, &CommandHandlerFilesystem::HandleGetVolumeInfo);
    commandHandler->RegisterPacketHandler<SerialProtocol::GetDirectory>(this, &CommandHandlerFilesystem::HandleGetDirectory);
    commandHandler->RegisterPacketHandler<SerialProtocol::CreateFile>(this, &CommandHandlerFilesystem::HandleCreateFile);
    commandHandler->RegisterPacketHandler<SerialProtocol::CreateDirectory>(this, &CommandHandlerFilesystem::HandleCreateDirectory);
    commandHandler->RegisterPacketHandler<SerialProtocol::OpenFile>(this, &CommandHandlerFilesystem::HandleOpenFile);
    commandHandler->RegisterPacketHandler<SerialProtocol::WriteFile>(this, &CommandHandlerFilesystem::HandleWriteFile);
    commandHandler->RegisterPacketHandler<SerialProtocol::ReadFile>(this, &CommandHandlerFilesystem::HandleReadFile);
    commandHandler->RegisterPacketHandler<SerialProtocol::CloseFile>(this, &CommandHandlerFilesystem::HandleCloseFile);
    commandHandler->RegisterPacketHandler<SerialProtocol::DeleteFile>(this, &CommandHandlerFilesystem::HandleDeleteFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CommandHandlerFilesystem::ValidateSession(int32_t sessionID)
{
    const bool isValid = m_Sessions.find(sessionID) != m_Sessions.end();
    
    if (!isValid)
    {
        p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {} failed.", __PRETTY_FUNCTION__, sessionID);
        m_CommandHandler->SendMessage<SerialProtocol::FilesystemStatusReply>(sessionID, SerialProtocol::FilesystemError::UnknownSession);
    }

    return isValid;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleOpenSession(const SerialProtocol::OpenSession& msg)
{
    static int32_t nextSessionID = 1;

    int32_t sessionID = nextSessionID++;
    m_Sessions[sessionID] = SessionData();

    p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}.", __PRETTY_FUNCTION__, sessionID);

    m_CommandHandler->SendMessage<SerialProtocol::OpenSessionReply>(sessionID, msg.m_ClientToken);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCloseSession(const SerialProtocol::CloseSession& msg)
{
    auto sessionIterator = m_Sessions.find(msg.m_SessionID);

    p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {} ({}).", __PRETTY_FUNCTION__, msg.m_SessionID, (sessionIterator != m_Sessions.end()) ? "valid" : "invalid");

    if (sessionIterator == m_Sessions.end())
    {
        m_CommandHandler->SendMessage<SerialProtocol::FilesystemStatusReply>(msg.m_SessionID, SerialProtocol::FilesystemError::UnknownSession);
        return;
    }
    for (int fileEntry : sessionIterator->second.m_OpenFiles) {
        close(fileEntry);
    }
    m_Sessions.erase(sessionIterator);
    m_CommandHandler->SendMessage<SerialProtocol::FilesystemStatusReply>(msg.m_SessionID, SerialProtocol::FilesystemError::OK);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleRenameFile(const SerialProtocol::RenameFile& msg)
{
    p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, old_path: '{}', new_path: '{}'.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_OldPath, msg.m_NewPath);

    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    const int result = krename(KLocateFlag::None, msg.m_OldPath, msg.m_NewPath);
    m_CommandHandler->SendMessage<SerialProtocol::FilesystemStatusReply>(
        msg.m_SessionID,
        (result == 0) ? SerialProtocol::FilesystemError::OK : SerialProtocol::FilesystemError::IOError
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleGetVolumeInfo(const SerialProtocol::GetVolumeInfo& msg)
{
    p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}.", __PRETTY_FUNCTION__, msg.m_SessionID);

    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    
//    struct statvfs fsStats;
//    if (statvfs("/", &fsStats) == 0)
//    {
//        const int64_t totalBytes = int64_t(fsStats.f_blocks) * int64_t(fsStats.f_frsize);
//        const int64_t freeBytes  = int64_t(fsStats.f_bfree)  * int64_t(fsStats.f_frsize);
//        m_CommandHandler->SendMessage<SerialProtocol::GetVolumeInfoReply>(SerialProtocol::FilesystemError::OK, totalBytes, freeBytes);
//    }
//    else
//    {
//        m_CommandHandler->SendMessage<SerialProtocol::GetVolumeInfoReply>(SerialProtocol::FilesystemError::IOError, 0LL, 0LL);
//    }
    m_CommandHandler->SendMessage<SerialProtocol::GetVolumeInfoReply>(msg.m_SessionID, SerialProtocol::FilesystemError::OK, 1024LL * 1024LL * 1024LL * 8LL, 1024LL * 1024LL * 1024LL * 4LL);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleGetDirectory(const SerialProtocol::GetDirectory& packet)
{
    p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, path: '{}'.", __PRETTY_FUNCTION__, packet.m_SessionID, packet.m_Path);

    if (!ValidateSession(packet.m_SessionID)) {
        return;
    }
    std::vector<SerialProtocol::GetDirectoryReplyDirEnt> entryList;
    int dir = open(packet.m_Path, O_RDONLY);
    if (dir >= 0)
    {
        constexpr size_t maxEntriesPerPackage = (4096 - sizeof(SerialProtocol::GetDirectoryReply)) / sizeof(SerialProtocol::GetDirectoryReplyDirEnt);
        static_assert(maxEntriesPerPackage >= 5);
        dirent_t dirEntry;
        for (int i = 0; posix_getdents(dir, &dirEntry, sizeof(dirEntry), 0) == sizeof(dirEntry); ++i)
        {
            PString path = packet.m_Path;
            path += "/";
            path += dirEntry.d_name;
            struct stat statResult;
            if (stat(path.c_str(), &statResult) < 0) {
                continue;
            }
            if (dirEntry.d_namlen >= sizeof(SerialProtocol::GetDirectoryReplyDirEnt::m_Name)) {
                continue;
            }

            SerialProtocol::GetDirectoryReplyDirEnt& replyEntry = entryList.emplace_back();
            replyEntry.m_Size        = statResult.st_size;
            replyEntry.m_ModTime     = statResult.st_mtim.tv_sec;
            replyEntry.m_Attributes  = statResult.st_mode;
            replyEntry.m_IsDirectory = dirEntry.d_type == DT_DIR;
            memcpy(replyEntry.m_Name, dirEntry.d_name, dirEntry.d_namlen);
            replyEntry.m_Name[dirEntry.d_namlen] = '\0';

            char timeStr[64];
            std::strftime(timeStr, sizeof(timeStr), "%b %d %Y", std::localtime(&statResult.st_mtime));
            p_system_log<PLogSeverity::INFO_FLOODING>(LogCategorySerialHandlerFS, "    {} {} {:>8} '{}'.",
                PString::format_file_permissions(replyEntry.m_Attributes),
                timeStr,
                PString::format_byte_size(statResult.st_size, -2),
                replyEntry.m_Name
            );

            if (entryList.size() >= maxEntriesPerPackage)
            {
                SendDirectoryEntries(packet.m_SessionID, entryList);
                entryList.erase(entryList.begin(), entryList.end());
            }
        }
        if (!entryList.empty()) {
            SendDirectoryEntries(packet.m_SessionID, entryList);
        }
        m_CommandHandler->SendMessage<SerialProtocol::GetDirectoryReply>(packet.m_SessionID, 0);
    }
    else
    {
        p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandlerFS, "Failed to open directory '{}'.", packet.m_Path);
        m_CommandHandler->SendMessage<SerialProtocol::GetDirectoryReply>(packet.m_SessionID, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCreateFile(const SerialProtocol::CreateFile& msg)
{
    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    SessionData& session = m_Sessions[msg.m_SessionID];
    int fd = open(msg.m_Path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    if (fd >= 0)
    {
        session.m_OpenFiles.insert(fd);
        p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, path: '{}', file: {}.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_Path, fd);
        m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(msg.m_SessionID, fd);
    }
    else
    {
        p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, path: '{}', failed.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_Path);
        m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(msg.m_SessionID, -1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCreateDirectory(const SerialProtocol::CreateDirectory& msg)
{
    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    const int result = mkdir(msg.m_Path, 0777);

    p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {} path: '{}', result: {}.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_Path, result);
    
    m_CommandHandler->SendMessage<SerialProtocol::CreateDirectoryReply>(
        msg.m_SessionID,
        (result == 0) ? SerialProtocol::FilesystemError::OK : SerialProtocol::FilesystemError::IOError
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleOpenFile(const SerialProtocol::OpenFile& msg)
{
    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    SessionData& session = m_Sessions[msg.m_SessionID];
    int fd = open(msg.m_Path, O_RDONLY);
    if (fd >= 0)
    {
        session.m_OpenFiles.insert(fd);

        p_system_log<PLogSeverity::INFO_HIGH_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, path: '{}', file: {}.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_Path, fd);

        m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(msg.m_SessionID, fd);
    }
    else
    {
        p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandlerFS, "{}: ses: {} path: '{}', failed.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_Path);

        m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(msg.m_SessionID, -1);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleWriteFile(const SerialProtocol::WriteFile& msg)
{
    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    const SessionData& session = m_Sessions[msg.m_SessionID];
    if (!session.m_OpenFiles.contains(msg.m_File))
    {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandlerFS, "{}: ses: {}, file: {}, offset: {}, size: {} invalid file.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_File, msg.m_StartPos, msg.m_Size);
        m_CommandHandler->SendMessage<SerialProtocol::WriteFileReply>(msg.m_SessionID, msg.m_File, -1);
        return;
    }
    const ssize_t result = pwrite(msg.m_File, msg.m_Buffer, msg.m_Size, msg.m_StartPos);

    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, file: {}, offset: {}, size: {}, result: {}.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_File, msg.m_StartPos, msg.m_Size, result);

    m_CommandHandler->SendMessage<SerialProtocol::WriteFileReply>(msg.m_SessionID, msg.m_File, (result == msg.m_Size) ? (msg.m_StartPos + result) : -1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleReadFile(const SerialProtocol::ReadFile& msg)
{
    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    const SessionData& session = m_Sessions[msg.m_SessionID];
    
    if (!session.m_OpenFiles.contains(msg.m_File))
    {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandlerFS, "{}: ses: {}, file: {}, offset: {}, size: {} invalid file.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_File, msg.m_StartPos, msg.m_Size);

        std::unique_ptr<SerialProtocol::ReadFileReply> reply = std::make_unique<SerialProtocol::ReadFileReply>();
        SerialProtocol::ReadFileReply::InitMsg(*reply, msg.m_SessionID, msg.m_File, msg.m_StartPos, -1);
        m_CommandHandler->SendSerialPacket(reply.get());
        return;
    }

    size_t size = msg.m_Size;

    if (size > sizeof(SerialProtocol::ReadFileReply::m_Buffer))
    {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandlerFS, "{}: Request too large: {} > {}.", __PRETTY_FUNCTION__, size, sizeof(SerialProtocol::ReadFileReply::m_Buffer));
        size = sizeof(SerialProtocol::ReadFileReply::m_Buffer);
    }

    std::unique_ptr<SerialProtocol::ReadFileReply> reply = std::make_unique<SerialProtocol::ReadFileReply>();

    ssize_t result = pread(msg.m_File, reply->m_Buffer, size, msg.m_StartPos);

    if (result < 0) {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandlerFS, "{}: Failed to read: {}.", __PRETTY_FUNCTION__, strerror(errno));
    } else if (result > ssize_t(size)) {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandlerFS, "{}: Read more bytes than requested: {}/{}.", __PRETTY_FUNCTION__, result, size);
    }
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, file: {}, offset: {}, size: {}, result: {}.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_File, msg.m_StartPos, msg.m_Size, result);
    SerialProtocol::ReadFileReply::InitMsg(*reply, msg.m_SessionID, msg.m_File, msg.m_StartPos, result);
    m_CommandHandler->SendSerialPacket(reply.get());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCloseFile(const SerialProtocol::CloseFile& msg)
{
    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandlerFS, "{}: ses: {}, file: {}.", __PRETTY_FUNCTION__, msg.m_SessionID, msg.m_File);
    SessionData& session = m_Sessions[msg.m_SessionID];
    auto fileIterator = session.m_OpenFiles.find(msg.m_File);
    if (fileIterator == session.m_OpenFiles.end())
    {
        m_CommandHandler->SendMessage<SerialProtocol::CloseFileReply>(msg.m_SessionID, SerialProtocol::FilesystemError::NotFound);
        return;
    }
    close(msg.m_File);
    session.m_OpenFiles.erase(fileIterator);
    m_CommandHandler->SendMessage<SerialProtocol::CloseFileReply>(msg.m_SessionID, SerialProtocol::FilesystemError::OK);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleDeleteFile(const SerialProtocol::DeleteFile& msg)
{
    if (!ValidateSession(msg.m_SessionID)) {
        return;
    }
    PFSNode file(msg.m_Path);
    if (!file.IsValid())
    {
        m_CommandHandler->SendMessage<SerialProtocol::DeleteFileReply>(msg.m_SessionID, SerialProtocol::FilesystemError::NotFound);
        return;
    }
    const bool isDirectory = file.IsDir();
    file.Close();
    int result = isDirectory ? rmdir(msg.m_Path) : unlink(msg.m_Path);
    m_CommandHandler->SendMessage<SerialProtocol::DeleteFileReply>(
        msg.m_SessionID,
        (result == 0) ? SerialProtocol::FilesystemError::OK : SerialProtocol::FilesystemError::IOError
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CommandHandlerFilesystem::SendDirectoryEntries(int32_t sessionID, const std::vector<SerialProtocol::GetDirectoryReplyDirEnt>& entryList)
{
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandlerFS, "Returning {} entries.", entryList.size());
    SerialProtocol::GetDirectoryReply msg;
    SerialProtocol::GetDirectoryReply::InitMsg(msg, sessionID, entryList.size());
    msg.Command       = SerialProtocol::GetDirectoryReply::COMMAND;
    msg.Magic         = SerialProtocol::PacketHeader::MAGIC;
    msg.PackageLength = sizeof(msg) + entryList.size() * sizeof(SerialProtocol::GetDirectoryReplyDirEnt);

    return m_CommandHandler->SendSerialData(&msg, sizeof(msg), entryList.data(), entryList.size() * sizeof(SerialProtocol::GetDirectoryReplyDirEnt));
}

} // namespace kernel
