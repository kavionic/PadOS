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

#pragma once

#include <deque>
#include <stdint.h>
#include <vector>

#include <Kernel/USB/USBCommon.h>

struct USB_DescInterface;
struct USB_DescriptorHeader;

namespace kernel
{
class USBDeviceNode;
class USBHost;

enum class USB_URBState : uint8_t;

class USBHostHub
{
public:
    void Setup(USBHost* host);
    void Reset();

    bool ConfigureInterface(uint8_t deviceAddr, const USB_DescInterface* interfaceDesc, const void* endDesc, const USB_DescriptorHeader** nextDesc);
    void Initialize(uint8_t deviceAddr);
    void StopInterruptReceive(USBDeviceNode& hub);
    void HandleStatusTransaction(USB_PipeIndex pipeIndex, USB_URBState urbState, size_t transactionLength);
    void CompletePortChange(uint8_t hubAddress);
    void HandlePortEnumerationFailed(uint8_t hubAddress, uint8_t portIndex);

private:
    struct HubPortEvent
    {
        HubPortEvent(uint8_t hubAddress = 0, uint8_t portIndex = 0) noexcept : HubAddress(hubAddress), PortIndex(portIndex) {}

        uint8_t HubAddress;
        uint8_t PortIndex;
    };

    void HandleGetDescriptorResult(bool result, uint8_t deviceAddr);
    void PowerPort(uint8_t deviceAddr, uint8_t portIndex);
    void StartInterruptReceive(uint8_t deviceAddr);
    void QueuePortChange(uint8_t hubAddress, uint8_t portIndex);
    void ProcessNextPortChange();
    void QueuePollRestart(uint8_t hubAddress);
    void RestartPendingPolls();
    void HandlePortStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex);
    void ClearPortConnectionChange(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus);
    void ResetPort(uint8_t hubAddress, uint8_t portIndex);
    void HandlePortResetStatusResult(bool result, uint8_t hubAddress, uint8_t portIndex);
    void HandlePortResetChangeCleared(bool result, uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus, uint16_t portChange);
    void EnumeratePortDevice(uint8_t hubAddress, uint8_t portIndex, uint16_t portStatus);
    USB_Speed GetPortDeviceSpeed(uint16_t portStatus) const;

    USBHost*                m_Host = nullptr;
    std::deque<HubPortEvent> m_PendingPortChanges;
    std::vector<uint8_t>    m_PollRestartList;
    bool                    m_PortChangeActive = false;
};

} // namespace kernel
