// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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
    virtual void                        HandleEndpointHaltCleared(uint8_t endpointAddr) override;

    uint32_t            GetChannelCount() const { return m_Channels.size(); }
    Ptr<USBClientCDCChannel>  GetChannel(uint32_t channelIndex);

    Signal<void, Ptr<USBClientCDCChannel>> SignalChannelAdded;
    Signal<void, Ptr<USBClientCDCChannel>> SignalChannelRemoved;

private:
    friend class USBClientCDCRecoveryTest;

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
