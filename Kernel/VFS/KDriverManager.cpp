// This file is part of PadOS.
//
// Copyright (C) 2025 Kurt Skauen <http://kavionic.com/>
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
// Created: 04.11.2025 23:30

#include <string.h>

#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/VFS/KRootFilesystem.h>
#include <System/ExceptionHandling.h>

extern const kernel::KDriverDescriptor* const _driver_descriptors_start;
extern const kernel::KDriverDescriptor* const _driver_descriptors_end;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const KDriverDescriptor* kget_driver_descriptor_trw(const char* name)
{
    for (const KDriverDescriptor* const* i = &_driver_descriptors_start; i != &_driver_descriptors_end; ++i)
    {
        const KDriverDescriptor* const descriptor = *i;
        if (strcmp(name, descriptor->Name) == 0) {
            return descriptor;
        }
    }
    PERROR_THROW_CODE(PErrorCode::NoEntry);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksetup_device_driver_trw(const char* name, const char* parameters)
{
    try
    {
        ksystem_log(LogCatKernel_Drivers, PLogSeverity::INFO_LOW_VOL, "Setting up driver '{}'.", name);
        const KDriverDescriptor* descriptor = kget_driver_descriptor_trw(name);
        descriptor->Initialize(parameters);
    }
    PERROR_CATCH([name](PErrorCode error)
        {
            ksystem_log(LogCatKernel_Drivers, PLogSeverity::ERROR, "Failed to setup driver '{}': {}", name, strerror(std::to_underlying(error)));
            throw;
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kregister_device_root_trw(const char* devicePath, Ptr<KINode> rootINode)
{
    return kget_rootfs_trw()->RegisterDevice(devicePath, rootINode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void krename_device_root_trw(int handle, const char* newPath)
{
    kget_rootfs_trw()->RenameDevice(handle, newPath);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void kremove_device_root_trw(int handle)
{
    kget_rootfs_trw()->RemoveDevice(handle);
}

} // namespace kernel
