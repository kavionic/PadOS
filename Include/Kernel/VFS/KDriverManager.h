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

#pragma once

#include <Utils/JSON.h>
#include <Kernel/VFS/FileIO.h>

struct KDriverParametersBase;

namespace kernel
{
struct KDriverDescriptor;

const KDriverDescriptor* kget_driver_descriptor_trw(const char* name);

void ksetup_device_driver_trw(const char* name, const char* parameters);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T>
    requires std::convertible_to<T, KDriverParametersBase>
void ksetup_device_driver_trw(const T& parameters)
{
    Pjson parameterData = parameters;
    ksetup_device_driver_trw(T::DRIVER_NAME, parameterData.dump().c_str());
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename TInitializerObject, typename... TArgumentTypes>
void ksetup_device_driver_trw(TArgumentTypes&& ...parameters)
{
    ksetup_device_driver_trw(TInitializerObject(std::forward<TArgumentTypes>(parameters)...));
}


int     kregister_device_root_trw(const char* devicePath, Ptr<KINode> rootINode);
void    krename_device_root_trw(int handle, const char* newPath);
void    kremove_device_root_trw(int handle);


template<class T> concept kregister_device_driver_has_register_device = requires(T t) { { t->RegisterDevice() } -> std::convertible_to<int>; };

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T> requires kregister_device_driver_has_register_device<T>
int kregister_device_driver_trw(const char* devicePath, T driver)
{
    return driver->RegisterDevice();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

template<typename T> requires (!kregister_device_driver_has_register_device<T>)
int kregister_device_driver_trw(const char* devicePath, T rootINode)
{
    return kregister_device_root_trw(devicePath, rootINode); // Kernel::RegisterDevice_trw(devicePath, rootINode);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////


#define PREGISTER_KERNEL_DRIVER(DRIVER_CLASS, PARAMETER_CLASS) \
    static bool __SetupDevice##DRIVER_CLASS(const char* parameters) \
    { \
        PARAMETER_CLASS parametersData; \
        Pjson::parse(parameters).get_to(parametersData); \
        printf("Setup driver '%s'\n", PARAMETER_CLASS::DRIVER_NAME); \
        Ptr<DRIVER_CLASS> driver = ptr_new<DRIVER_CLASS>(parametersData); \
        kregister_device_driver_trw(parametersData.DevicePath.c_str(), driver); \
        return true; \
    } \
    static KDriverDescriptor descriptor##DRIVER_CLASS = { PARAMETER_CLASS::DRIVER_NAME, __SetupDevice##DRIVER_CLASS }; \
    SECTION_DEVICE_DESCRIPTORS KDriverDescriptor* __DriverDesc##DRIVER_CLASS = &descriptor##DRIVER_CLASS


} // namespace kernel
