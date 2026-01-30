// This file is part of PadOS.
//
// Copyright (C) 2021-2022 Kurt Skauen <http://kavionic.com/>
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

#include <stdio.h>
#include <dirent.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include <Storage/FSNode.h>
#include <Utils/HashCalculator.h>
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

void CommandHandlerFilesystem::HandleGetDirectory(const SerialProtocol::GetDirectory& packet)
{
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandler, "Directory '{}' requested.", packet.m_Path);
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
            replyEntry.m_Size = statResult.st_size;
            replyEntry.m_ModTime = statResult.st_mtim.tv_sec;
            replyEntry.m_IsDirectory = dirEntry.d_type == DT_DIR;
            memcpy(replyEntry.m_Name, dirEntry.d_name, dirEntry.d_namlen);
            replyEntry.m_Name[dirEntry.d_namlen] = '\0';

            if (entryList.size() >= maxEntriesPerPackage)
            {
                SendDirectoryEntries(entryList);
                entryList.erase(entryList.begin(), entryList.end());
            }
        }
        if (!entryList.empty()) {
            SendDirectoryEntries(entryList);
        }
    }
    else
    {
        p_system_log<PLogSeverity::WARNING>(LogCategorySerialHandler, "Failed to open directory '{}'.", packet.m_Path);
        m_CommandHandler->SendMessage<SerialProtocol::GetDirectoryReply>(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCreateFile(const SerialProtocol::CreateFile& msg)
{
    if (m_CurrentExternalFile != -1) {
        close(m_CurrentExternalFile);
    }
    m_CurrentExternalFile = open(msg.m_Path, O_WRONLY | O_CREAT | O_TRUNC, 0777);
    m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(m_CurrentExternalFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCreateDirectory(const SerialProtocol::CreateDirectory& msg)
{
    mkdir(msg.m_Path, 0777);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleOpenFile(const SerialProtocol::OpenFile& msg)
{
    if (m_CurrentExternalFile != -1) {
        close(m_CurrentExternalFile);
    }
    m_CurrentExternalFile = open(msg.m_Path, O_RDONLY);
    m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(m_CurrentExternalFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleWriteFile(const SerialProtocol::WriteFile& msg)
{
    if (m_CurrentExternalFile == -1 || msg.m_File != m_CurrentExternalFile) {
        return;
    }
    ssize_t result = pwrite(m_CurrentExternalFile, msg.m_Buffer, msg.m_Size, msg.m_StartPos);
    m_CommandHandler->SendMessage<SerialProtocol::WriteFileReply>(m_CurrentExternalFile, (result == msg.m_Size) ? (msg.m_StartPos + result) : -1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleReadFile(const SerialProtocol::ReadFile& msg)
{
    if (m_CurrentExternalFile == -1 || msg.m_File != m_CurrentExternalFile) {
        return;
    }
    size_t size = msg.m_Size;
    if (size > sizeof(SerialProtocol::ReadFileReply::m_Buffer))
    {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandler, "{}: Request too large: {} > {}.", __PRETTY_FUNCTION__, size, sizeof(SerialProtocol::ReadFileReply::m_Buffer));
        size = sizeof(SerialProtocol::ReadFileReply::m_Buffer);
    }
    std::unique_ptr<SerialProtocol::ReadFileReply> reply = std::make_unique<SerialProtocol::ReadFileReply>();
    ssize_t result = pread(m_CurrentExternalFile, reply->m_Buffer, size, msg.m_StartPos);
    if (result < 0) {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandler, "{}: Failed to read: {}.", __PRETTY_FUNCTION__, strerror(errno));
    } else if (result > size) {
        p_system_log<PLogSeverity::ERROR>(LogCategorySerialHandler, "{}: Read more bytes than requested: {}/{}.", __PRETTY_FUNCTION__, result, size);
    }
    SerialProtocol::ReadFileReply::InitMsg(*reply, m_CurrentExternalFile, msg.m_StartPos, result);

    m_CommandHandler->SendSerialPacket(reply.get());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCloseFile(const SerialProtocol::CloseFile& msg)
{
    if (m_CurrentExternalFile == -1 || msg.m_File != m_CurrentExternalFile) {
        return;
    }
    close(m_CurrentExternalFile);
    m_CurrentExternalFile = -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleDeleteFile(const SerialProtocol::DeleteFile& msg)
{
    if (m_CurrentExternalFile != -1) {
        close(m_CurrentExternalFile);
    }

    PFSNode file(msg.m_Path);
    if (file.IsValid())
    {
        bool isDirectory = file.IsDir();
        file.Close();
        if (isDirectory) {
            __remove_directory(AT_FDCWD, msg.m_Path);
        }
        else {
            unlink(msg.m_Path);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool CommandHandlerFilesystem::SendDirectoryEntries(const std::vector<SerialProtocol::GetDirectoryReplyDirEnt>& entryList)
{
    p_system_log<PLogSeverity::INFO_LOW_VOL>(LogCategorySerialHandler, "Returning {} entries.", entryList.size());
    SerialProtocol::GetDirectoryReply msg;
    SerialProtocol::GetDirectoryReply::InitMsg(msg, entryList.size());
    msg.Command = SerialProtocol::GetDirectoryReply::COMMAND;
    msg.Magic = SerialProtocol::PacketHeader::MAGIC;
    msg.PackageLength = sizeof(msg) + entryList.size() * sizeof(SerialProtocol::GetDirectoryReplyDirEnt);

    return m_CommandHandler->SendSerialData(&msg, sizeof(msg), entryList.data(), entryList.size() * sizeof(SerialProtocol::GetDirectoryReplyDirEnt));
}

} // namespace kernel
