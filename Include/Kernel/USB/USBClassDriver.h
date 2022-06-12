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

class USBClassDriver : public PtrTarget
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
    virtual void                        StartOfFrame() {}

protected:
    USBDevice* m_DeviceHandler = nullptr;
};


} // namespace kernel
