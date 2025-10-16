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

#include <string.h>

#include <Kernel/USB/USBProtocolCDC.h>
#include <Kernel/USB/USBHost.h>
#include <Kernel/USB/ClassDrivers/USBHostClassCDC.h>
#include <Kernel/USB/ClassDrivers/USBHostCDCChannel.h>


namespace kernel
{


///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBHostClassCDC::USBHostClassCDC()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_ClassCode USBHostClassCDC::GetClassCode() const
{
    return USB_ClassCode::CDC;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const char* USBHostClassCDC::GetName() const
{
    return "CDC";
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostClassCDC::Init(USBHost* host)
{
    return USBClassDriverHost::Init(host);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassCDC::Shutdown()
{
    USBClassDriverHost::Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescriptorHeader* USBHostClassCDC::Open(uint8_t deviceAddr, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc)
{
    Ptr<USBHostCDCChannel> channel = ptr_new<USBHostCDCChannel>(m_HostHandler, this);
    const int channelIndex = m_Channels.size();
    const USB_DescriptorHeader* result = channel->Open(deviceAddr, channelIndex, interfaceDesc, interfaceAssociationDesc, endDesc);

    m_Channels.push_back(channel);
    SignalChannelAdded(channel);

    return result;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassCDC::Close()
{
    for (Ptr<USBHostCDCChannel> channel : m_Channels)
    {
        channel->Close();
        SignalChannelRemoved(channel);
    }
    m_Channels.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassCDC::Startup()
{
    for (Ptr<USBHostCDCChannel> channel : m_Channels)
    {
        channel->Startup();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassCDC::StartOfFrame()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<USBHostCDCChannel> USBHostClassCDC::GetChannel(uint32_t channelIndex)
{
    CRITICAL_SCOPE(m_HostHandler->GetMutex());
    if (channelIndex < m_Channels.size()) {
        return m_Channels[channelIndex];
    } else {
        return nullptr;
    }
}


} // namespace kernel
