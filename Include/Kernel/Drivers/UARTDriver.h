// This file is part of PadOS.
//
// Copyright (c) 2018 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 22.02.2018 23:06:23

#pragma once

#include "UART.h"
//#include "Kernel/VFS/KDeviceNode.h"
#include "Kernel/VFS/KInode.h"
#include "Kernel/VFS/KFilesystem.h"

namespace kernel
{

class UARDDriverInode : public KInode
{
public:
    UARDDriverInode(UART::Channels channel, KFilesystemFileOps* fileOps);
    
    UART m_Port;    
};

class UARTDriver : public PtrTarget, public KFilesystemFileOps
{
public:
    UARTDriver();

    void Setup(const char* devicePath, UART::Channels channel);

    virtual ~UARTDriver() override;

//    virtual Ptr<KFileHandle> OpenFile(Ptr<KFSVolume> volume, Ptr<KInode> node, int flags);
//    virtual int              CloseFile(Ptr<KFSVolume> volume, Ptr<KFileHandle> file);

//    virtual Ptr<KDirectoryNode> OpenDirectory(Ptr<KFSVolume> volume, Ptr<KInode> node);
//    virtual int                 CloseDirectory(Ptr<KFSVolume> volume, Ptr<KDirectoryNode> directory);

    virtual ssize_t Read(Ptr<KFileNode> file, off64_t position, void* buffer, size_t length) override;
    virtual ssize_t Write(Ptr<KFileNode> file, off64_t position, const void* buffer, size_t length) override;
    virtual int     DeviceControl(Ptr<KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;


//    virtual int     DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;
//    virtual ssize_t Read(Ptr<KFileHandle> file, off64_t position, void* buffer, size_t length) override;
//    virtual ssize_t Write(Ptr<KFileHandle> file, off64_t position, const void* buffer, size_t length) override;


private:
    UARTDriver( const UARTDriver &c );
    UARTDriver& operator=( const UARTDriver &c );
};

} // namespace
