// This file is part of PadOS.
//
// Copyright (c) 2021-2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 18.04.2021 23:30

#pragma once

#include <map>
#include <set>
#include <vector>

namespace SerialProtocol
{
struct OpenSession;
struct CloseSession;
struct RenameFile;
struct GetVolumeInfo;
struct GetDirectory;
struct CreateFile;
struct CreateDirectory;
struct OpenFile;
struct WriteFile;
struct SetFileStat;
struct ReadFile;
struct CloseFile;
struct DeleteFile;
struct GetDirectoryReplyDirEnt;
}

namespace kernel
{
PDEFINE_LOG_CATEGORY(LogCategorySerialHandlerFS, "SCMDHFS", PLogSeverity::ERROR, PLogChannel::SerialManager);

class SerialCommandHandler;

struct SessionData
{
    std::set<int> m_OpenFiles;
};

class CommandHandlerFilesystem
{
public:
    void Setup(SerialCommandHandler* commandHandler);

private:
    void HandleOpenSession(const SerialProtocol::OpenSession& msg);
    void HandleCloseSession(const SerialProtocol::CloseSession& msg);
    void HandleRenameFile(const SerialProtocol::RenameFile& msg);
    void HandleGetVolumeInfo(const SerialProtocol::GetVolumeInfo& msg);
    void HandleGetDirectory(const SerialProtocol::GetDirectory& packet);
    void HandleCreateFile(const SerialProtocol::CreateFile& msg);
    void HandleCreateDirectory(const SerialProtocol::CreateDirectory& msg);
    void HandleOpenFile(const SerialProtocol::OpenFile& msg);
    void HandleWriteFile(const SerialProtocol::WriteFile& msg);
    void HandleSetFileStat(const SerialProtocol::SetFileStat& msg);
    void HandleReadFile(const SerialProtocol::ReadFile& msg);
    void HandleCloseFile(const SerialProtocol::CloseFile& msg);
    void HandleDeleteFile(const SerialProtocol::DeleteFile& msg);

    bool ValidateSession(int32_t sessionID);
    bool SendDirectoryEntries(int32_t sessionID, const std::vector<SerialProtocol::GetDirectoryReplyDirEnt>& entryList);

    SerialCommandHandler*          m_CommandHandler = nullptr;
    std::map<int32_t, SessionData> m_Sessions;
};


} // namespace kernel
