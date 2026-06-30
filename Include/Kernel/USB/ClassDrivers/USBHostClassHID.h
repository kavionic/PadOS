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

#pragma once

#include <stdint.h>
#include <vector>

#include <Ptr/Ptr.h>
#include <Kernel/USB/USBProtocolHID.h>
#include <Kernel/USB/USBClassDriverHost.h>

namespace kernel
{

class USBHostHIDInterface;

class USBHostClassHID : public USBClassDriverHost
{
public:
    USBHostClassHID();

    virtual USB_ClassCode               GetClassCode() const override;
    virtual const char*                 GetName() const override;
    virtual bool                        Init(USBHost* host) override;
    virtual void                        Shutdown() override;
    virtual const USB_DescriptorHeader* Open(uint8_t deviceAddress, const USB_DescInterface* interfaceDescriptor, const USB_DescInterfaceAssociation* interfaceAssociationDescriptor, const void* endDescriptor) override;
    virtual void                        Close() override;
    virtual void                        CloseDevice(uint8_t deviceAddress) override;
    virtual void                        Startup() override;
    virtual void                        StartupDevice(uint8_t deviceAddress) override;
    virtual void                        StartOfFrame() override;

private:
    bool HasActiveInterfaces() const;

    std::vector<Ptr<USBHostHIDInterface>>   m_Interfaces;
    uint32_t                                m_NextInterfaceIndex = 0;
};

} // namespace kernel
