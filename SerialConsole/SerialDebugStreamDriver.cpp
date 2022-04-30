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
// Created: 01.05.2021

#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <SerialConsole/SerialCommandHandler.h>
#include <SerialConsole/SerialDebugStreamDriver.h>


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialDebugStreamINode::SerialDebugStreamINode(kernel::KFilesystemFileOps* fileOps)
    : KINode(nullptr, nullptr, fileOps, false)
    , m_Mutex("SerDebugStream")
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SerialDebugStreamINode::Read(Ptr<kernel::KFileNode> file, void* buffer, size_t length)
{
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SerialDebugStreamINode::Write(Ptr<kernel::KFileNode> file, const void* buffer, size_t length)
{

    return SerialCommandHandler::GetInstance().WriteLogMessage((const char*)buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialDebugStreamDriver::Setup(const char* devicePath)
{
    Ptr<SerialDebugStreamINode> node = ptr_new<SerialDebugStreamINode>(this);
    kernel::Kernel::RegisterDevice(devicePath, node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SerialDebugStreamDriver::Read(Ptr<kernel::KFileNode> file, off64_t position, void* buffer, size_t length)
{
    Ptr<SerialDebugStreamINode> node = ptr_static_cast<SerialDebugStreamINode>(file->GetINode());
    return node->Read(file, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t SerialDebugStreamDriver::Write(Ptr<kernel::KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    Ptr<SerialDebugStreamINode> node = ptr_static_cast<SerialDebugStreamINode>(file->GetINode());
    return node->Write(file, buffer, length);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int SerialDebugStreamDriver::DeviceControl(Ptr<kernel::KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    return -1;
}


