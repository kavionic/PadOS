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
// Created: 16.02.2018 20:54:52

#pragma once

#include <unistd.h>

#include "KFilesystem.h"
#include "System/System.h"


namespace kernel
{

class KDeviceNode : public PtrTarget
{
public:
    KDeviceNode();
    virtual ~KDeviceNode();

    virtual void Tick() {}

    virtual Ptr<KFileHandle> Open(int flags);
    virtual status_t         Close(Ptr<KFileHandle> file);

    virtual int     DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength) { set_last_error(ENOSYS); return -1; }

    virtual ssize_t Read(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length) { set_last_error(ENOSYS); return -1; }
    virtual ssize_t Write(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length) { set_last_error(ENOSYS); return -1; }

    virtual int ReadAsync(Ptr<KFileHandle> file, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) { return -1; }
    virtual int WriteAsync(Ptr<KFileHandle> file, off_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback) { return -1; }
    virtual int CancelAsyncRequest(Ptr<KFileHandle> file, int handle) { set_last_error(ENOSYS); return -1; }

private:
    KDeviceNode( const KDeviceNode &c );
    KDeviceNode& operator=( const KDeviceNode &c );

};


} // namespace
