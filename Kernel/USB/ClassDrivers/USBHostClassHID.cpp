// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/ClassDrivers/USBHIDDriver.h>
#include <Kernel/USB/ClassDrivers/USBHostClassHID.h>
#include <Kernel/USB/ClassDrivers/USBHostHIDInterface.h>

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostClassHID::USBHostClassHID()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostClassHID::UnregisterInputDriver(int handle)
{
    for (auto driverIterator = m_InputDriverRegistrations.begin(); driverIterator != m_InputDriverRegistrations.end(); ++driverIterator)
    {
        if (driverIterator->Handle == handle)
        {
            m_InputDriverRegistrations.erase(driverIterator);
            return true;
        }
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_ClassCode USBHostClassHID::GetClassCode() const
{
    return USB_ClassCode::HID;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const char* USBHostClassHID::GetName() const
{
    return "HID";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostClassHID::Init(USBHost* host)
{
    return USBClassDriverHost::Init(host);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassHID::Shutdown()
{
    USBClassDriverHost::Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescriptorHeader* USBHostClassHID::Open(uint8_t deviceAddress, const USB_DescInterface* interfaceDescriptor, const USB_DescInterfaceAssociation*, const void* endDescriptor)
{
    Ptr<USBHostHIDInterface> hidInterface = ptr_new<USBHostHIDInterface>(m_HostHandler, this);
    const USB_DescriptorHeader* result = hidInterface->Open(deviceAddress, m_NextInterfaceIndex, interfaceDescriptor, endDescriptor);

    m_Interfaces.push_back(hidInterface);
    ++m_NextInterfaceIndex;

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassHID::Close()
{
    for (Ptr<USBHostHIDInterface> hidInterface : m_Interfaces) {
        hidInterface->Close();
    }
    m_Interfaces.clear();
    m_NextInterfaceIndex = 0;
    m_IsActive = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassHID::CloseDevice(uint8_t deviceAddress)
{
    for (auto interfaceIterator = m_Interfaces.begin(); interfaceIterator != m_Interfaces.end(); )
    {
        Ptr<USBHostHIDInterface> hidInterface = *interfaceIterator;

        if (hidInterface->GetDeviceAddress() == deviceAddress)
        {
            hidInterface->Close();
            interfaceIterator = m_Interfaces.erase(interfaceIterator);
        }
        else
        {
            ++interfaceIterator;
        }
    }
    if (m_Interfaces.empty()) {
        m_NextInterfaceIndex = 0;
    }
    m_IsActive = HasActiveInterfaces();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassHID::Startup()
{
    for (Ptr<USBHostHIDInterface> hidInterface : m_Interfaces) {
        if (!hidInterface->IsActive()) {
            hidInterface->Startup();
        }
    }
    m_IsActive = HasActiveInterfaces();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassHID::StartupDevice(uint8_t deviceAddress)
{
    for (Ptr<USBHostHIDInterface> hidInterface : m_Interfaces) {
        if (hidInterface->GetDeviceAddress() == deviceAddress && !hidInterface->IsActive()) {
            hidInterface->Startup();
        }
    }
    m_IsActive = HasActiveInterfaces();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassHID::StartOfFrame()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<USBHIDDriver> USBHostClassHID::CreateInputDriver(USBHostHIDInterface& hidInterface, const USBHIDInterfaceInfo& interfaceInfo) const
{
    const InputDriverRegistration* selectedRegistration = nullptr;
    int selectedScore = USBHIDDriver::PROBE_SCORE_NONE;

    for (const InputDriverRegistration& registration : m_InputDriverRegistrations)
    {
        const int score = registration.Probe(interfaceInfo);
        if (score > selectedScore)
        {
            selectedScore = score;
            selectedRegistration = &registration;
        }
    }

    Ptr<USBHIDDriver> selectedDriver;
    if (selectedRegistration != nullptr) {
        selectedDriver = selectedRegistration->Create(hidInterface);
    }
    return selectedDriver;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostClassHID::HasActiveInterfaces() const
{
    for (Ptr<USBHostHIDInterface> hidInterface : m_Interfaces) {
        if (hidInterface->IsActive()) {
            return true;
        }
    }
    return false;
}

} // namespace kernel
