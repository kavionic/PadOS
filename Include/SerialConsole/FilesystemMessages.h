// This file is part of PadOS.
//
// Copyright (C) 2021-2026 Kurt Skauen <http://kavionic.com/>
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

enum class FilesystemError : int32_t
{
    OK               =  0,
    UnknownSession   = -1,
    NotFound         = -2,
    IOError          = -3,
    PermissionDenied = -4,
};

static constexpr size_t FILESYSTEM_IOBUFFER_SIZE = 1024 * 64;

// Base for all filesystem packets that carry a session ID.
// OpenSession is the only exception — it uses m_ClientToken instead
// because no session exists yet when it is sent.
struct FilesystemSessionPacket : PacketHeader
{
    int32_t m_SessionID;
};

// Generic status reply used for: CloseSession, RenameFile.
// Also used by CreateDirectoryReply, CloseFileReply, DeleteFileReply (same layout, different COMMAND).
struct FilesystemStatusReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::FilesystemStatusReply;
    static void InitMsg(FilesystemStatusReply& msg, int32_t sessionID, FilesystemError status) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_Status = status; }

    FilesystemError m_Status;
};

struct OpenSession : PacketHeader
{
    static constexpr Commands::Value COMMAND = Commands::OpenSession;
    static void InitMsg(OpenSession& msg, uint32_t clientToken) { InitHeader(msg); msg.m_ClientToken = clientToken; }

    uint32_t m_ClientToken;
};

struct OpenSessionReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::OpenSessionReply;
    static void InitMsg(OpenSessionReply& msg, int32_t sessionID, uint32_t clientToken) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_ClientToken = clientToken; }

    uint32_t m_ClientToken;
};

struct CloseSession : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::CloseSession;
    static void InitMsg(CloseSession& msg, int32_t sessionID) { InitHeader(msg); msg.m_SessionID = sessionID; }
};

struct RenameFile : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::RenameFile;
    static void InitMsg(RenameFile& msg, int32_t sessionID, const char* oldPath, int oldPathLength, const char* newPath, int newPathLength)
    {
        InitHeader(msg);
        msg.m_SessionID = sessionID;
        if (oldPathLength >= int(sizeof(msg.m_OldPath))) oldPathLength = sizeof(msg.m_OldPath) - 1;
        memcpy(msg.m_OldPath, oldPath, oldPathLength);
        msg.m_OldPath[oldPathLength] = '\0';
        if (newPathLength >= int(sizeof(msg.m_NewPath))) newPathLength = sizeof(msg.m_NewPath) - 1;
        memcpy(msg.m_NewPath, newPath, newPathLength);
        msg.m_NewPath[newPathLength] = '\0';
    }

    char m_OldPath[1024];
    char m_NewPath[1024];
};

struct GetVolumeInfo : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::GetVolumeInfo;
    static void InitMsg(GetVolumeInfo& msg, int32_t sessionID) { InitHeader(msg); msg.m_SessionID = sessionID; }
};

struct GetVolumeInfoReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::GetVolumeInfoReply;
    static void InitMsg(GetVolumeInfoReply& msg, int32_t sessionID, FilesystemError status, int64_t totalBytes, int64_t freeBytes)
    {
        InitHeader(msg);
        msg.m_SessionID  = sessionID;
        msg.m_Status     = status;
        msg.m_TotalBytes = totalBytes;
        msg.m_FreeBytes  = freeBytes;
    }

    FilesystemError m_Status;
    int64_t         m_TotalBytes;
    int64_t         m_FreeBytes;
};

// Directory entry returned inside GetDirectoryReply packets.
// sizeof == 280 bytes (same as before m_Attributes was added; m_Name shrunk from [263] to [259]).
struct GetDirectoryReplyDirEnt
{
    int64_t  m_Size;
    int64_t  m_ModTime;
    uint32_t m_Attributes;  // POSIX mode bits from stat() st_mode
    bool     m_IsDirectory;
    char     m_Name[259];
};

struct GetDirectory : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::GetDirectory;
    static void InitMsg(GetDirectory& msg, int32_t sessionID, const char* path, int pathLength)
    {
        InitHeader(msg);
        msg.m_SessionID = sessionID;
        if (pathLength >= int(sizeof(msg.m_Path))) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }

    char m_Path[1024];
};

struct GetDirectoryReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::GetDirectoryReply;
    static void InitMsg(GetDirectoryReply& msg, int32_t sessionID, int32_t entryCount) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_EntryCount = entryCount; }

    int32_t m_EntryCount;
};

