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
// Created: 27.06.2026 23:30

#include <algorithm>

#include <Kernel/KLogging.h>
#include <Kernel/KTime.h>
#include <Kernel/USB/USBHostHub.h>
#include <Kernel/USB/USBHost.h>


namespace kernel
{

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::Setup(USBHost* host)
{
    m_Host = host;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::Reset()
{
    m_PortChangeActive = false;
    m_PendingPortChanges.clear();
    m_PollRestartList.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

bool USBHostHub::ConfigureInterface(uint8_t deviceAddr, const USB_DescInterface* interfaceDesc, const void* endDesc, const USB_DescriptorHeader** nextDesc)
{
    if (m_Host == nullptr) {
        return false;
    }

    USBDeviceNode* device = m_Host->GetDevice(deviceAddr);
    if (device == nullptr) {
        return false;
    }

    device->m_IsHub = true;
    device->m_HubStatusEndpoint = USB_INVALID_ENDPOINT;
    device->m_HubStatusEndpointSize = 0;

    const USB_DescriptorHeader* desc = interfaceDesc->GetNext();
    for (; desc < endDesc; desc = desc->GetNext())
    {
        if (desc->bDescriptorType == USB_DescriptorType::INTERFACE || desc->bDescriptorType == USB_DescriptorType::INTERFACE_ASSOCIATION) {
            break;
        }
        if (desc->bDescriptorType != USB_DescriptorType::ENDPOINT) {
            continue;
        }

        const USB_DescEndpoint* endpointDesc = static_cast<const USB_DescEndpoint*>(desc);
        if (endpointDesc->GetTransferType() == USB_TransferType::INTERRUPT && (endpointDesc->bEndpointAddress & USB_ADDRESS_DIR_IN) != 0)
        {
            if (!endpointDesc->Validate(device->m_Speed)) {
                return false;
            }
            device->m_HubStatusEndpoint = endpointDesc->bEndpointAddress;
            device->m_HubStatusEndpointSize = endpointDesc->GetMaxPacketSize();
            break;
        }
    }

    if (device->m_HubStatusEndpoint == USB_INVALID_ENDPOINT || device->m_HubStatusEndpointSize == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Hub interface has no valid interrupt status endpoint.");
        return false;
    }

    device->m_HubStatusPipe = m_Host->AllocPipe(device->m_HubStatusEndpoint);
    if (device->m_HubStatusPipe == USB_INVALID_PIPE)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to allocate hub status pipe.");
        return false;
    }

    if (!m_Host->OpenPipe(device->m_HubStatusPipe, device->m_HubStatusEndpoint, device->m_Address, device->m_Speed, USB_TransferType::INTERRUPT, device->m_HubStatusEndpointSize))
    {
        m_Host->FreePipe(device->m_HubStatusPipe);
        device->m_HubStatusPipe = USB_INVALID_PIPE;
        return false;
    }

    m_Host->SetDataToggle(device->m_HubStatusPipe, false);
    *nextDesc = desc;
    return true;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::Initialize(uint8_t deviceAddr)
{
    if (m_Host == nullptr) {
        return;
    }
    uint8_t* buffer = m_Host->GetControlHandler().GetCtrlDataBuffer();
    m_Host->GetControlHandler().ReqGetHubDescriptor(deviceAddr, buffer, sizeof(USB_DescHub), p_bind_method(this, &USBHostHub::HandleGetDescriptorResult));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::StopInterruptReceive(USBDeviceNode& hub)
{
    if (m_Host == nullptr) {
        return;
    }
    if (hub.m_HubStatusPipe != USB_INVALID_PIPE)
    {
        m_Host->ClosePipe(hub.m_HubStatusPipe);
        m_Host->FreePipe(hub.m_HubStatusPipe);
        hub.m_HubStatusPipe = USB_INVALID_PIPE;
    }
    hub.m_HubStatusBuffer.clear();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::HandleStatusTransaction(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength)
{
    if (m_Host == nullptr) {
        return;
    }

    USBDeviceNode* hub = m_Host->GetHubDevice(pipeIndex);

    if (hub == nullptr) {
        return;
    }
    if (urbState == USB_URBState::NotReady) {
        return;
    }
    if (urbState != USB_URBState::Done)
    {
        QueuePollRestart(hub->m_Address);
        ProcessNextPortChange();
        return;
    }

    bool hasPortChanges = false;
    const size_t bitCount = std::min(transactionLength * 8, size_t(hub->m_HubPortCount) + 1);
    for (size_t portIndex = 1; portIndex < bitCount; ++portIndex)
    {
        const size_t byteIndex = portIndex / 8;
        const uint8_t bitMask = uint8_t(1u << (portIndex & 7));

        if ((hub->m_HubStatusBuffer[byteIndex] & bitMask) != 0)
        {
            QueuePortChange(hub->m_Address, uint8_t(portIndex));
            hasPortChanges = true;
        }
    }

    if (hasPortChanges)
    {
        QueuePollRestart(hub->m_Address);
        ProcessNextPortChange();
    }
    else
    {
        StartInterruptReceive(hub->m_Address);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::CompletePortChange(uint8_t hubAddress)
{
    if (hubAddress != 0) {
        QueuePollRestart(hubAddress);
    }
    m_PortChangeActive = false;
    ProcessNextPortChange();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::HandleGetDescriptorResult(bool result, uint8_t deviceAddr)
{
    if (m_Host == nullptr) {
        return;
    }

    USBDeviceNode* device = m_Host->GetDevice(deviceAddr);
    if (device == nullptr) {
        return;
    }

    if (!result)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Failed to get hub descriptor.");
        m_Host->FinishDeviceConfiguration(deviceAddr);
        return;
    }

    const USB_DescHub* hubDesc = reinterpret_cast<const USB_DescHub*>(m_Host->GetControlHandler().GetCtrlDataBuffer());
    if (hubDesc->bDescriptorType != USB_DescriptorType::HUB || hubDesc->bNbrPorts == 0)
    {
        kernel_log<PLogSeverity::ERROR>(LogCategoryUSBHost, "Invalid hub descriptor.");
        m_Host->FinishDeviceConfiguration(deviceAddr);
        return;
    }

    device->m_HubPortCount = hubDesc->bNbrPorts;
    device->m_HubPowerOnDelayMS = std::max<uint16_t>(hubDesc->GetPowerOnDelayMS(), 100);

    const size_t statusBitmapBytes = (size_t(device->m_HubPortCount) + 1 + 7) / 8;
    device->m_HubStatusBuffer.resize(std::max(device->m_HubStatusEndpointSize, statusBitmapBytes));

    kernel_log<PLogSeverity::INFO_LOW_VOL>(LogCategoryUSBHost, "Hub {} has {} ports.", deviceAddr, device->m_HubPortCount);

    PowerPort(deviceAddr, 1);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::PowerPort(uint8_t deviceAddr, uint8_t portIndex)
{
    if (m_Host == nullptr) {
        return;
    }

    USBDeviceNode* device = m_Host->GetDevice(deviceAddr);
    if (device == nullptr) {
        return;
    }

    if (portIndex > device->m_HubPortCount)
    {
        if (device->m_HubPowerOnDelayMS != 0) {
            snooze_ms(device->m_HubPowerOnDelayMS);
        }
        for (uint8_t port = 1; port <= device->m_HubPortCount; ++port)
        {
            QueuePortChange(deviceAddr, port);
        }
        QueuePollRestart(deviceAddr);
        m_Host->FinishDeviceConfiguration(deviceAddr);
        ProcessNextPortChange();
        return;
    }

    m_Host->GetControlHandler().ReqSetHubPortFeature(deviceAddr, portIndex, USB_HubFeatureSelector::PORT_POWER,
        [this, portIndex](bool result, uint8_t deviceAddr)
        {
            if (!result) {
                kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "Failed to power hub {} port {}.", deviceAddr, portIndex);
            }
            PowerPort(deviceAddr, uint8_t(portIndex + 1));
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::StartInterruptReceive(uint8_t deviceAddr)
{
    if (m_Host == nullptr) {
        return;
    }

    USBDeviceNode* hub = m_Host->GetDevice(deviceAddr);
    if (hub == nullptr || !hub->m_IsHub || hub->m_HubStatusPipe == USB_INVALID_PIPE || hub->m_HubStatusBuffer.empty()) {
        return;
    }
    if (m_Host->GetURBState(hub->m_HubStatusPipe) != USB_URBState::Idle) {
        return;
    }
    std::fill(hub->m_HubStatusBuffer.begin(), hub->m_HubStatusBuffer.end(), 0);
    m_Host->InterruptReceiveData(hub->m_HubStatusPipe, hub->m_HubStatusBuffer.data(), hub->m_HubStatusBuffer.size(), p_bind_method(this, &USBHostHub::HandleStatusTransaction));
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::QueuePortChange(uint8_t hubAddress, uint8_t portIndex)
{
    if (hubAddress == 0 || portIndex == 0) {
        return;
    }
    for (const HubPortEvent& event : m_PendingPortChanges)
    {
        if (event.HubAddress == hubAddress && event.PortIndex == portIndex) {
            return;
        }
    }
    m_PendingPortChanges.emplace_back(hubAddress, portIndex);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::ProcessNextPortChange()
{
    if (m_PortChangeActive) {
        return;
    }

    while (!m_PendingPortChanges.empty())
    {
        const HubPortEvent event = m_PendingPortChanges.front();
        m_PendingPortChanges.pop_front();

        USBDeviceNode* hub = m_Host->GetDevice(event.HubAddress);
        if (hub == nullptr || !hub->m_IsHub || event.PortIndex == 0 || event.PortIndex > hub->m_HubPortCount) {
            continue;
        }

        m_PortChangeActive = true;
        USB_HubPortStatus* status = reinterpret_cast<USB_HubPortStatus*>(m_Host->GetControlHandler().GetCtrlDataBuffer());
        if (m_Host->GetControlHandler().ReqGetHubPortStatus(event.HubAddress, event.PortIndex, status,
            [this, event](bool result, uint8_t deviceAddr)
            {
                HandlePortStatusResult(result, event.HubAddress, event.PortIndex);
            }
        ))
        {
            return;
        }
        m_PortChangeActive = false;
    }
    RestartPendingPolls();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::QueuePollRestart(uint8_t hubAddress)
{
    if (hubAddress == 0) {
        return;
    }
    if (std::find(m_PollRestartList.begin(), m_PollRestartList.end(), hubAddress) == m_PollRestartList.end()) {
        m_PollRestartList.push_back(hubAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::RestartPendingPolls()
{
    std::vector<uint8_t> hubList;
    hubList.swap(m_PollRestartList);

    for (uint8_t hubAddress : hubList)
    {
        StartInterruptReceive(hubAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::HandlePortStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex)
{
    if (!result)
    {
        kernel_log<PLogSeverity::WARNING>(LogCategoryUSBHost, "Failed to get hub {} port {} status.", hubAddress, portIndex);
        CompletePortChange(hubAddress);
        return;
    }

    const USB_HubPortStatus* status = reinterpret_cast<const USB_HubPortStatus*>(m_Host->GetControlHandler().GetCtrlDataBuffer());
    const uint16_t portStatus = PLittleEndianToHost(status->wPortStatus);
    const uint16_t portChange = PLittleEndianToHost(status->wPortChange);

    if ((portChange & USB_HubPortStatus::PORT_CHANGE_CONNECTION) != 0)
    {
        ClearPortConnectionChange(hubAddress, portIndex, portStatus);
        return;
    }

    if ((portStatus & USB_HubPortStatus::PORT_STATUS_CONNECTION) == 0)
    {
        USBDeviceNode* device = m_Host->GetDeviceOnHubPort(hubAddress, portIndex);
        if (device != nullptr) {
            m_Host->CloseDevice(device->m_Address);
        }
        CompletePortChange(hubAddress);
        return;
    }

    if ((portStatus & USB_HubPortStatus::PORT_STATUS_ENABLE) == 0 || m_Host->GetDeviceOnHubPort(hubAddress, portIndex) == nullptr)
    {
        ResetPort(hubAddress, portIndex);
        return;
    }

    CompletePortChange(hubAddress);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::ClearPortConnectionChange(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus)
{
    m_Host->GetControlHandler().ReqClearHubPortFeature(hubAddress, portIndex, USB_HubFeatureSelector::C_PORT_CONNECTION,
        [this, hubAddress, portIndex, portStatus](bool result, uint8_t deviceAddr)
        {
            if (!result)
            {
                CompletePortChange(hubAddress);
                return;
            }

            USBDeviceNode* device = m_Host->GetDeviceOnHubPort(hubAddress, portIndex);
            if (device != nullptr) {
                m_Host->CloseDevice(device->m_Address);
            }

            if ((portStatus & USB_HubPortStatus::PORT_STATUS_CONNECTION) != 0) {
                ResetPort(hubAddress, portIndex);
            } else {
                CompletePortChange(hubAddress);
            }
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::ResetPort(uint8_t hubAddress, uint8_t portIndex)
{
    USBDeviceNode* device = m_Host->GetDeviceOnHubPort(hubAddress, portIndex);
    if (device != nullptr) {
        m_Host->CloseDevice(device->m_Address);
    }

    m_Host->GetControlHandler().ReqSetHubPortFeature(hubAddress, portIndex, USB_HubFeatureSelector::PORT_RESET,
        [this, hubAddress, portIndex](bool result, uint8_t deviceAddr)
        {
            if (!result)
            {
                CompletePortChange(hubAddress);
                return;
            }

            snooze_ms(100);

            USB_HubPortStatus* status = reinterpret_cast<USB_HubPortStatus*>(m_Host->GetControlHandler().GetCtrlDataBuffer());
            m_Host->GetControlHandler().ReqGetHubPortStatus(hubAddress, portIndex, status,
                [this, hubAddress, portIndex](bool result, uint8_t deviceAddr)
                {
                    HandlePortResetStatusResult(result, hubAddress, portIndex);
                }
            );
        }
    );
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::HandlePortResetStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex)
{
    if (!result)
    {
        CompletePortChange(hubAddress);
        return;
    }

    const USB_HubPortStatus* status = reinterpret_cast<const USB_HubPortStatus*>(m_Host->GetControlHandler().GetCtrlDataBuffer());
    const uint16_t portStatus = PLittleEndianToHost(status->wPortStatus);
    const uint16_t portChange = PLittleEndianToHost(status->wPortChange);

    if ((portChange & USB_HubPortStatus::PORT_CHANGE_RESET) != 0)
    {
        m_Host->GetControlHandler().ReqClearHubPortFeature(hubAddress, portIndex, USB_HubFeatureSelector::C_PORT_RESET,
            [this, hubAddress, portIndex, portStatus](bool result, uint8_t deviceAddr)
            {
                HandlePortResetChangeCleared(result, hubAddress, portIndex, portStatus);
            }
        );
    }
    else
    {
        HandlePortResetChangeCleared(true, hubAddress, portIndex, portStatus);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::HandlePortResetChangeCleared(bool result, uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus)
{
    if (!result)
    {
        CompletePortChange(hubAddress);
        return;
    }

    if ((portStatus & USB_HubPortStatus::PORT_STATUS_CONNECTION) == 0 || (portStatus & USB_HubPortStatus::PORT_STATUS_ENABLE) == 0)
    {
        CompletePortChange(hubAddress);
        return;
    }

    EnumeratePortDevice(hubAddress, portIndex, portStatus);
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void USBHostHub::EnumeratePortDevice(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus)
{
    const USB_Speed speed = GetPortDeviceSpeed(portStatus);

    if (!m_Host->EnumeratePortDevice(hubAddress, portIndex, speed)) {
        CompletePortChange(hubAddress);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

USB_Speed USBHostHub::GetPortDeviceSpeed(uint16_t portStatus) const
{
    if ((portStatus & USB_HubPortStatus::PORT_STATUS_HIGH_SPEED) != 0) {
        return USB_Speed::HIGH;
    }
    if ((portStatus & USB_HubPortStatus::PORT_STATUS_LOW_SPEED) != 0) {
        return USB_Speed::LOW;
    }
    return USB_Speed::FULL;
}

} // namespace kernel
