// This file is part of PadOS.
//
// Copyright (c) 2022-2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 26.05.2022 14:00

#pragma once

#include <stdint.h>
#include <Ptr/PtrTarget.h>

enum class USB_ClassCode : uint8_t;

namespace kernel
{

class USBHost;

class USBClassDriverHost : public PtrTarget
{
public:
    virtual USB_ClassCode               GetClassCode() const = 0;
    virtual const char*                 GetName() const = 0;
    virtual bool                        Init(USBHost* host) { m_HostHandler = host; return true; }
    virtual void                        Shutdown() { m_HostHandler = nullptr; }
    virtual const USB_DescriptorHeader* Open(uint8_t deviceAddress, const USB_DescInterface* interfaceDesc, const USB_DescInterfaceAssociation* interfaceAssociationDesc, const void* endDesc) = 0;
    virtual void                        Close() = 0;
    virtual void                        CloseDevice(uint8_t deviceAddress) { Close(); }
    virtual void                        Startup() = 0;
    virtual void                        StartupDevice(uint8_t deviceAddress) { Startup(); }
    virtual void                        StartOfFrame() = 0;

    bool IsActive() const { return m_IsActive; }

protected:
    friend class USBHost;
    USBHost*    m_HostHandler = nullptr;
    bool        m_IsActive = false;

};


} // namespace kernel
