// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <deque>
#include <vector>

#include <Ptr/Ptr.h>
#include <Kernel/KConditionVariable.h>
#include <Kernel/KMutex.h>
#include <Kernel/KThread.h>
#include <Kernel/USB/USBProtocolMSC.h>
#include <Kernel/USB/USBClassDriverHost.h>

namespace kernel
{
class USBHost;
class USBHostMSCInterface;

class USBHostClassMSC : public USBClassDriverHost, public KThread
{
public:
    USBHostClassMSC();
    virtual ~USBHostClassMSC();

    virtual USB_ClassCode               GetClassCode() const override;
    virtual const char*                 GetName() const override;
    virtual bool                        Init(USBHost* host) override;
    virtual void                        Shutdown() override;
    virtual const USB_DescriptorHeader* Open(uint8_t deviceAddress, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc) override;
    virtual void                        Close() override;
    virtual void                        CloseDevice(uint8_t deviceAddress) override;
    virtual void                        Startup() override;
    virtual void                        StartupDevice(uint8_t deviceAddress) override;
    virtual void                        StartOfFrame() override;

    virtual void* Run() override;

private:
    friend class USBHostMSCInterface;

    bool HasActiveInterfaces_pl() const;
    void QueueInitialization(Ptr<USBHostMSCInterface> interface);
    void QueueNodeRemovals(std::vector<int>&& nodeHandles);

    std::vector<Ptr<USBHostMSCInterface>> m_Interfaces;

    KMutex                                m_WorkMutex;
    KConditionVariable                    m_WorkCondition;
    std::deque<Ptr<USBHostMSCInterface>>  m_InitializationQueue;
    std::deque<int>                       m_NodeRemovalQueue;
    bool                                  m_StopRequested = false;
    bool                                  m_WorkerStarted = false;
};

} // namespace kernel
