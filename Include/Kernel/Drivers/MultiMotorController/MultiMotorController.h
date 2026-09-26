// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
#include <Kernel/VFS/KInode.h>
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
