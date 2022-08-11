// This file is part of PadOS.
//
// Copyright (C) 2022 Kurt Skauen <http://kavionic.com/>
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
// Created: 23.07.2022 22:00

#pragma once

#include <stdint.h>
#include <System/Sections.h>
#include <Kernel/USB/USBClassDriverHost.h>
#include <Kernel/USB/USBProtocolCDC.h>



namespace kernel
{
class USBHost;
class USBHostCDCChannel;

class USBHostClassCDC : public USBClassDriverHost
{
public:
    USBHostClassCDC();

    virtual IFLASHC USB_ClassCode               GetClassCode() const override;
    virtual IFLASHC const char*                 GetName() const override;
    virtual IFLASHC bool                        Init(USBHost* host) override;
    virtual IFLASHC void                        Shutdown() override;
    virtual IFLASHC const USB_DescriptorHeader* Open(uint8_t deviceAddr, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc) override;
    virtual IFLASHC void                        Close() override;
    virtual IFLASHC void                        Startup() override;
    virtual IFLASHC void                        StartOfFrame() override;

    uint32_t                GetChannelCount() const { return m_Channels.size(); }
    Ptr<USBHostCDCChannel>  GetChannel(uint32_t channelIndex);

    Signal<void, Ptr<USBHostCDCChannel>> SignalChannelAdded;
    Signal<void, Ptr<USBHostCDCChannel>> SignalChannelRemoved;


private:
    std::vector<Ptr<USBHostCDCChannel>> m_Channels;
};


} // namespace kernel
