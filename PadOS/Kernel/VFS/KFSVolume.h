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
// Created: 23.02.2018 01:46:15

#pragma once

#include <string>

#include "Ptr/PtrTarget.h"
#include "Ptr/Ptr.h"
#include "Utils/String.h"
#include "KFilesystem.h"

namespace kernel
{

class KFilesystem;
class KINode;

enum
{
    VOLID_ROOT = 1,
    VOLID_FIRST_NORMAL = 100 // Reserve the first IDs for special mounts like root, dev, pty, etc etc
};

class KFSVolume : public PtrTarget
{
public:
    KFSVolume(fs_id volumeID, const os::String& devicePath);
    
    void     SetFlags(uint32_t flags) { m_Flags = flags; }
    uint32_t GetFlags() const         { return m_Flags; }
    bool     HasFlag(FSVolumeFlags flag) const { return m_Flags & uint32_t(flag); }
    
    fs_id            m_VolumeID;
    uint32_t         m_Flags;
    Ptr<KFilesystem> m_Filesystem;
    Ptr<KINode>      m_MountPoint;
    os::String       m_DevicePath;

    Ptr<KINode>      m_RootNode;
};

} // namespace
