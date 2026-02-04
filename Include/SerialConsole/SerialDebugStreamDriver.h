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

#include <Kernel/VFS/KInode.h>
#include <Kernel/VFS/KFilesystem.h>
#include <Kernel/KMutex.h>


struct SerialDebugStreamParameters : KDriverParametersBase
{
    static constexpr char DRIVER_NAME[] = "debug_stream";

    SerialDebugStreamParameters() = default;
    SerialDebugStreamParameters(const PString& devicePath) : KDriverParametersBase(devicePath) {}

    friend void to_json(Pjson& data, const SerialDebugStreamParameters& value)
    {
        to_json(data, static_cast<const KDriverParametersBase&>(value));
    }
    friend void from_json(const Pjson& data, SerialDebugStreamParameters& outValue)
    {
        from_json(data, static_cast<KDriverParametersBase&>(outValue));
    }
};


namespace kernel
{

class SerialDebugStreamInode : public KInode, public KFilesystemFileOps
{
public:
	SerialDebugStreamInode(const SerialDebugStreamParameters& parameters);

    virtual size_t Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position) override;
    virtual void   ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf) override;

private:
};


} // namespace kernel
