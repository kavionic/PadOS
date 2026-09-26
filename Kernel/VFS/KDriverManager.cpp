// This file is part of PadOS.
//
// Copyright (c) 2025 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 04.11.2025 23:30

#include <string.h>

#include <Kernel/VFS/KDriverManager.h>
#include <Kernel/VFS/KFSVolume.h>
#include <Kernel/VFS/KDriverDescriptor.h>
#include <Kernel/VFS/KRootFilesystem.h>
#include <System/ExceptionHandling.h>
#include <Utils/Logging.h>

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
    PERROR_THROW_CODE(PErrorCode::NOENT);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void ksetup_device_driver_trw(const char* name, const char* parameters)
{
    try
    {
        kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCatKernel_Drivers, "Setting up driver '{}'.", name);
        const KDriverDescriptor* descriptor = kget_driver_descriptor_trw(name);
        descriptor->Initialize(parameters);
    }
    PERROR_CATCH([name](PErrorCode error)
        {
            kernel_log<PLogSeverity::ERROR>(LogCatKernel_Drivers, "Failed to setup driver '{}': {}", name, strerror(std::to_underlying(error)));
            throw;
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

int kregister_device_root_trw(const char* devicePath, Ptr<KInode> rootInode)
{
    return kget_rootfs_trw()->RegisterDevice(devicePath, rootInode);
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PErrorCode kremove_device_root(int handle) noexcept
{
    try
    {
        kremove_device_root_trw(handle);
        return PErrorCode::Success;
    }
    PERROR_CATCH_RET_CODE;
}

} // namespace kernel
