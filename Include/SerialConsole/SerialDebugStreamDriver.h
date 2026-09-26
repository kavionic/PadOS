// This file is part of PadOS.
//
// Copyright (c) 2021-2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
