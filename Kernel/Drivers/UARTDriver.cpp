// This file is part of PadOS.
//
// Copyright (C) 2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 22.02.2018 23:06:22

#include "UARTDriver.h"
#include "Kernel/VFS/KFileHandle.h"
#include "Kernel/VFS/KFSVolume.h"

using namespace kernel;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

UARDDriverINode::UARDDriverINode(UART::Channels channel, KFilesystemFileOps* fileOps) : KINode(nullptr, nullptr, fileOps, false)
{
    m_Port.Initialize(channel, 921600);
    m_Port.SetParity(UART::Parity::NONE);
    m_Port.EnableTX(true);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

UARTDriver::UARTDriver()
{
/*    m_Port.Initialize(channel, 921600);
    m_Port.SetParity(UART::Parity::NONE);
    m_Port.EnableTX(true);*/
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

UARTDriver::~UARTDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void UARTDriver::Setup(const char* devicePath, UART::Channels channel)
{
    Ptr<UARDDriverINode> node = ptr_new<UARDDriverINode>(channel, this);
    Kernel::RegisterDevice(devicePath, node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int UARTDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t UARTDriver::Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length)
{
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t UARTDriver::Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length)
{
    Ptr<UARDDriverINode> node = ptr_static_cast<UARDDriverINode>(file->GetINode());
    node->m_Port.Send(buffer, length);

    return length;
}

