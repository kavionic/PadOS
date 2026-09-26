// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    m_Channels.reserve(m_Channels.size() + 1);
    Ptr<USBHostCDCChannel> channel = ptr_new<USBHostCDCChannel>(m_HostHandler, this);
    const int channelIndex = int(m_NextChannelIndex++);
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
    m_NextChannelIndex = 0;
    m_IsActive = false;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassCDC::CloseDevice(uint8_t deviceAddr)
{
    for (auto channelIterator = m_Channels.begin(); channelIterator != m_Channels.end(); )
    {
        Ptr<USBHostCDCChannel> channel = *channelIterator;

        if (channel->GetDeviceAddress() == deviceAddr)
        {
            channel->Close();
            SignalChannelRemoved(channel);
            channelIterator = m_Channels.erase(channelIterator);
        }
        else
        {
            ++channelIterator;
        }
    }
    if (m_Channels.empty()) {
        m_NextChannelIndex = 0;
    }
    m_IsActive = HasActiveChannels();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassCDC::Startup()
{
    for (Ptr<USBHostCDCChannel> channel : m_Channels)
    {
        if (!channel->IsActive()) {
            channel->Startup();
        }
    }
    m_IsActive = HasActiveChannels();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostClassCDC::StartupDevice(uint8_t deviceAddr)
{
    for (Ptr<USBHostCDCChannel> channel : m_Channels)
    {
        if (channel->GetDeviceAddress() == deviceAddr && !channel->IsActive()) {
            channel->Startup();
        }
    }
    m_IsActive = HasActiveChannels();
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostClassCDC::HasActiveChannels() const
{
    for (Ptr<USBHostCDCChannel> channel : m_Channels)
    {
        if (channel->IsActive()) {
            return true;
        }
    }
    return false;
}


} // namespace kernel
