// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
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

#include <Kernel/USB/USBHost.h>
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
    Ptr<USBHostHIDInterface> hidInterface = ptr_new<USBHostHIDInterface>(m_HostHandler);
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
