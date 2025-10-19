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
// Created: 30.07.2022 23:00

#pragma once

#include <stdint.h>
#include <System/Sections.h>
#include <Kernel/USB/USBHostControl.h>

namespace kernel
{
class USBHost;
class USBDeviceNode;

class USBHostEnumerator
{
public:
    void Setup(USBHost* host) { m_HostHandler = host; }

    void Reset();
    bool Enumerate(USBHostControlRequestCallback&& callback);

private:
    void SendResult(bool result, uint8_t deviceAddr);

    void HandleConfigurationHeaderResult(bool result, uint8_t deviceAddr);
    void HandleConfigurationFullResult(bool result, uint8_t deviceAddr);
    void HandleSetAddressResult(bool result, uint8_t deviceAddr);
    void GetStringDescriptors(uint8_t deviceAddr);
    void HandleGetManufacturerStringResult(bool result, uint8_t deviceAddr);
    void HandleGetProductStringResult(bool result, uint8_t deviceAddr);
    void HandleGetSerialNumberResult(bool result, uint8_t deviceAddr);


    USBHost* m_HostHandler = nullptr;
    USBHostControlRequestCallback m_ResultCallback;
};

} // namespace kernel
