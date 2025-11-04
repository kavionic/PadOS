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

#pragma once

#include <DeviceControl/DeviceControlInvoker.h>
#include <RPC/RPCDispatcher.h>
#include <RPC/ArgumentDeserializer.h>

#include <Ptr/PtrTarget.h>
#include <Signals/RemoteSignal.h>
#include <System/ExceptionHandling.h>

#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/VFS/KINode.h>
#include <Kernel/Drivers/MultiMotorController/TMC2209Driver.h>
#include <Kernel/Drivers/MultiMotorController/TMC2209IODriver.h>


namespace kernel
{

class MultiMotorDriver : public PtrTarget, public KFilesystemFileOps
{
public:
    void Setup(const char* devicePath, const char* controlPortPath, uint32_t baudrate);

    virtual void DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

};

} // namespace kernel
