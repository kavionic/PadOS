// This file is part of PadOS.
//
// Copyright (c) 2021-2024 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 01.05.2021

#include <Kernel/VFS/KFileHandle.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/VFS/KDriverParametersBase.h>
#include <System/ExceptionHandling.h>
#include <SerialConsole/SerialDebugStreamDriver.h>

namespace kernel
{


PREGISTER_KERNEL_DRIVER(SerialDebugStreamInode, SerialDebugStreamParameters);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

SerialDebugStreamInode::SerialDebugStreamInode(const SerialDebugStreamParameters& parameters)
    : KInode(nullptr, nullptr, this, S_IFCHR | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)
{

}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t SerialDebugStreamInode::Write(Ptr<KFileNode> file, const void* buffer, size_t length, off64_t position)
{
    kernel_log<PLogSeverity::NOTICE>(LogCatKernel_General, "{}", std::string_view(reinterpret_cast<const char*>(buffer), length));
    return length;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void SerialDebugStreamInode::ReadStat(Ptr<KFSVolume> volume, Ptr<KInode> inode, struct stat* statBuf)
{
    KFilesystemFileOps::ReadStat(volume, inode, statBuf);
}

} // namespace kernel
