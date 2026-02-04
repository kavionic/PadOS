// This file is part of PadOS.
//
// Copyright (C) 2021-2024 Kurt Skauen <http://kavionic.com/>
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
