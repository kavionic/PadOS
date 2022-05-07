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
// Created: 18.04.2021 20:15

#pragma once

#include <string.h>
#include <SerialConsole/SerialProtocol.h>

namespace SerialProtocol
{

struct GetDirectory : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::GetDirectory;
    static void InitMsg(GetDirectory& msg, const char* path, int pathLength)
    {
        InitHeader(msg);
        if (pathLength >= sizeof(msg.m_Path)) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }

    char    m_Path[1024];
};

struct GetDirectoryReplyDirEnt
{
    int32_t m_Size;
    int64_t m_ModTime;
    bool    m_IsDirectory;
    char    m_Name[256 + 7];
};


struct GetDirectoryReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::GetDirectoryReply;
    static void InitMsg(GetDirectoryReply& msg, int32_t entryCount) { InitHeader(msg); msg.m_EntryCount = entryCount; }

    int32_t m_EntryCount;
};

struct CreateFile : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::CreateFile;
    static void InitMsg(CreateFile& msg, const char* path, int pathLength)
    {
        InitHeader(msg);
        if (pathLength >= sizeof(msg.m_Path)) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }
    char    m_Path[1024];
};

struct CreateDirectory : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::CreateDirectory;
    static void InitMsg(CreateDirectory& msg, const char* path, int pathLength)
    {
        InitHeader(msg);
        if (pathLength >= sizeof(msg.m_Path)) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }
    char    m_Path[1024];
};

struct OpenFile : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::OpenFile;
    static void InitMsg(OpenFile& msg, const char* path, int pathLength)
    {
        InitHeader(msg);
        if (pathLength >= sizeof(msg.m_Path)) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }
    char    m_Path[1024];
};

struct CloseFile : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::CloseFile;
    static void InitMsg(CloseFile& msg, int32_t file) { InitHeader(msg); msg.m_File = file; }
    int32_t m_File;
};

struct OpenFileReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::OpenFileReply;
    static void InitMsg(OpenFileReply& msg, int32_t file) { InitHeader(msg); msg.m_File = file; }

    int32_t m_File;
};

struct WriteFile : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::WriteFile;
    static void InitMsg(WriteFile& msg, int32_t file, const void* data, int32_t startPos, int32_t size)
    {
        InitHeader(msg);
        assert(size <= sizeof(m_Buffer));
        memcpy(msg.m_Buffer, data, size);

        msg.m_File = file;
        msg.m_Size = size;
        msg.m_StartPos = startPos;
    }
    int32_t m_File;
    int32_t m_Size;
    int32_t m_StartPos;
    char    m_Buffer[1024];
};

struct WriteFileReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::WriteFileReply;
    static void InitMsg(WriteFileReply& msg, int32_t file, int32_t bytesWritten) { InitHeader(msg); msg.m_File = file; msg.m_BytesWritten = bytesWritten; }

    int32_t m_BytesWritten;
    int32_t m_File;

};


struct ReadFile : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::ReadFile;
    static void InitMsg(ReadFile& msg, int32_t file, int32_t startPos, int32_t size)
    {
        InitHeader(msg);
        msg.m_File = file;
        msg.m_Size = size;
        msg.m_StartPos = startPos;
    }
    int32_t m_File;
    int32_t m_Size;
    int32_t m_StartPos;
};

struct ReadFileReply : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::ReadFileReply;
    static void InitMsg(ReadFileReply& msg, int32_t file, int32_t startPos, int32_t size)
    {
        InitHeader(msg);
        assert(size <= sizeof(m_Buffer));

        msg.m_File = file;
        msg.m_Size = size;
        msg.m_StartPos = startPos;
    }
    int32_t m_File;
    int32_t m_Size;
    int32_t m_StartPos;
    char    m_Buffer[1024];
};

struct DeleteFile : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::DeleteFile;
    static void InitMsg(DeleteFile& msg, const char* path, int pathLength)
    {
        InitHeader(msg);
        if (pathLength >= sizeof(msg.m_Path)) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }
    char    m_Path[1024];
};

} //namespace SerialProtocol
