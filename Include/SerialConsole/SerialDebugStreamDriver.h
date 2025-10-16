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

#pragma once

#include <Kernel/VFS/KINode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/KMutex.h>

class SerialDebugStreamINode : public kernel::KINode
{
public:
	SerialDebugStreamINode(kernel::KFilesystemFileOps* fileOps);

    size_t Read(Ptr<kernel::KFileNode> file, void* buffer, size_t length);
    size_t Write(Ptr<kernel::KFileNode> file, const void* buffer, size_t length);

private:
	kernel::KMutex m_Mutex;
};


class SerialDebugStreamDriver : public PtrTarget, public kernel::KFilesystemFileOps
{
public:
	SerialDebugStreamDriver() {}

    void Setup(const char* devicePath);

    virtual size_t Read(Ptr<kernel::KFileNode> file, void* buffer, size_t length, off64_t position) override;
    virtual size_t Write(Ptr<kernel::KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void   DeviceControl(Ptr<kernel::KFileNode> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) override;

private:
    SerialDebugStreamDriver(const SerialDebugStreamDriver &other) = delete;
    SerialDebugStreamDriver& operator=(const SerialDebugStreamDriver &other) = delete;
};


