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
// Created: 23.02.2018 01:41:28

#pragma once

#include "sam.h"

#include <stdio.h>

#include <vector>
#include <atomic>
#include <cstdint>
#include <unistd.h>

#include "System/Ptr/PtrTarget.h"
#include "System/Ptr/Ptr.h"

namespace kernel
{

class KDeviceNode;
class KFileHandle;
class KFSVolume;
class KRootFilesystem;

typedef void KIRQHandler(IRQn_Type irq, void* userData);

struct KIRQAction
{

    KIRQHandler* m_Handler;
    void*        m_UserData;
    int32_t      m_Handle;
    KIRQAction*  m_Next;
};

void panic(const char* message);

template<typename ...ARGS>
void kassure(bool expression, ARGS... args)
{
    if (!expression) printf(args...);
}

typedef void AsyncIOResultCallback(void* userObject, void* data, ssize_t length);

class Kernel
{
public:

    static void Initialize();
    static void SystemTick();
    static int RegisterDevice(const char* path, Ptr<KDeviceNode> device);

    static int RegisterIRQHandler(IRQn_Type irqNum, KIRQHandler* handler, void* userData);
    static int UnregisterIRQHandler(IRQn_Type irqNum, int handle);
    static void HandleIRQ(IRQn_Type irqNum);

//    static void SetError(int error) { s_LastError = error; }
//    static int  GetLastError() { return s_LastError; }

    static bigtime_t GetTime();

    static int     OpenFile(const char* path, int flags);
    static int     DupeFile(int oldHandle, int newHandle);
    static int     CloseFile(int handle);
    static ssize_t Read(int handle, void* buffer, size_t length);
    static ssize_t Write(int handle, const void* buffer, size_t length);
    static ssize_t Write(int handle, off_t position, const void* buffer, size_t length);
    static ssize_t Read(int handle, off_t position, void* buffer, size_t length);
    static int     DeviceControl(int handle, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength);

    static int ReadAsync(int handle, off_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback);
    static int WriteAsync(int handle, off_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback);
    static int CancelAsyncRequest(int handle, int requestHandle);

private:
    static int              AllocateFileHandle();
    static void             FreeFileHandle(int handle);
    static Ptr<KFileHandle> GetFile(int handle);
    static void             SetFile(int handle, Ptr<KFileHandle> file);

    static volatile bigtime_t   s_SystemTime;
//    static int                  s_LastError;
    static Ptr<KFileHandle>     s_PlaceholderFile;
    static Ptr<KRootFilesystem> s_RootFilesystem;
    static Ptr<KFSVolume>       s_RootVolume;
    static std::vector<Ptr<KFileHandle>> s_FileTable;
    static KIRQAction*                   s_IRQHandlers[PERIPH_COUNT_IRQn];
};

} // namespace
