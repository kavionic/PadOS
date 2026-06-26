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
#include <Kernel/KLogging.h>
#include <Kernel/USB/ClassDrivers/USBClientClassCDC.h>
#include <Kernel/USB/ClassDrivers/USBClientCDCChannel.h>
#include <Kernel/USB/USBDevice.h>
#include <Kernel/USB/USBProtocol.h>
#include <Kernel/VFS/KDriverManager.h>


namespace kernel
{

class USBClientClassCDC::DeviceNodeCleanupThread : public KThread
{
public:
    explicit DeviceNodeCleanupThread(USBClientClassCDC& driver)
        : KThread("usb_cdc_cleanup")
        , m_Driver(driver)
    {
        SetDeleteOnExit(false);
    }

    virtual void* Run() override
    {
        return m_Driver.RunDeviceNodeCleanup();
    }

private:
    USBClientClassCDC& m_Driver;
};

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBClientClassCDC::USBClientClassCDC()
    : m_DeferredNodeMutex("usb_cdc_cleanup", PEMutexRecursionMode_RaiseError)
    , m_DeferredNodeCondition("usb_cdc_cleanup")
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USBClientClassCDC::~USBClientClassCDC()
{
    StopCleanupThread();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientClassCDC::Init(USBDevice* deviceHandler)
{
    USBClassDriverDevice::Init(deviceHandler);

    if (m_CleanupThread == nullptr)
    {
        m_CleanupThreadStopRequested = false;
        m_CleanupThread = std::make_unique<DeviceNodeCleanupThread>(*this);
        m_CleanupThread->Start_trw(KSpawnThreadFlag::None, PThreadDetachState_Joinable);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientClassCDC::Shutdown()
{
    kassert(m_DeviceHandler == nullptr || !m_DeviceHandler->GetMutex().IsLocked());

    std::vector<Ptr<USBClientCDCChannel>> closedChannels;
    std::vector<int> devNodeHandles;

    if (m_DeviceHandler != nullptr)
    {
        {
            CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
            CloseChannels(closedChannels, devNodeHandles);
        }

        for (Ptr<USBClientCDCChannel> channel : closedChannels) {
            SignalChannelRemoved(channel);
        }
    }

    QueueDeviceNodeRemoval(devNodeHandles);
    StopCleanupThread();

    USBClassDriverDevice::Shutdown();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientClassCDC::Reset()
{
    kassert(m_DeviceHandler != nullptr);
    kassert(!m_DeviceHandler->GetMutex().IsLocked());

    std::vector<Ptr<USBClientCDCChannel>> closedChannels;
    std::vector<int> devNodeHandles;

    {
        CRITICAL_SCOPE(m_DeviceHandler->GetMutex());
        CloseChannels(closedChannels, devNodeHandles);
    }

    QueueDeviceNodeRemoval(devNodeHandles);

    for (Ptr<USBClientCDCChannel> channel : closedChannels) {
        SignalChannelRemoved(channel);
    }

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "USBClientClassCDC::Reset() completed.");
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
    bool     hasDataInterface           = false;
    uint8_t  dataInterfaceNum           = 0;
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
        const USB_DescInterface* dataInterfaceDesc = static_cast<const USB_DescInterface*>(desc);
        hasDataInterface = true;
        dataInterfaceNum = dataInterfaceDesc->bInterfaceNumber;

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
    if (hasDataInterface) {
        m_InterfaceToChannelMap[dataInterfaceNum] = channel;
    }
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
    const uint8_t interfaceNum = uint8_t(request.wIndex & 0xff);

    auto channelItr = m_InterfaceToChannelMap.find(interfaceNum);
    if (channelItr != m_InterfaceToChannelMap.end())
    {
        return channelItr->second->HandleControlTransfer(stage, request);
    }
    else
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBClientClassCDC::HandleControlTransfer() unknown interface {}.", interfaceNum);
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
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "USBClientClassCDC::HandleDataTransfer() unknown endpoint {:02x}.", endpointAddr);
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

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientClassCDC::CloseChannels(std::vector<Ptr<USBClientCDCChannel>>& closedChannels, std::vector<int>& devNodeHandles)
{
    kassert(m_DeviceHandler != nullptr);
    kassert(m_DeviceHandler->GetMutex().IsLocked());

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "USBClientClassCDC::Reset() closing {} channel(s).", m_Channels.size());

    closedChannels.swap(m_Channels);
    devNodeHandles.reserve(closedChannels.size());

    for (Ptr<USBClientCDCChannel> channel : closedChannels)
    {
        try
        {
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "USBClientClassCDC::Reset() close channel.");
            const int devNodeHandle = channel->Close();
            if (devNodeHandle != -1) {
                devNodeHandles.push_back(devNodeHandle);
            }
            kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBDevice, "USBClientClassCDC::Reset() channel closed.");
        }
        PERROR_CATCH([](PErrorCode error)
        {
            kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Failed to close channel: {}.", std::to_underlying(error));
        });
    }

    m_InterfaceToChannelMap.clear();
    m_EndpointToChannelMap.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientClassCDC::QueueDeviceNodeRemoval(std::vector<int>& devNodeHandles)
{
    if (!devNodeHandles.empty())
    {
        CRITICAL_SCOPE(m_DeferredNodeMutex);
        m_DeferredDevNodeHandles.insert(m_DeferredDevNodeHandles.end(), devNodeHandles.begin(), devNodeHandles.end());
        m_DeferredNodeCondition.WakeupAll();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBClientClassCDC::StopCleanupThread()
{
    if (m_CleanupThread != nullptr)
    {
        {
            CRITICAL_SCOPE(m_DeferredNodeMutex);
            m_CleanupThreadStopRequested = true;
            m_DeferredNodeCondition.WakeupAll();
        }

        m_CleanupThread->Join_trw();
        m_CleanupThread.reset();
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void* USBClientClassCDC::RunDeviceNodeCleanup()
{
    for (;;)
    {
        std::vector<int> devNodeHandles;

        {
            CRITICAL_SCOPE(m_DeferredNodeMutex);

            while (m_DeferredDevNodeHandles.empty() && !m_CleanupThreadStopRequested) {
                m_DeferredNodeCondition.Wait(m_DeferredNodeMutex);
            }
            if (m_DeferredDevNodeHandles.empty() && m_CleanupThreadStopRequested) {
                break;
            }
            devNodeHandles.swap(m_DeferredDevNodeHandles);
        }

        for (int devNodeHandle : devNodeHandles)
        {
            try
            {
                kremove_device_root_trw(devNodeHandle);
            }
            PERROR_CATCH([](PErrorCode error)
            {
                kernel_log<PLogSeverity::ERROR>(LogCategoryUSBDevice, "Failed to remove CDC device node: {}.", std::to_underlying(error));
            });
        }
    }
    return nullptr;
}


} // namespace kernel
