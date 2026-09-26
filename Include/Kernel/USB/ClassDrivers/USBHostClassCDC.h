// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
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

    virtual USB_ClassCode               GetClassCode() const override;
    virtual const char*                 GetName() const override;
    virtual bool                        Init(USBHost* host) override;
    virtual void                        Shutdown() override;
    virtual const USB_DescriptorHeader* Open(uint8_t deviceAddr, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc) override;
    virtual void                        Close() override;
    virtual void                        CloseDevice(uint8_t deviceAddr) override;
    virtual void                        Startup() override;
    virtual void                        StartupDevice(uint8_t deviceAddr) override;
    virtual void                        StartOfFrame() override;

    uint32_t                GetChannelCount() const { return m_Channels.size(); }
    Ptr<USBHostCDCChannel>  GetChannel(uint32_t channelIndex);

    Signal<void, Ptr<USBHostCDCChannel>> SignalChannelAdded;
    Signal<void, Ptr<USBHostCDCChannel>> SignalChannelRemoved;


private:
    bool HasActiveChannels() const;

    std::vector<Ptr<USBHostCDCChannel>> m_Channels;
    uint32_t                            m_NextChannelIndex = 0;
};


} // namespace kernel