struct CreateFile : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::CreateFile;
    static void InitMsg(CreateFile& msg, int32_t sessionID, const char* path, int pathLength)
    {
        InitHeader(msg);
        msg.m_SessionID = sessionID;
        if (pathLength >= int(sizeof(msg.m_Path))) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }

    char m_Path[1024];
};

struct CreateDirectory : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::CreateDirectory;
    static void InitMsg(CreateDirectory& msg, int32_t sessionID, const char* path, int pathLength)
    {
        InitHeader(msg);
        msg.m_SessionID = sessionID;
        if (pathLength >= int(sizeof(msg.m_Path))) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }

    char m_Path[1024];
};

struct CreateDirectoryReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::CreateDirectoryReply;
    static void InitMsg(CreateDirectoryReply& msg, int32_t sessionID, FilesystemError status) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_Status = status; }

    FilesystemError m_Status;
};

struct OpenFile : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::OpenFile;
    static void InitMsg(OpenFile& msg, int32_t sessionID, const char* path, int pathLength)
    {
        InitHeader(msg);
        msg.m_SessionID = sessionID;
        if (pathLength >= int(sizeof(msg.m_Path))) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }

    char m_Path[1024];
};

struct CloseFile : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::CloseFile;
    static void InitMsg(CloseFile& msg, int32_t sessionID, int32_t file) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_File = file; }

    int32_t m_File;
};

struct CloseFileReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::CloseFileReply;
    static void InitMsg(CloseFileReply& msg, int32_t sessionID, FilesystemError status) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_Status = status; }

    FilesystemError m_Status;
};

struct OpenFileReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::OpenFileReply;
    static void InitMsg(OpenFileReply& msg, int32_t sessionID, int32_t file) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_File = file; }

    int32_t m_File;
};

struct WriteFile : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::WriteFile;
    static void InitMsg(WriteFile& msg, int32_t sessionID, int32_t file, const void* data, int64_t startPos, int32_t size)
    {
        InitHeader(msg);
        assert(size <= int32_t(sizeof(m_Buffer)));
        memcpy(msg.m_Buffer, data, size);
        msg.m_SessionID = sessionID;
        msg.m_File      = file;
        msg.m_Size      = size;
        msg.m_StartPos  = startPos;
    }

    int32_t m_File;
    int32_t m_Size;
    int64_t m_StartPos;
    char    m_Buffer[FILESYSTEM_IOBUFFER_SIZE];
};

struct WriteFileReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::WriteFileReply;
    static void InitMsg(WriteFileReply& msg, int32_t sessionID, int32_t file, int64_t bytesWritten) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_File = file; msg.m_BytesWritten = bytesWritten; }

    int32_t m_File;
    int64_t m_BytesWritten;
};

struct ReadFile : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::ReadFile;
    static void InitMsg(ReadFile& msg, int32_t sessionID, int32_t file, int64_t startPos, int32_t size)
    {
        InitHeader(msg);
        msg.m_SessionID = sessionID;
        msg.m_File      = file;
        msg.m_Size      = size;
        msg.m_StartPos  = startPos;
    }

    int32_t m_File;
    int32_t m_Size;
    int64_t m_StartPos;
};

struct ReadFileReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::ReadFileReply;
    static void InitMsg(ReadFileReply& msg, int32_t sessionID, int32_t file, int64_t startPos, int32_t size)
    {
        InitHeader(msg);
        assert(size <= int32_t(sizeof(m_Buffer)));
        msg.m_SessionID = sessionID;
        msg.m_File     = file;
        msg.m_Size     = size;
        msg.m_StartPos = startPos;
    }

    int32_t m_File;
    int32_t m_Size;
    int64_t m_StartPos;
    char    m_Buffer[FILESYSTEM_IOBUFFER_SIZE];
};

struct DeleteFile : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::DeleteFile;
    static void InitMsg(DeleteFile& msg, int32_t sessionID, const char* path, int pathLength)
    {
        InitHeader(msg);
        msg.m_SessionID = sessionID;
        if (pathLength >= int(sizeof(msg.m_Path))) pathLength = sizeof(msg.m_Path) - 1;
        memcpy(msg.m_Path, path, pathLength);
        msg.m_Path[pathLength] = '\0';
    }

    char m_Path[1024];
};

struct DeleteFileReply : FilesystemSessionPacket
{
    static constexpr Commands::Value COMMAND = Commands::DeleteFileReply;
    static void InitMsg(DeleteFileReply& msg, int32_t sessionID, FilesystemError status) { InitHeader(msg); msg.m_SessionID = sessionID; msg.m_Status = status; }

    FilesystemError m_Status;
};

} //namespace SerialProtocol
