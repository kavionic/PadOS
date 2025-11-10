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
// Created: 25.05.2022 22:00

#include <string.h>
#include <System/ExceptionHandling.h>
#include <Utils/String.h>
#include <Utils/Utils.h>
#include <Kernel/USB/ClassDrivers/USBClientClassCDC.h>
#include <Kernel/USB/ClassDrivers/USBClientCDCChannel.h>
#include <Kernel/USB/USBDevice.h>
#include <Kernel/USB/USBProtocol.h>

using namespace os;

namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBClientClassCDC::USBClientClassCDC()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientClassCDC::Reset()
{
    for (Ptr<USBClientCDCChannel> channel : m_Channels)
    {
        try
        {
            channel->Close();
        }
        PERROR_CATCH([](PErrorCode error) { kernel_log(LogCategoryUSBHost, PLogSeverity::ERROR, "Failed to close channel."); });

        SignalChannelRemoved(channel);
    }
    m_Channels.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

const USB_DescriptorHeader* USBClientClassCDC::Open(const USB_DescInterface* interfaceDesc, const void* endDesc)
{
    if (interfaceDesc->bInterfaceClass != USB_ClassCode::CDC || USB_CDC_CommSubclassType(interfaceDesc->bInterfaceSubClass) != USB_CDC_CommSubclassType::ABSTRACT_CONTROL_MODEL) {
        return nullptr;
    }
    const USB_DescriptorHeader* desc = interfaceDesc->GetNext();
    while (desc < endDesc && desc->bDescriptorType == USB_DescriptorType::CS_INTERFACE) desc = desc->GetNext();

    if (desc >= endDesc) {
        return nullptr;
    }
    uint8_t  endpointAddrNotifications  = 0;
    uint8_t  endpointOutAddr            = 0;
    uint8_t  endpointInAddr             = 0;
    uint16_t endpointOutSize            = 0;
    uint16_t endpointInSize             = 0;

    if (desc->bDescriptorType == USB_DescriptorType::ENDPOINT)
    {
        const USB_DescEndpoint& endpointDescriptor = *static_cast<const USB_DescEndpoint*>(desc);

        if (!m_DeviceHandler->OpenEndpoint(endpointDescriptor)) {
            return nullptr;
        }
        endpointAddrNotifications = endpointDescriptor.bEndpointAddress;
        desc = desc->GetNext();
    }

    if (desc->bDescriptorType == USB_DescriptorType::INTERFACE && static_cast<const USB_DescInterface*>(desc)->bInterfaceClass == USB_ClassCode::CDC_DATA)
    {
        // Open int/out endpoint pair.
        desc = m_DeviceHandler->OpenEndpointPair(desc->GetNext(), USB_TransferType::BULK, endpointOutAddr, endpointInAddr, endpointOutSize, endpointInSize);
        if (desc == nullptr) {
            return nullptr;
        }
    }

    const uint32_t channelIndex = m_Channels.size();
    Ptr<USBClientCDCChannel> channel = ptr_new<USBClientCDCChannel>(m_DeviceHandler, channelIndex, endpointAddrNotifications, endpointOutAddr, endpointInAddr, endpointOutSize, endpointInSize);
    m_Channels.push_back(channel);

    m_InterfaceToChannelMap[interfaceDesc->bInterfaceNumber] = channel;
    m_EndpointToChannelMap[endpointOutAddr] = channel;
    m_EndpointToChannelMap[endpointInAddr]  = channel;

    SignalChannelAdded(channel);

    return desc;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientClassCDC::HandleControlTransfer(USB_ControlStage stage, const USB_ControlRequest& request)
{
    const USB_RequestType requestType = USB_RequestType((request.bmRequestType & USB_ControlRequest::REQUESTTYPE_TYPE_Msk) >> USB_ControlRequest::REQUESTTYPE_TYPE_Pos);

    if (requestType != USB_RequestType::CLASS) {
        return false;
    }
    const uint16_t interfaceNum = request.wIndex;

    auto channelItr = m_InterfaceToChannelMap.find(interfaceNum);
    if (channelItr != m_InterfaceToChannelMap.end())
    {
        return channelItr->second->HandleControlTransfer(stage, request);
    }
    else
    {
        kernel_log(LogCategoryUSBDevice, PLogSeverity::ERROR, "USBClientClassCDC::HandleControlTransfer() unknown interface {}.", interfaceNum);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBClientClassCDC::HandleDataTransfer(uint8_t endpointAddr, USB_TransferResult result, uint32_t length)
{
    auto channelItr = m_EndpointToChannelMap.find(endpointAddr);
    if (channelItr != m_EndpointToChannelMap.end())
    {
        return channelItr->second->HandleDataTransfer(endpointAddr, result, length);
    }
    else
    {
        kernel_log(LogCategoryUSBDevice, PLogSeverity::ERROR, "USBClientClassCDC::HandleDataTransfer() unknown endpoint {:02x}.", endpointAddr);
        return false;
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<USBClientCDCChannel> USBClientClassCDC::GetChannel(uint32_t channelIndex)
{
    CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
    if (channelIndex < m_Channels.size()) {
        return m_Channels[channelIndex];
    } else {
        return nullptr;
    }
}


} // namespace kernel
