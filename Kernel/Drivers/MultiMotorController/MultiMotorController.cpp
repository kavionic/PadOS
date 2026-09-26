// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 19.10.2025 22:30

#include <System/ExceptionHandling.h>
#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/Drivers/MultiMotorController/MultiMotorController.h>
#include <Kernel/Drivers/MultiMotorController/MultiMotorInode.h>


namespace kernel
{

void MultiMotorDriver::Setup(const char* devicePath, const char* controlPortPath, uint32_t baudrate)
{
    Ptr<MultiMotorInode> node = ptr_new<MultiMotorInode>(controlPortPath, baudrate, this);
    Kernel::RegisterDevice_trw(devicePath, node);
}

void MultiMotorDriver::DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<MultiMotorInode> node = ptr_static_cast<MultiMotorInode>(file->GetInode());
    node->DeviceControl(request, inData, inDataLength, outData, outDataLength);
}

} // namespace kernel
