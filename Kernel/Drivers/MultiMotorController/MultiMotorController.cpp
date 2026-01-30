// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 19.10.2025 22:30

#include <System/ExceptionHandling.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/Drivers/MultiMotorController/MultiMotorController.h>
#include <Kernel/Drivers/MultiMotorController/MultiMotorINode.h>


namespace kernel
{

void MultiMotorDriver::Setup(const char* devicePath, const char* controlPortPath, uint32_t baudrate)
{
    Ptr<MultiMotorINode> node = ptr_new<MultiMotorINode>(controlPortPath, baudrate, this);
    Kernel::RegisterDevice_trw(devicePath, node);
}

void MultiMotorDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<MultiMotorINode> node = ptr_static_cast<MultiMotorINode>(file->GetINode());
    node->DeviceControl(request, inData, inDataLength, outData, outDataLength);
}

} // namespace kernel
