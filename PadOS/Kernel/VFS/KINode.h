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
// Created: 23.02.2018 01:44:32

#pragma once

#include "System/Ptr/PtrTarget.h"
#include "System/Ptr/Ptr.h"

namespace kernel
{

class KFilesystem;
class KFSVolume;
class KINode;

class KINode : public PtrTarget
{
    public:
    KINode(Ptr<KFilesystem> filesystem, Ptr<KFSVolume> volume);
    Ptr<KFilesystem> m_Filesystem;
    Ptr<KFSVolume>   m_Volume; // The volume this inode came from.
    Ptr<KINode>      m_MountRoot; // Root node of filesystem mounted on this inode if any.
    uint64_t         m_Number = 0;

};

} // namespace
