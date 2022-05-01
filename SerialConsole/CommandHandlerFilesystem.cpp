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
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include <Kernel/VFS/FileIO.h>
#include <Storage/FSNode.h>
#include <Utils/HashCalculator.h>
#include <SerialConsole/SerialCommandHandler.h>
#include <SerialConsole/CommandHandlerFilesystem.h>
#include <SerialConsole/FilesystemMessages.h>

using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::Setup(SerialCommandHandler* commandHandler)
{
    m_CommandHandler = commandHandler;

    commandHandler->RegisterPacketHandler<SerialProtocol::GetDirectory>(this, &CommandHandlerFilesystem::HandleGetDirectory, false);
    commandHandler->RegisterPacketHandler<SerialProtocol::CreateFile>(this, &CommandHandlerFilesystem::HandleCreateFile, false);
    commandHandler->RegisterPacketHandler<SerialProtocol::CreateDirectory>(this, &CommandHandlerFilesystem::HandleCreateDirectory);
    commandHandler->RegisterPacketHandler<SerialProtocol::OpenFile>(this, &CommandHandlerFilesystem::HandleOpenFile, false);
    commandHandler->RegisterPacketHandler<SerialProtocol::WriteFile>(this, &CommandHandlerFilesystem::HandleWriteFile, false);
    commandHandler->RegisterPacketHandler<SerialProtocol::ReadFile>(this, &CommandHandlerFilesystem::HandleReadFile, false);
    commandHandler->RegisterPacketHandler<SerialProtocol::CloseFile>(this, &CommandHandlerFilesystem::HandleCloseFile);
    commandHandler->RegisterPacketHandler<SerialProtocol::DeleteFile>(this, &CommandHandlerFilesystem::HandleDeleteFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleGetDirectory(const SerialProtocol::GetDirectory& packet)
{
    kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Directory '%s' requested\n", packet.m_Path);
    std::vector<SerialProtocol::GetDirectoryReplyDirEnt> entryList;
    int dir = FileIO::Open(packet.m_Path, O_RDONLY);
    if (dir >= 0)
    {
        kernel::dir_entry dirEntry;
        for (int i = 0; FileIO::ReadDirectory(dir, &dirEntry, sizeof(dirEntry)) == 1; ++i)
        {
            String path = packet.m_Path;
            path += "/";
            path += dirEntry.d_name;
            struct stat statResult;
            if (stat(path.c_str(), &statResult) < 0) {
                continue;
            }
            if (dirEntry.d_namelength >= sizeof(SerialProtocol::GetDirectoryReplyDirEnt::m_Name)) {
                continue;
            }

            SerialProtocol::GetDirectoryReplyDirEnt& replyEntry = entryList.emplace_back();
            replyEntry.m_Size = statResult.st_size;
            replyEntry.m_ModTime = statResult.st_mtime;
            replyEntry.m_IsDirectory = dirEntry.d_type == kernel::dir_entry_type::DT_DIRECTORY;
            memcpy(replyEntry.m_Name, dirEntry.d_name, dirEntry.d_namelength);
            replyEntry.m_Name[dirEntry.d_namelength] = '\0';
        }
        kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::INFO_LOW_VOL, "Returning %d entries\n", entryList.size());
        SerialProtocol::GetDirectoryReply msg;
        SerialProtocol::GetDirectoryReply::InitMsg(msg, entryList.size());
        msg.Command = SerialProtocol::GetDirectoryReply::COMMAND;
        msg.Magic = SerialProtocol::PacketHeader::MAGIC;
        msg.Token = packet.Token;
        msg.PackageLength = sizeof(msg) + entryList.size() * sizeof(SerialProtocol::GetDirectoryReplyDirEnt);

        m_CommandHandler->SendSerialData(&msg, sizeof(msg), entryList.data(), entryList.size() * sizeof(SerialProtocol::GetDirectoryReplyDirEnt));
    }
    else
    {
        kernel::kernel_log(LogCategorySerialHandler, kernel::KLogSeverity::WARNING, "ERROR: failed to open directory '%s'\n", packet.m_Path);
        m_CommandHandler->SendMessage<SerialProtocol::GetDirectoryReply>(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCreateFile(const SerialProtocol::CreateFile& msg)
{
    if (m_CurrentExternalFile != -1) {
        FileIO::Close(m_CurrentExternalFile);
    }
    m_CurrentExternalFile = FileIO::Open(msg.m_Path, O_WRONLY | O_CREAT | O_TRUNC);
    m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(msg.Token, m_CurrentExternalFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCreateDirectory(const SerialProtocol::CreateDirectory& msg)
{
    FileIO::CreateDirectory(msg.m_Path);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleOpenFile(const SerialProtocol::OpenFile& msg)
{
    if (m_CurrentExternalFile != -1) {
        FileIO::Close(m_CurrentExternalFile);
    }
    m_CurrentExternalFile = FileIO::Open(msg.m_Path, O_RDONLY);
    m_CommandHandler->SendMessage<SerialProtocol::OpenFileReply>(msg.Token, m_CurrentExternalFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleWriteFile(const SerialProtocol::WriteFile& msg)
{
    if (m_CurrentExternalFile == -1 || msg.m_File != m_CurrentExternalFile) {
        return;
    }
    ssize_t result = FileIO::Write(m_CurrentExternalFile, msg.m_StartPos, msg.m_Buffer, msg.m_Size);
    m_CommandHandler->SendMessage<SerialProtocol::WriteFileReply>(msg.Token, m_CurrentExternalFile, (result == msg.m_Size) ? (msg.m_StartPos + result) : -1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleReadFile(const SerialProtocol::ReadFile& msg)
{
    if (m_CurrentExternalFile == -1 || msg.m_File != m_CurrentExternalFile) {
        return;
    }
    SerialProtocol::ReadFileReply reply;
    ssize_t result = FileIO::Read(m_CurrentExternalFile, msg.m_StartPos, reply.m_Buffer, msg.m_Size);
    SerialProtocol::ReadFileReply::InitMsg(reply, msg.Token, m_CurrentExternalFile, msg.m_StartPos, result);

    m_CommandHandler->SendSerialPacket(&reply);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleCloseFile(const SerialProtocol::CloseFile& msg)
{
    if (m_CurrentExternalFile == -1 || msg.m_File != m_CurrentExternalFile) {
        return;
    }
    FileIO::Close(m_CurrentExternalFile);
    m_CurrentExternalFile = -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void CommandHandlerFilesystem::HandleDeleteFile(const SerialProtocol::DeleteFile& msg)
{
    if (m_CurrentExternalFile != -1) {
        FileIO::Close(m_CurrentExternalFile);
    }

    FSNode file(msg.m_Path);
    if (file.IsValid())
    {
        bool isDirectory = file.IsDir();
        file.Close();
        if (isDirectory) {
            FileIO::RemoveDirectory(msg.m_Path);
        }
        else {
            FileIO::Unlink(msg.m_Path);
        }
    }
}
