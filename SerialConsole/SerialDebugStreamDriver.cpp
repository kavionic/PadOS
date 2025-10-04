// This file is part of PadOS.
//
// Copyright (C) 2021-2024 Kurt Skauen <http://kavionic.com/>
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
    , m_Mutex("SerDebugStream", PEMutexRecursionMode_RaiseError)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode SerialDebugStreamINode::Read(Ptr<kernel::KFileNode> file, void* buffer, size_t length, ssize_t& outLength)
{
    return PErrorCode::NotImplemented;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode SerialDebugStreamINode::Write(Ptr<kernel::KFileNode> file, const void* buffer, size_t length, ssize_t& outLength)
{
    outLength = SerialCommandHandler::Get().WriteLogMessage(buffer, length);
    return PErrorCode::Success;
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

PErrorCode SerialDebugStreamDriver::Read(Ptr<kernel::KFileNode> file, void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    Ptr<SerialDebugStreamINode> node = ptr_static_cast<SerialDebugStreamINode>(file->GetINode());
    return node->Read(file, buffer, length, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode SerialDebugStreamDriver::Write(Ptr<kernel::KFileNode> file, const void* buffer, size_t length, off64_t position, ssize_t& outLength)
{
    Ptr<SerialDebugStreamINode> node = ptr_static_cast<SerialDebugStreamINode>(file->GetINode());
    return node->Write(file, buffer, length, outLength);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int SerialDebugStreamDriver::DeviceControl(Ptr<kernel::KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    return -1;
}


