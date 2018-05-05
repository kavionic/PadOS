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
// Created: 23.02.2018 01:49:14

#include "sam.h"

#include <string.h>

#include "KRootFilesystem.h"
#include "KFSVolume.h"
#include "KFileHandle.h"
#include "KDeviceNode.h"
#include "System/System.h"
#include "System/String.h"

using namespace kernel;
using namespace os;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFSVolume> KRootFilesystem::Mount(Ptr<KINode> mountPoint, const char* devicePath, int devicePathLength)
{
    m_Volume = ptr_new<KFSVolume>(ptr_tmp_cast(this), mountPoint, devicePath);

    if (m_Volume == nullptr)
    {
        set_last_error(ENOMEM);
        return nullptr;
    }
    Ptr<KRootFSINode> rootNode = ptr_new<KRootFSINode>(ptr_tmp_cast(this), m_Volume);
    m_Volume->m_RootNode = rootNode;
    if (m_Volume->m_RootNode == nullptr)
    {
        set_last_error(ENOMEM);
        return nullptr;
    }
    m_DevRoot = ptr_new<KRootFSINode>(ptr_tmp_cast(this), m_Volume);
    if (m_DevRoot == nullptr)
    {
        set_last_error(ENOMEM);
        return nullptr;
    }
    rootNode->m_Children["dev"] = m_DevRoot;
    return m_Volume;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KRootFSINode> KRootFilesystem::LocateParentInode(Ptr<KRootFSINode> parent, const char* path, int pathLength, bool createParents, int* outNameStart)
{
    Ptr<KRootFSINode> current = parent;

    int nameStart = 0;
    for (int i = 0; i <= pathLength; ++i)
    {
        if (i == pathLength)
        {
            *outNameStart = nameStart;
            return current;
        }
        if (path[i] == '/')
        {
            if (i == nameStart) {
                nameStart = i + 1;
                continue;
            }
            String name(path + nameStart, i - nameStart);
            auto nodeIterator = current->m_Children.find(name);
            if (nodeIterator != current->m_Children.end())
            {
                current = nodeIterator->second;
            }
            else
            {
                if (createParents)
                {
                    Ptr<KRootFSINode> folder = ptr_new<KRootFSINode>(ptr_tmp_cast(this), m_Volume);

                    if (folder == nullptr) return nullptr;
                    current->m_Children[name] = folder;
                    current = folder;
                }
                else
                {
                    return nullptr;
                }
            }
            nameStart = i + 1;
        }
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KINode> KRootFilesystem::LocateInode(Ptr<KINode> parent, const char* path, int pathLength)
{
    Ptr<KRootFSINode> current = ptr_static_cast<KRootFSINode>(parent);

    if (current == nullptr)
    {
        set_last_error(ENOENT);
        return nullptr;
    }
    int nameStart = 0;

    current = LocateParentInode(current, path, pathLength, false, &nameStart);

    int nameLength = pathLength - nameStart;
    if (current == nullptr || nameLength == 0)
    {
        set_last_error(ENOENT);
        return nullptr;
    }

    auto nodeIterator = current->m_Children.find(String(path + nameStart, nameLength));
    if (nodeIterator != current->m_Children.end())
    {
        return nodeIterator->second;
    }
    else
    {
        set_last_error(ENOENT);
        return nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileHandle> KRootFilesystem::OpenFile(Ptr<KINode> node, int flags)
{
    Ptr<KRootFSINode> inode = ptr_static_cast<KRootFSINode>(node);
    Ptr<KFileHandle> file;
    if (inode->m_DeviceNode != nullptr)
    {
        set_last_error(0);
        file = inode->m_DeviceNode->Open(flags);
        if (file == nullptr && get_last_error() != 0) {
            return nullptr;
        }
    }
    if (file == nullptr)
    {
        file = ptr_new<KFileHandle>();
        file->m_INode = node;
        if (file == nullptr)
        {
            set_last_error(ENOMEM);
            return nullptr;
        }
    }
    file->m_INode = node;
    return file;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileHandle> KRootFilesystem::CreateFile(Ptr<KINode> parent, const char* name, int nameLength, int flags, int permission)
{
    set_last_error(ENOSYS);
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t KRootFilesystem::Read(Ptr<KFileHandle> file, off64_t position, void* buffer, size_t length)
{
    Ptr<KRootFSINode> inode = ptr_static_cast<KRootFSINode>(file->m_INode);
    if (inode->m_DeviceNode != nullptr)
    {
        return inode->m_DeviceNode->Read(file, position, buffer, length);
    }
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t KRootFilesystem::Write(Ptr<KFileHandle> file, off64_t position, const void* buffer, size_t length)
{
    Ptr<KRootFSINode> inode = ptr_static_cast<KRootFSINode>(file->m_INode);
    if (inode->m_DeviceNode != nullptr)
    {
        return inode->m_DeviceNode->Write(file, position, buffer, length);
    }
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::DeviceControl(Ptr<KFileHandle> file, int request, const void* inData, size_t inDataLength, void* outData, size_t outDataLength)
{
    Ptr<KRootFSINode> inode = ptr_static_cast<KRootFSINode>(file->m_INode);
    if (inode->m_DeviceNode != nullptr)
    {
        return inode->m_DeviceNode->DeviceControl(file, request, inData, inDataLength, outData, outDataLength);
    }
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::ReadAsync(Ptr<KFileHandle> file, off64_t position, void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback)
{
    Ptr<KRootFSINode> inode = ptr_static_cast<KRootFSINode>(file->m_INode);
    if (inode->m_DeviceNode != nullptr)
    {
        return inode->m_DeviceNode->ReadAsync(file, position, buffer, length, userObject, callback);
    }
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::WriteAsync(Ptr<KFileHandle> file, off64_t position, const void* buffer, size_t length, void* userObject, AsyncIOResultCallback* callback)
{
    Ptr<KRootFSINode> inode = ptr_static_cast<KRootFSINode>(file->m_INode);
    if (inode->m_DeviceNode != nullptr)
    {
        return inode->m_DeviceNode->WriteAsync(file, position, buffer, length, userObject, callback);
    }
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::CancelAsyncRequest(Ptr<KFileHandle> file, int handle)
{
    Ptr<KRootFSINode> inode = ptr_static_cast<KRootFSINode>(file->m_INode);
    if (inode->m_DeviceNode != nullptr)
    {
        return inode->m_DeviceNode->CancelAsyncRequest(file, handle);
    }
    set_last_error(ENOSYS);
    return -1;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KRootFilesystem::RegisterDevice(const char* path, Ptr<KDeviceNode> device)
{
    int pathLength = strlen(path);

    int nameStart = 0;
    Ptr<KRootFSINode> parent = LocateParentInode(m_DevRoot, path, pathLength, true, &nameStart);

    int nameLength = pathLength - nameStart;
    if (parent == nullptr || nameLength == 0)
    {
        return -1;
    }
    Ptr<KRootFSINode> deviceInode = ptr_new<KRootFSINode>(ptr_tmp_cast(this), m_Volume);
    deviceInode->m_DeviceNode = device;
    parent->m_Children[path + nameStart] = deviceInode;
    return 0;
}
