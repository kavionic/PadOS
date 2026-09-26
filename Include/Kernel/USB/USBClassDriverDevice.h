// This file is part of PadOS.
//
// Copyright (c) 2022 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////
// Created: 26.05.2022 14:00

#pragma once

#include <stdint.h>
#include <Ptr/PtrTarget.h>

struct USB_DescriptorHeader;
struct USB_DescInterface;
struct USB_ControlRequest;

namespace kernel
{

class USBDevice;

enum class USB_TransferResult : uint8_t;
enum class USB_ControlStage : int;

class USBClassDriverDevice : public PtrTarget
{
public:
    virtual const char*                 GetName() const = 0;
    virtual uint32_t                    GetInterfaceCount() = 0;
    virtual void                        Init(USBDevice* deviceHandler) { m_DeviceHandler = deviceHandler; }
    virtual void                        Shutdown() { m_DeviceHandler = nullptr; }
    virtual void                        Reset() = 0;
    virtual const USB_DescriptorHeader* Open(USB_DescInterface const* desc_intf, const void* endDesc) = 0;
    virtual bool                        HandleControlTransfer(USB_ControlStage stage, const USB_ControlRequest& request) = 0;
    virtual bool                        HandleDataTransfer(uint8_t endpointAddr, USB_TransferResult result, uint32_t length) = 0;
    virtual void                        HandleEndpointHaltCleared(uint8_t endpointAddr) {}
    virtual void                        StartOfFrame() {}

protected:
    USBDevice* m_DeviceHandler = nullptr;
};



} // namespace kernel
