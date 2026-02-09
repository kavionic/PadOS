// This file is part of PadOS.
//
// Copyright (C) 2017-2018 Kurt Skauen <http://kavionic.com/>
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
// Created: 18/06/19 23:44:30

#include <Kernel/VFS/KIOContext.h>
#include <Kernel/VFS/KNodeMonitor.h>
#include <Kernel/VFS/KInode.h>

namespace kernel
{

Ptr<KFileTableNode> KIOContext::s_PlaceholderFile;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KIOContext::KIOContext() : m_Mutex("iocontext", PEMutexRecursionMode_RaiseError)
{
    if (s_PlaceholderFile == nullptr) {
        s_PlaceholderFile = ptr_new<KFileTableNode>(0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

KIOContext::~KIOContext()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KIOContext::SetCurrentDirectory(Ptr<KInode> inode)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    m_CurrentDirectory = inode;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KInode> KIOContext::GetCurrentDirectory() const
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return m_CurrentDirectory;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KIOContext::AllocFileHandle()
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return AllocFileHandle_pl();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KIOContext::FreeFileHandle(int handle) noexcept
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    if (handle >= 0 && handle < int(m_FileTable.size())) {
        m_FileTable[handle] = nullptr;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileTableNode> KIOContext::GetFileNode(int handle) const
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    return GetFileNode_pl(handle);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KIOContext::SetFileNode(int handle, Ptr<KFileTableNode> node)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);
    SetFileNode_pl(handle, node);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KIOContext::DupeFileHandle(int oldHandle, int newHandle)
{
    kassert(!m_Mutex.IsLocked());
    CRITICAL_SCOPE(m_Mutex);

    if (oldHandle < 0 || oldHandle >= OPEN_MAX || newHandle >= OPEN_MAX) {
        PERROR_THROW_CODE(PErrorCode::MFILE);
    }
    if (oldHandle == newHandle) {
        return oldHandle;
    }
    for (;;)
    {
        Ptr<KFileTableNode> file = GetFileNode_pl(oldHandle);
        if (newHandle < 0)
        {
            newHandle = AllocFileHandle_pl();
        }
        else
        {
            if (newHandle < int(m_FileTable.size() && m_FileTable[newHandle] != nullptr))
            {
                m_Mutex.Unlock();
                kclose(newHandle);
                m_Mutex.Lock();
                continue;
            }
            if (newHandle >= int(m_FileTable.size())) {
                m_FileTable.resize(newHandle + 1);
            }
        }
        SetFileNode_pl(newHandle, file);
        return newHandle;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int KIOContext::AllocFileHandle_pl()
{
    kassert(m_Mutex.IsLocked());

    auto i = std::find(m_FileTable.begin(), m_FileTable.end(), nullptr);
    if (i != m_FileTable.end())
    {
        int handle = i - m_FileTable.begin();
        m_FileTable[handle] = s_PlaceholderFile;
        return handle;
    }
    else
    {
        if (m_FileTable.size() >= OPEN_MAX) {
            PERROR_THROW_CODE(PErrorCode::MFILE);
        }
        int handle = m_FileTable.size();
        m_FileTable.push_back(s_PlaceholderFile);
        return handle;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<KFileTableNode> KIOContext::GetFileNode_pl(int handle) const
{
    kassert(m_Mutex.IsLocked());

    if (handle >= 0 && handle < int(m_FileTable.size()) && m_FileTable[handle] != nullptr && m_FileTable[handle]->GetInode() != nullptr) {
        return m_FileTable[handle];
    }
    PERROR_THROW_CODE(PErrorCode::BadFile);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void KIOContext::SetFileNode_pl(int handle, Ptr<KFileTableNode> node)
{
    kassert(m_Mutex.IsLocked());

    if (handle >= 0 && handle < int(m_FileTable.size()))
    {
        m_FileTable[handle] = node;
    }
}

} // namespace
