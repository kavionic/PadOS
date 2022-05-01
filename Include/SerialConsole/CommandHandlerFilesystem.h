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

#pragma once

class SerialCommandHandler;

namespace SerialProtocol
{
struct GetDirectory;
struct CreateFile;
struct CreateDirectory;
struct OpenFile;
struct WriteFile;
struct ReadFile;
struct CloseFile;
struct DeleteFile;
}


class CommandHandlerFilesystem
{
public:
    void Setup(SerialCommandHandler* commandHandler);

private:
    void HandleGetDirectory(const SerialProtocol::GetDirectory& packet);
    void HandleCreateFile(const SerialProtocol::CreateFile& msg);
    void HandleCreateDirectory(const SerialProtocol::CreateDirectory& msg);
    void HandleOpenFile(const SerialProtocol::OpenFile& msg);
    void HandleWriteFile(const SerialProtocol::WriteFile& msg);
    void HandleReadFile(const SerialProtocol::ReadFile& msg);
    void HandleCloseFile(const SerialProtocol::CloseFile& msg);
    void HandleDeleteFile(const SerialProtocol::DeleteFile& msg);

    SerialCommandHandler*   m_CommandHandler = nullptr;
    int                     m_CurrentExternalFile = -1;
};


