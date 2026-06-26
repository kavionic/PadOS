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


#pragma once

#include <map>
#include <memory>
#include <vector>
#include <System/Platform.h>
#include <Signals/Signal.h>
#include <Ptr/PtrTarget.h>
#include <Kernel/HAL/STM32/USB_STM32.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/KMutex.h>
#include <Kernel/KThread.h>
#include <Kernel/USB/USBClassDriverDevice.h>
#include <Kernel/USB/USBProtocolCDC.h>

namespace kernel
{
class USBClientCDCChannel;

class USBClientClassCDC : public USBClassDriverDevice
{
public:
    USBClientClassCDC();
    virtual ~USBClientClassCDC();

    virtual const char*                 GetName() const override { return "CDC"; }
    virtual uint32_t                    GetInterfaceCount() override { return 2; }
    virtual void                        Init(USBDevice* deviceHandler) override;
    virtual void                        Shutdown() override;
    virtual void                        Reset() override;
    virtual const USB_DescriptorHeader* Open(const USB_DescInterface* desc_intf, const void* endDesc) override;
    virtual bool                        HandleControlTransfer(USB_ControlStage stage, const USB_ControlRequest& request) override;
    virtual bool                        HandleDataTransfer(uint8_t endpointAddr, USB_TransferResult result, uint32_t length) override;

    uint32_t            GetChannelCount() const { return m_Channels.size(); }
    Ptr<USBClientCDCChannel>  GetChannel(uint32_t channelIndex);

    Signal<void, Ptr<USBClientCDCChannel>> SignalChannelAdded;
    Signal<void, Ptr<USBClientCDCChannel>> SignalChannelRemoved;

private:
    class DeviceNodeCleanupThread;

    void CloseChannels(std::vector<Ptr<USBClientCDCChannel>>& closedChannels, std::vector<int>& devNodeHandles);
    void QueueDeviceNodeRemoval(std::vector<int>& devNodeHandles);
    void StopCleanupThread();
    void* RunDeviceNodeCleanup();

    std::vector<Ptr<USBClientCDCChannel>>         m_Channels;
    std::map<uint16_t, Ptr<USBClientCDCChannel>>  m_InterfaceToChannelMap;
    std::map<uint8_t, Ptr<USBClientCDCChannel>>   m_EndpointToChannelMap;

    KMutex m_DeferredNodeMutex;
    KConditionVariable m_DeferredNodeCondition;
    std::vector<int> m_DeferredDevNodeHandles;
    std::unique_ptr<DeviceNodeCleanupThread> m_CleanupThread;
    bool m_CleanupThreadStopRequested = false;
};


} // namespace kernel
